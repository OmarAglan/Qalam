# قلم - Qalam IDE
## المراحل 7-9: المترجم، المنقح، الميزات المتقدمة

**السابق:** [QALAM_ROADMAP_2.md](QALAM_ROADMAP_2.md) - المراحل 4-6

---

## المرحلة ٧: تكامل المترجم 🔨

**الهدف:** سير عمل بناء-تشغيل-تصحيح سلس

**التقنيات:** Process API, Pipe, Error Parsing

### 7.1 نظام البناء

```c
// === build.h ===

#ifndef QALAM_BUILD_H
#define QALAM_BUILD_H

#include <windows.h>
#include <stdbool.h>

// حالة البناء
typedef enum {
    BUILD_IDLE,
    BUILD_COMPILING,
    BUILD_LINKING,
    BUILD_SUCCESS,
    BUILD_FAILED,
    BUILD_CANCELLED,
} BuildStatus;

// نوع الإخراج
typedef enum {
    OUTPUT_EXECUTABLE,        // ملف تنفيذي
    OUTPUT_LIBRARY,           // مكتبة
    OUTPUT_OBJECT,            // ملف كائن
} OutputType;

// إعدادات البناء
typedef struct {
    wchar_t source_file[MAX_PATH];
    wchar_t output_file[MAX_PATH];
    wchar_t working_dir[MAX_PATH];
    wchar_t compiler_path[MAX_PATH];
    
    OutputType output_type;
    bool debug_info;          // معلومات التنقيح
    bool optimize;            // تحسين الكود
    bool warnings_as_errors;
    
    wchar_t* include_paths[32];
    size_t include_count;
    
    wchar_t* defines[32];
    size_t define_count;
} BuildConfig;

// نتيجة البناء
typedef struct {
    BuildStatus status;
    int exit_code;
    double duration_ms;
    
    wchar_t output_path[MAX_PATH];
    wchar_t error_message[1024];
    
    size_t error_count;
    size_t warning_count;
} BuildResult;

// مدير البناء
typedef struct {
    BuildConfig config;
    BuildResult result;
    
    HANDLE hProcess;
    HANDLE hPipeOut;
    HANDLE hPipeErr;
    HANDLE hThread;
    
    bool is_building;
    bool cancel_requested;
    
    // Callbacks
    void (*on_output)(const wchar_t* text, void* user_data);
    void (*on_error)(const wchar_t* text, void* user_data);
    void (*on_complete)(BuildResult* result, void* user_data);
    void* user_data;
} BuildManager;

// === الدوال ===
BuildManager* build_create(void);
void build_destroy(BuildManager* bm);

bool build_start(BuildManager* bm, BuildConfig* config);
void build_cancel(BuildManager* bm);
bool build_is_running(BuildManager* bm);

void build_config_init(BuildConfig* config);
void build_config_set_source(BuildConfig* config, const wchar_t* path);

#endif
```

### 7.2 تنفيذ البناء

```c
// === build.c ===

#include "build.h"
#include <process.h>

BuildManager* build_create(void) {
    BuildManager* bm = calloc(1, sizeof(BuildManager));
    return bm;
}

void build_destroy(BuildManager* bm) {
    if (!bm) return;
    
    if (bm->is_building) {
        build_cancel(bm);
    }
    
    free(bm);
}

void build_config_init(BuildConfig* config) {
    memset(config, 0, sizeof(BuildConfig));
    
    // المسار الافتراضي للمترجم
    GetModuleFileNameW(NULL, config->compiler_path, MAX_PATH);
    PathRemoveFileSpecW(config->compiler_path);
    PathAppendW(config->compiler_path, L"baa.exe");
    
    // الدليل الحالي
    GetCurrentDirectoryW(MAX_PATH, config->working_dir);
    
    config->output_type = OUTPUT_EXECUTABLE;
    config->debug_info = true;
}

void build_config_set_source(BuildConfig* config, const wchar_t* path) {
    wcscpy_s(config->source_file, MAX_PATH, path);
    
    // تعيين اسم الإخراج تلقائياً
    wcscpy_s(config->output_file, MAX_PATH, path);
    PathRenameExtensionW(config->output_file, L".exe");
}

static unsigned __stdcall build_thread(void* arg) {
    BuildManager* bm = (BuildManager*)arg;
    
    LARGE_INTEGER start_time, end_time, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start_time);
    
    // بناء سطر الأوامر
    wchar_t cmd_line[2048];
    swprintf_s(cmd_line, 2048, L"\"%s\" بناء \"%s\" -o \"%s\"",
               bm->config.compiler_path,
               bm->config.source_file,
               bm->config.output_file);
    
    if (bm->config.debug_info) {
        wcscat_s(cmd_line, 2048, L" -g");
    }
    
    // إنشاء الأنابيب
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hOutRead, hOutWrite;
    HANDLE hErrRead, hErrWrite;
    
    CreatePipe(&hOutRead, &hOutWrite, &sa, 0);
    CreatePipe(&hErrRead, &hErrWrite, &sa, 0);
    
    SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hErrRead, HANDLE_FLAG_INHERIT, 0);
    
    // إعداد العملية
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hOutWrite;
    si.hStdError = hErrWrite;
    si.hStdInput = NULL;
    
    PROCESS_INFORMATION pi = {0};
    
    BOOL success = CreateProcessW(
        NULL,
        cmd_line,
        NULL, NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        bm->config.working_dir,
        &si,
        &pi
    );
    
    CloseHandle(hOutWrite);
    CloseHandle(hErrWrite);
    
    if (!success) {
        bm->result.status = BUILD_FAILED;
        wcscpy_s(bm->result.error_message, 1024, L"فشل في تشغيل المترجم");
        bm->is_building = false;
        
        if (bm->on_complete) {
            bm->on_complete(&bm->result, bm->user_data);
        }
        return 1;
    }
    
    bm->hProcess = pi.hProcess;
    bm->hPipeOut = hOutRead;
    bm->hPipeErr = hErrRead;
    
    // قراءة الإخراج
    char buffer[4096];
    DWORD bytes_read;
    
    while (!bm->cancel_requested) {
        // قراءة stdout
        if (PeekNamedPipe(hOutRead, NULL, 0, NULL, &bytes_read, NULL) && bytes_read > 0) {
            if (ReadFile(hOutRead, buffer, sizeof(buffer) - 1, &bytes_read, NULL)) {
                buffer[bytes_read] = '\0';
                
                if (bm->on_output) {
                    wchar_t wbuffer[4096];
                    MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuffer, 4096);
                    bm->on_output(wbuffer, bm->user_data);
                }
            }
        }
        
        // قراءة stderr
        if (PeekNamedPipe(hErrRead, NULL, 0, NULL, &bytes_read, NULL) && bytes_read > 0) {
            if (ReadFile(hErrRead, buffer, sizeof(buffer) - 1, &bytes_read, NULL)) {
                buffer[bytes_read] = '\0';
                
                if (bm->on_error) {
                    wchar_t wbuffer[4096];
                    MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuffer, 4096);
                    bm->on_error(wbuffer, bm->user_data);
                }
            }
        }
        
        // التحقق من انتهاء العملية
        DWORD wait_result = WaitForSingleObject(pi.hProcess, 100);
        if (wait_result == WAIT_OBJECT_0) {
            break;
        }
    }
    
    // الحصول على كود الخروج
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    QueryPerformanceCounter(&end_time);
    
    bm->result.exit_code = exit_code;
    bm->result.status = (exit_code == 0) ? BUILD_SUCCESS : BUILD_FAILED;
    bm->result.duration_ms = (double)(end_time.QuadPart - start_time.QuadPart) 
                             / freq.QuadPart * 1000.0;
    
    if (bm->cancel_requested) {
        bm->result.status = BUILD_CANCELLED;
    }
    
    wcscpy_s(bm->result.output_path, MAX_PATH, bm->config.output_file);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutRead);
    CloseHandle(hErrRead);
    
    bm->is_building = false;
    
    if (bm->on_complete) {
        bm->on_complete(&bm->result, bm->user_data);
    }
    
    return 0;
}

bool build_start(BuildManager* bm, BuildConfig* config) {
    if (!bm || bm->is_building) return false;
    
    memcpy(&bm->config, config, sizeof(BuildConfig));
    memset(&bm->result, 0, sizeof(BuildResult));
    
    bm->result.status = BUILD_COMPILING;
    bm->is_building = true;
    bm->cancel_requested = false;
    
    bm->hThread = (HANDLE)_beginthreadex(NULL, 0, build_thread, bm, 0, NULL);
    
    return bm->hThread != NULL;
}

void build_cancel(BuildManager* bm) {
    if (!bm || !bm->is_building) return;
    
    bm->cancel_requested = true;
    
    if (bm->hProcess) {
        TerminateProcess(bm->hProcess, 1);
    }
    
    WaitForSingleObject(bm->hThread, 5000);
    CloseHandle(bm->hThread);
    bm->hThread = NULL;
}
```

### 7.3 تحليل رسائل الأخطاء

```c
// === error_parser.c ===

#include "diagnostics.h"
#include <wchar.h>
#include <stdlib.h>

// صيغة رسائل الخطأ المتوقعة:
// ملف.baa:سطر:عمود: خطأ: الرسالة
// ملف.baa:سطر:عمود: تحذير: الرسالة

bool parse_error_line(const wchar_t* line, Diagnostic* diag) {
    if (!line || !diag) return false;
    
    memset(diag, 0, sizeof(Diagnostic));
    
    // البحث عن النمط
    const wchar_t* p = line;
    
    // تخطي اسم الملف
    const wchar_t* colon1 = wcschr(p, L':');
    if (!colon1) return false;
    
    p = colon1 + 1;
    
    // رقم السطر
    wchar_t* end;
    diag->line = wcstoul(p, &end, 10);
    if (*end != L':') return false;
    
    p = end + 1;
    
    // رقم العمود
    diag->column_start = wcstoul(p, &end, 10);
    if (*end != L':') return false;
    
    p = end + 1;
    
    // تخطي المسافات
    while (*p == L' ') p++;
    
    // نوع الرسالة
    if (wcsncmp(p, L"خطأ", 3) == 0 || wcsncmp(p, L"error", 5) == 0) {
        diag->severity = DIAG_ERROR;
    } else if (wcsncmp(p, L"تحذير", 5) == 0 || wcsncmp(p, L"warning", 7) == 0) {
        diag->severity = DIAG_WARNING;
    } else if (wcsncmp(p, L"ملاحظة", 6) == 0 || wcsncmp(p, L"note", 4) == 0) {
        diag->severity = DIAG_INFO;
    } else {
        return false;
    }
    
    // البحث عن ":"  بعد نوع الرسالة
    const wchar_t* msg_start = wcschr(p, L':');
    if (!msg_start) return false;
    
    msg_start++;
    while (*msg_start == L' ') msg_start++;
    
    // نسخ الرسالة
    wcsncpy_s(diag->message, 256, msg_start, _TRUNCATE);
    
    // إزالة السطر الجديد
    size_t len = wcslen(diag->message);
    if (len > 0 && diag->message[len - 1] == L'\n') {
        diag->message[len - 1] = L'\0';
    }
    
    diag->column_end = diag->column_start + 1;
    
    return true;
}

void diagnostics_parse_compiler_output(DiagnosticList* list, const wchar_t* output) {
    if (!list || !output) return;
    
    diagnostics_clear(list);
    
    const wchar_t* line_start = output;
    const wchar_t* line_end;
    
    while (*line_start) {
        line_end = wcschr(line_start, L'\n');
        if (!line_end) {
            line_end = line_start + wcslen(line_start);
        }
        
        // نسخ السطر
        size_t line_len = line_end - line_start;
        wchar_t* line = malloc((line_len + 1) * sizeof(wchar_t));
        wcsncpy(line, line_start, line_len);
        line[line_len] = L'\0';
        
        // تحليل السطر
        Diagnostic diag;
        if (parse_error_line(line, &diag)) {
            diagnostics_add(list, &diag);
        }
        
        free(line);
        
        if (*line_end == L'\0') break;
        line_start = line_end + 1;
    }
}
```

### 7.4 التشغيل

```c
// === runner.h ===

typedef struct {
    wchar_t executable[MAX_PATH];
    wchar_t arguments[1024];
    wchar_t working_dir[MAX_PATH];
    wchar_t environment[4096];
    
    bool capture_output;
    bool pause_on_exit;
} RunConfig;

typedef struct {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD process_id;
    
    HANDLE hPipeIn;
    HANDLE hPipeOut;
    
    bool is_running;
    int exit_code;
    
    void (*on_output)(const wchar_t* text, void* user_data);
    void (*on_exit)(int exit_code, void* user_data);
    void* user_data;
} RunSession;

// === الدوال ===
RunSession* runner_create(void);
void runner_destroy(RunSession* session);

bool runner_start(RunSession* session, RunConfig* config);
void runner_stop(RunSession* session);
void runner_send_input(RunSession* session, const wchar_t* text);
bool runner_is_running(RunSession* session);
```

### 7.5 مهام المرحلة ٧

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| استدعاء المترجم | تشغيل `baa.exe` كعملية فرعية | ⬜ |
| التقاط الإخراج | توجيه stdout/stderr | ⬜ |
| تحليل الأخطاء | استخراج سطر/عمود من رسائل الخطأ | ⬜ |
| الانتقال للخطأ | نقر الخطأ → الانتقال للموقع | ⬜ |
| تقدم البناء | عرض حالة التجميع | ⬜ |
| زر التشغيل | F5 للبناء والتشغيل | ⬜ |
| زر الإيقاف | إنهاء العملية الجارية | ⬜ |
| إعدادات التشغيل | تكوينات تشغيل متعددة | ⬜ |

---

## المرحلة ٨: المُنقِّح (Debugger) 🐛

**الهدف:** تنقيح تفاعلي للبرامج

**التقنيات:** Debug API, Symbol Files

### 8.1 هيكل المُنقِّح

```c
// === debugger.h ===

#ifndef QALAM_DEBUGGER_H
#define QALAM_DEBUGGER_H

#include <windows.h>
#include <stdbool.h>

// حالة المُنقِّح
typedef enum {
    DEBUG_IDLE,
    DEBUG_RUNNING,
    DEBUG_PAUSED,
    DEBUG_STEPPING,
    DEBUG_TERMINATED,
} DebugState;

// نوع نقطة التوقف
typedef enum {
    BP_LINE,                  // سطر معين
    BP_FUNCTION,              // بداية دالة
    BP_CONDITION,             // شرط معين
    BP_DATA,                  // تغيير بيانات
} BreakpointType;

// نقطة توقف
typedef struct {
    int id;
    BreakpointType type;
    
    wchar_t file[MAX_PATH];
    size_t line;
    wchar_t function_name[128];
    wchar_t condition[256];
    
    bool enabled;
    bool temporary;           // تُحذف بعد الوصول
    int hit_count;
} Breakpoint;

// إطار في مكدس الاستدعاء
typedef struct {
    size_t index;
    wchar_t function_name[128];
    wchar_t file[MAX_PATH];
    size_t line;
    void* address;
    void* frame_pointer;
} StackFrame;

// متغير
typedef struct {
    wchar_t name[64];
    wchar_t type[32];
    wchar_t value[256];
    void* address;
    size_t size;
    bool is_pointer;
    bool is_array;
    size_t array_length;
} DebugVariable;

// مراقبة (Watch)
typedef struct {
    wchar_t expression[128];
    wchar_t value[256];
    wchar_t type[32];
    bool is_valid;
    wchar_t error[128];
} WatchItem;

// المُنقِّح
typedef struct {
    DebugState state;
    
    // العملية
    HANDLE hProcess;
    HANDLE hThread;
    DWORD process_id;
    DWORD thread_id;
    
    // الملف التنفيذي
    wchar_t executable[MAX_PATH];
    wchar_t arguments[1024];
    wchar_t working_dir[MAX_PATH];
    
    // نقاط التوقف
    Breakpoint* breakpoints;
    size_t breakpoint_count;
    size_t breakpoint_capacity;
    int next_breakpoint_id;
    
    // الموقع الحالي
    wchar_t current_file[MAX_PATH];
    size_t current_line;
    void* current_address;
    
    // مكدس الاستدعاء
    StackFrame* call_stack;
    size_t stack_depth;
    size_t selected_frame;
    
    // المتغيرات
    DebugVariable* locals;
    size_t local_count;
    DebugVariable* globals;
    size_t global_count;
    
    // المراقبات
    WatchItem* watches;
    size_t watch_count;
    
    // Callbacks
    void (*on_state_change)(DebugState state, void* user_data);
    void (*on_breakpoint_hit)(Breakpoint* bp, void* user_data);
    void (*on_output)(const wchar_t* text, void* user_data);
    void* user_data;
    
} Debugger;

// === إنشاء وتدمير ===
Debugger* debugger_create(void);
void debugger_destroy(Debugger* dbg);

// === التحكم ===
bool debugger_start(Debugger* dbg, const wchar_t* exe, 
                    const wchar_t* args, const wchar_t* cwd);
void debugger_stop(Debugger* dbg);
void debugger_pause(Debugger* dbg);
void debugger_continue(Debugger* dbg);

// === التنفيذ التدريجي ===
void debugger_step_over(Debugger* dbg);    // الخطوة التالية
void debugger_step_into(Debugger* dbg);    // الدخول في الدالة
void debugger_step_out(Debugger* dbg);     // الخروج من الدالة
void debugger_run_to_cursor(Debugger* dbg, const wchar_t* file, size_t line);

// === نقاط التوقف ===
int debugger_add_breakpoint(Debugger* dbg, const wchar_t* file, size_t line);
int debugger_add_function_breakpoint(Debugger* dbg, const wchar_t* func_name);
int debugger_add_conditional_breakpoint(Debugger* dbg, const wchar_t* file, 
                                        size_t line, const wchar_t* condition);
void debugger_remove_breakpoint(Debugger* dbg, int bp_id);
void debugger_enable_breakpoint(Debugger* dbg, int bp_id, bool enabled);
void debugger_clear_all_breakpoints(Debugger* dbg);
Breakpoint* debugger_get_breakpoint_at(Debugger* dbg, const wchar_t* file, size_t line);

// === المعلومات ===
void debugger_get_call_stack(Debugger* dbg);
void debugger_get_locals(Debugger* dbg);
void debugger_get_globals(Debugger* dbg);
bool debugger_evaluate(Debugger* dbg, const wchar_t* expr, wchar_t* result, size_t max_len);

// === المراقبات ===
void debugger_add_watch(Debugger* dbg, const wchar_t* expression);
void debugger_remove_watch(Debugger* dbg, size_t index);
void debugger_update_watches(Debugger* dbg);

#endif
```

### 8.2 واجهة المُنقِّح

```
┌─────────────────────────────────────────────────────────────────────┐
│  ملف   تحرير   عرض   بناء   تنقيح   مساعدة                          │
├─────────────────────────────────────────────────────────────────────┤
│  [⏵] [⏸] [⏹] [⟳] [↓] [↗] [↩]                                       │
│  تشغيل إيقاف توقف  تالي دخول خروج متابعة                            │
├──────────────────────────────────────────┬──────────────────────────┤
│                                          │  ╔═══ نقاط التوقف ═══╗   │
│  ┌─ main.baa ─────────────────────────┐  │  ║                    ║   │
│  │ ○  ١  صحيح الرئيسية() {            │  │  ║ ● main.baa:5      ║   │
│  │ ○  ٢      صحيح س = ١٠.             │  │  ║ ○ main.baa:12     ║   │
│  │ ○  ٣      صحيح ن = ٢٠.             │  │  ║                    ║   │
│  │ ○  ٤                               │  │  ╚════════════════════╝   │
│  │ ●  ٥      صحيح ناتج = جمع(س، ن).   │  │                          │
│  │ ➤  ٦      اطبع ناتج.               │  │  ╔═══ المتغيرات ═════╗   │
│  │ ○  ٧                               │  │  ║ س    : صحيح = ١٠  ║   │
│  │ ○  ٨      إرجع ٠.                  │  │  ║ ن    : صحيح = ٢٠  ║   │
│  │ ○  ٩  }                            │  │  ║ ناتج: صحيح = ٣٠  ║   │
│  └────────────────────────────────────┘  │  ╚════════════════════╝   │
│                                          │                          │
├──────────────────────────────────────────┤  ╔═══ مكدس الاستدعاء ═╗   │
│  ╔═══ الطرفية ════════════════════════╗  │  ║ ٠ الرئيسية:6     ║   │
│  ║ [تنقيح] بدء التنقيح...              ║  │  ║ ١ جمع:3          ║   │
│  ║ [توقف] نقطة توقف في main.baa:5     ║  │  ╚════════════════════╝   │
│  ║                                     ║  │                          │
│  ╚═════════════════════════════════════╝  │  ╔═══ المراقبات ═════╗   │
│                                          │  ║ س + ن = ٣٠       ║   │
└──────────────────────────────────────────┴──╚════════════════════╝───┘
```

### 8.3 تنفيذ نقاط التوقف

```c
// === breakpoints.c ===

#include "debugger.h"

int debugger_add_breakpoint(Debugger* dbg, const wchar_t* file, size_t line) {
    if (!dbg) return -1;
    
    // التحقق من عدم وجود نقطة توقف في نفس الموقع
    for (size_t i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].line == line &&
            wcscmp(dbg->breakpoints[i].file, file) == 0) {
            return dbg->breakpoints[i].id;
        }
    }
    
    // توسيع المصفوفة إذا لزم
    if (dbg->breakpoint_count >= dbg->breakpoint_capacity) {
        dbg->breakpoint_capacity = dbg->breakpoint_capacity ? 
                                   dbg->breakpoint_capacity * 2 : 16;
        dbg->breakpoints = realloc(dbg->breakpoints, 
                                   dbg->breakpoint_capacity * sizeof(Breakpoint));
    }
    
    // إنشاء نقطة التوقف
    Breakpoint* bp = &dbg->breakpoints[dbg->breakpoint_count++];
    memset(bp, 0, sizeof(Breakpoint));
    
    bp->id = dbg->next_breakpoint_id++;
    bp->type = BP_LINE;
    wcscpy_s(bp->file, MAX_PATH, file);
    bp->line = line;
    bp->enabled = true;
    
    // إذا كان التنقيح جارياً، تفعيل نقطة التوقف
    if (dbg->state != DEBUG_IDLE) {
        debugger_set_breakpoint_active(dbg, bp, true);
    }
    
    return bp->id;
}

void debugger_remove_breakpoint(Debugger* dbg, int bp_id) {
    if (!dbg) return;
    
    for (size_t i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].id == bp_id) {
            // إلغاء تفعيل نقطة التوقف
            if (dbg->state != DEBUG_IDLE) {
                debugger_set_breakpoint_active(dbg, &dbg->breakpoints[i], false);
            }
            
            // حذف من المصفوفة
            memmove(&dbg->breakpoints[i], 
                    &dbg->breakpoints[i + 1],
                    (dbg->breakpoint_count - i - 1) * sizeof(Breakpoint));
            dbg->breakpoint_count--;
            return;
        }
    }
}

void debugger_toggle_breakpoint(Debugger* dbg, const wchar_t* file, size_t line) {
    Breakpoint* bp = debugger_get_breakpoint_at(dbg, file, line);
    
    if (bp) {
        debugger_remove_breakpoint(dbg, bp->id);
    } else {
        debugger_add_breakpoint(dbg, file, line);
    }
}
```

### 8.4 التنفيذ التدريجي

```c
// === stepping.c ===

void debugger_step_over(Debugger* dbg) {
    if (!dbg || dbg->state != DEBUG_PAUSED) return;
    
    dbg->state = DEBUG_STEPPING;
    
    // الحصول على معلومات السطر الحالي
    size_t current_line = dbg->current_line;
    
    // تعيين نقطة توقف مؤقتة على السطر التالي
    // أو نهاية الدالة الحالية
    
    // متابعة التنفيذ
    debugger_continue_internal(dbg);
}

void debugger_step_into(Debugger* dbg) {
    if (!dbg || dbg->state != DEBUG_PAUSED) return;
    
    dbg->state = DEBUG_STEPPING;
    
    // تعيين وضع single-step
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL;
    GetThreadContext(dbg->hThread, &ctx);
    
    ctx.EFlags |= 0x100;  // Trap Flag
    SetThreadContext(dbg->hThread, &ctx);
    
    // متابعة التنفيذ
    ContinueDebugEvent(dbg->process_id, dbg->thread_id, DBG_CONTINUE);
}

void debugger_step_out(Debugger* dbg) {
    if (!dbg || dbg->state != DEBUG_PAUSED) return;
    
    dbg->state = DEBUG_STEPPING;
    
    // الحصول على عنوان الإرجاع من المكدس
    if (dbg->stack_depth > 1) {
        StackFrame* parent_frame = &dbg->call_stack[1];
        
        // تعيين نقطة توقف مؤقتة عند عنوان الإرجاع
        Breakpoint temp_bp = {0};
        temp_bp.temporary = true;
        // ...
    }
    
    debugger_continue_internal(dbg);
}
```

### 8.5 مهام المرحلة ٨

| المهمة | الوصف | الأولوية | الحالة |
|--------|-------|----------|--------|
| نقاط التوقف | إضافة/إزالة Breakpoints | 🔴 عالية | ⬜ |
| التنفيذ خطوة بخطوة | Step Over, Step Into, Step Out | 🔴 عالية | ⬜ |
| مشاهدة المتغيرات | عرض قيم المتغيرات | 🔴 عالية | ⬜ |
| مكدس الاستدعاء | عرض Call Stack | 🟡 متوسطة | ⬜ |
| عرض الذاكرة | Memory Viewer | 🟢 منخفضة | ⬜ |
| المراقبات | مراقبة تعبيرات محددة | 🟡 متوسطة | ⬜ |
| تعديل القيم | تغيير قيم المتغيرات | 🟢 منخفضة | ⬜ |
| نقاط توقف شرطية | Conditional Breakpoints | 🟢 منخفضة | ⬜ |

---

## المرحلة ٩: الميزات المتقدمة 🚀

**الهدف:** ميزات IDE متقدمة لتحسين الإنتاجية

### 9.1 نظام الإعدادات

```c
// === settings.h ===

#ifndef QALAM_SETTINGS_H
#define QALAM_SETTINGS_H

typedef struct {
    // === المحرر ===
    struct {
        wchar_t font_name[64];
        int font_size;
        bool rtl_default;
        bool show_line_numbers;
        bool line_numbers_on_right;
        int tab_size;
        bool use_spaces;
        bool auto_indent;
        bool word_wrap;
        bool show_whitespace;
        bool highlight_current_line;
        bool auto_closing_brackets;
        bool auto_closing_quotes;
    } editor;
    
    // === المظهر ===
    struct {
        wchar_t theme[32];
        bool rtl_ui;
        bool show_minimap;
        float minimap_width;
        bool show_breadcrumbs;
    } appearance;
    
    // === الطرفية ===
    struct {
        wchar_t font_name[64];
        int font_size;
        int scrollback_lines;
        wchar_t default_shell[MAX_PATH];
        bool rtl_support;
        bool copy_on_select;
    } terminal;
    
    // === المترجم ===
    struct {
        wchar_t compiler_path[MAX_PATH];
        wchar_t output_dir[MAX_PATH];
        bool auto_save_before_build;
        bool show_warnings;
        bool treat_warnings_as_errors;
    } compiler;
    
    // === الملفات ===
    struct {
        bool auto_save;
        int auto_save_interval;       // بالثواني
        bool trim_trailing_whitespace;
        bool insert_final_newline;
        int default_encoding;         // UTF-8, UTF-16, etc.
        int default_line_ending;      // LF, CRLF
    } files;
    
    // === اختصارات لوحة المفاتيح ===
    struct {
        wchar_t action[64];
        wchar_t shortcut[32];
    } keybindings[256];
    size_t keybinding_count;
    
} QalamSettings;

// === الدوال ===
QalamSettings* settings_create(void);
void settings_destroy(QalamSettings* settings);
void settings_load(QalamSettings* settings, const wchar_t* path);
void settings_save(QalamSettings* settings, const wchar_t* path);
void settings_reset_defaults(QalamSettings* settings);

// === مسار الإعدادات ===
// %APPDATA%\Qalam\settings.json

#endif
```

### 9.2 إدارة الجلسات

```c
// === session.h ===

typedef struct {
    wchar_t path[MAX_PATH];
    size_t cursor_line;
    size_t cursor_column;
    size_t scroll_position;
    bool is_active;
    bool is_modified;
} OpenFile;

typedef struct {
    // الملفات المفتوحة
    OpenFile files[64];
    size_t file_count;
    
    // حالة النافذة
    int window_x, window_y;
    int window_width, window_height;
    bool is_maximized;
    
    // تقسيم الألواح
    float explorer_width;
    float terminal_height;
    bool explorer_visible;
    bool terminal_visible;
    
    // المشروع
    wchar_t project_path[MAX_PATH];
    
    // آخر بحث
    wchar_t last_search[256];
    wchar_t last_replace[256];
    bool search_case_sensitive;
    bool search_whole_word;
    bool search_regex;
    
    // التاريخ
    wchar_t recent_files[32][MAX_PATH];
    size_t recent_count;
    wchar_t recent_projects[16][MAX_PATH];
    size_t recent_project_count;
    
} QalamSession;

// === الدوال ===
QalamSession* session_create(void);
void session_destroy(QalamSession* session);
void session_load(QalamSession* session, const wchar_t* path);
void session_save(QalamSession* session, const wchar_t* path);
void session_restore(QalamSession* session, QalamWindow* window);
```

### 9.3 إدارة المشاريع

```c
// === project.h ===

typedef struct {
    wchar_t name[64];
    wchar_t path[MAX_PATH];
    wchar_t version[16];
    wchar_t description[256];
    wchar_t author[64];
    
    // الملفات المصدرية
    wchar_t source_dir[MAX_PATH];
    wchar_t main_file[MAX_PATH];
    wchar_t** source_files;
    size_t source_count;
    
    // البناء
    wchar_t output_dir[MAX_PATH];
    wchar_t output_name[64];
    
    // التبعيات
    struct {
        wchar_t name[64];
        wchar_t version[32];
    } dependencies[32];
    size_t dependency_count;
    
} QalamProject;

// === ملف المشروع: qalam.json ===
/*
{
    "الاسم": "تطبيقي",
    "الإصدار": "1.0.0",
    "الوصف": "تطبيق باء رائع",
    "المؤلف": "اسم المبرمج",
    "الرئيسي": "src/main.baa",
    "الإخراج": "bin",
    "تبعيات": {}
}
*/
```

### 9.4 قوالب المشاريع

```c
// === templates.h ===

typedef struct {
    wchar_t name[64];
    wchar_t description[256];
    wchar_t icon[32];
    
    // الملفات المُنشأة
    struct {
        wchar_t path[MAX_PATH];       // المسار النسبي
        const wchar_t* content;       // المحتوى
    } files[16];
    size_t file_count;
    
} ProjectTemplate;

// === القوالب المدمجة ===

static const wchar_t* TEMPLATE_EMPTY_MAIN = 
L"صحيح الرئيسية() {\n"
L"    إرجع ٠.\n"
L"}\n";

static const wchar_t* TEMPLATE_HELLO_WORLD = 
L"// برنامج مرحباً بالعالم\n"
L"\n"
L"صحيح الرئيسية() {\n"
L"    اطبع \"مرحباً بالعالم!\".\n"
L"    إرجع ٠.\n"
L"}\n";

static const wchar_t* TEMPLATE_CONSOLE_APP = 
L"// تطبيق سطر أوامر\n"
L"\n"
L"صحيح الرئيسية() {\n"
L"    اطبع \"أهلاً بك في تطبيقي!\".\n"
L"    اطبع \"\".\n"
L"    \n"
L"    // الكود الخاص بك هنا\n"
L"    \n"
L"    إرجع ٠.\n"
L"}\n";

static const ProjectTemplate TEMPLATES[] = {
    {
        L"فارغ",
        L"مشروع فارغ مع ملف رئيسي بسيط",
        L"📄",
        {
            {L"main.baa", TEMPLATE_EMPTY_MAIN},
        },
        1
    },
    {
        L"مرحباً بالعالم",
        L"برنامج بسيط يطبع مرحباً بالعالم",
        L"👋",
        {
            {L"main.baa", TEMPLATE_HELLO_WORLD},
        },
        1
    },
    {
        L"تطبيق سطر أوامر",
        L"تطبيق سطر أوامر مع هيكل أساسي",
        L"💻",
        {
            {L"src/main.baa", TEMPLATE_CONSOLE_APP},
            {L"qalam.json", NULL},  // يُنشأ تلقائياً
        },
        2
    },
};
```

### 9.5 البحث والاستبدال

```c
// === search.h ===

typedef struct {
    wchar_t query[256];
    wchar_t replace[256];
    
    bool case_sensitive;
    bool whole_word;
    bool use_regex;
    bool in_selection;
    
    // نتائج البحث
    struct {
        wchar_t file[MAX_PATH];
        size_t line;
        size_t column;
        size_t length;
        wchar_t context[128];         // السطر المحيط
    } results[1000];
    size_t result_count;
    size_t current_result;
    
    bool search_in_progress;
} SearchContext;

// === الدوال ===
SearchContext* search_create(void);
void search_destroy(SearchContext* ctx);

// بحث في الملف الحالي
void search_in_file(SearchContext* ctx, QalamBuffer* buf);
void search_next(SearchContext* ctx, QalamBuffer* buf, size_t from_pos);
void search_prev(SearchContext* ctx, QalamBuffer* buf, size_t from_pos);

// بحث في المشروع
void search_in_project(SearchContext* ctx, const wchar_t* project_dir);
void search_cancel(SearchContext* ctx);

// استبدال
void search_replace_current(SearchContext* ctx, QalamBuffer* buf);
void search_replace_all(SearchContext* ctx, QalamBuffer* buf);
```

### 9.6 طي الكود (Code Folding)

```c
// === folding.h ===

typedef struct {
    size_t start_line;
    size_t end_line;
    bool is_folded;
    int level;                        // مستوى التداخل
} FoldRegion;

typedef struct {
    FoldRegion* regions;
    size_t count;
    size_t capacity;
} FoldingState;

// === الدوال ===
FoldingState* folding_create(void);
void folding_destroy(FoldingState* state);
void folding_analyze(FoldingState* state, QalamBuffer* buf);
void folding_toggle(FoldingState* state, size_t line);
void folding_fold_all(FoldingState* state);
void folding_unfold_all(FoldingState* state);
bool folding_is_line_visible(FoldingState* state, size_t line);
size_t folding_visible_line_count(FoldingState* state);
```

### 9.7 Minimap

```c
// === minimap.h ===

typedef struct {
    HWND hwnd;
    
    int width;                        // 100 بكسل تقريباً
    int char_width;                   // 2 بكسل
    int char_height;                  // 1-2 بكسل
    
    // المنطقة المرئية
    size_t visible_start_line;
    size_t visible_end_line;
    
    // الألوان (مصغرة)
    COLORREF* line_colors;
    size_t line_count;
    
    bool dragging;
} Minimap;

void minimap_paint(Minimap* mm, HDC hdc, QalamBuffer* buf, 
                   SyntaxHighlighter* hl);
void minimap_on_click(Minimap* mm, int y, QalamEditor* editor);
```

### 9.8 مهام المرحلة ٩

| المهمة | الوصف | الأولوية | الحالة |
|--------|-------|----------|--------|
| نظام الإعدادات | تحميل/حفظ الإعدادات JSON | 🔴 عالية | ⬜ |
| واجهة الإعدادات | نافذة إعدادات رسومية | 🟡 متوسطة | ⬜ |
| إدارة الجلسات | حفظ/استعادة حالة IDE | 🔴 عالية | ⬜ |
| ملفات المشروع | `.qalam` project files | 🟡 متوسطة | ⬜ |
| مستكشف الملفات | شجرة ملفات مع أيقونات | 🔴 عالية | ⬜ |
| دعم Multi-file | فتح ملفات متعددة | 🔴 عالية | ⬜ |
| التبويبات | شريط تبويبات الملفات | 🔴 عالية | ⬜ |
| المشاريع الأخيرة | قائمة مشاريع حديثة | 🟡 متوسطة | ⬜ |
| بحث في ملف | Ctrl+F | 🔴 عالية | ⬜ |
| بحث في مشروع | Ctrl+Shift+F | 🟡 متوسطة | ⬜ |
| استبدال | بحث واستبدال مع معاينة | 🟡 متوسطة | ⬜ |
| دعم Regex | تعبيرات نمطية | 🟢 منخفضة | ⬜ |
| قوالب المشاريع | معالج مشروع جديد | 🟡 متوسطة | ⬜ |
| طي الكود | طي/فتح الكتل | 🟡 متوسطة | ⬜ |
| Minimap | خريطة مصغرة | 🟢 منخفضة | ⬜ |
| السمة الداكنة | سمة افتراضية داكنة | 🔴 عالية | ⬜ |
| السمة الفاتحة | سمة فاتحة اختيارية | 🟢 منخفضة | ⬜ |

---

## 📊 ملخص المراحل 7-9

| المرحلة | المكون | الحالة | الأهمية |
|---------|--------|--------|---------|
| ٧ | Compiler Integration | ⬜ لم يبدأ | 🔴 حرجة |
| ٨ | Debugger | ⬜ لم يبدأ | 🟡 عالية |
| ٩ | Advanced Features | ⬜ لم يبدأ | 🟡 عالية |

---

**السابق:** [QALAM_ROADMAP_2.md](QALAM_ROADMAP_2.md)
**التالي:** [QALAM_ROADMAP_4.md](QALAM_ROADMAP_4.md) - المراحل 10-12، المواصفات التقنية