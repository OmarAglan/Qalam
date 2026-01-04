# قلم - Qalam IDE
## بيئة التطوير المتكاملة للغة باء

<div align="center">

```
   ██████╗  █████╗ ██╗      █████╗ ███╗   ███╗
  ██╔═══██╗██╔══██╗██║     ██╔══██╗████╗ ████║
  ██║   ██║███████║██║     ███████║██╔████╔██║
  ██║▄▄ ██║██╔══██║██║     ██╔══██║██║╚██╔╝██║
  ╚██████╔╝██║  ██║███████╗██║  ██║██║ ╚═╝ ██║
   ╚══▀▀═╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝
   
              قَــــلَــــم
        محرر عربي لبرمجة باء
```

**الإصدار المخطط:** 1.0 | **الحالة:** 📋 في مرحلة التخطيط

*بيئة تطوير متكاملة احترافية مبنية من الصفر بلغة C*

</div>

---

## 📚 فهرس الوثائق

| الملف | المحتوى |
|-------|---------|
| **QALAM_ROADMAP_1.md** | النظرة العامة، الرؤية، المراحل 1-3 |
| **QALAM_ROADMAP_2.md** | المراحل 4-6 (المحرر، الطرفية، IDE) |
| **QALAM_ROADMAP_3.md** | المراحل 7-9 (المترجم، المنقح، متقدم) |
| **QALAM_ROADMAP_4.md** | المراحل 10-12، المواصفات التقنية |

---

## 🎯 الرؤية والأهداف

### بيان المهمة

بناء **بيئة تطوير متكاملة احترافية** مصممة خصيصاً للبرمجة بالعربية مع دعم كامل لـ RTL، طرفية مدمجة تدعم العربية، وتجربة مطور سلسة.

### المشاكل التي نحلها

```
┌─────────────────────────────────────────────────────────────────┐
│                 مشاكل المحررات الحالية مع العربية                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ❌ لا دعم حقيقي لـ RTL - النص يظهر معكوساً                      │
│  ❌ حركة المؤشر غريبة وغير منطقية                                │
│  ❌ التحديد (Selection) يعمل بشكل خاطئ                           │
│  ❌ CMD و PowerShell لا تعرض العربية بشكل صحيح                   │
│  ❌ الخطوط لا تدعم الحروف العربية المتصلة                         │
│  ❌ أرقام الأسطر على الجهة الخاطئة                               │
│  ❌ القوائم وواجهة المستخدم LTR فقط                              │
│  ❌ لا يوجد محرر عربي متخصص للبرمجة                              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### المبادئ الأساسية

| المبدأ | الوصف |
|--------|-------|
| **العربية أولاً** | واجهة RTL كاملة، خطوط عربية، طرفية عربية |
| **أداء محلي** | مكتوب بـ C، بدون Electron أو تقنيات الويب |
| **مستقل** | أقل تبعيات ممكنة، ملفات تنفيذية محمولة |
| **من الصفر** | محرك نصوص مخصص، رندر مخصص، كل شيء مخصص |
| **تجربة متكاملة** | طرفية مدمجة تحل مشكلة CMD/PowerShell |

### المخرجات النهائية

```
qalam.exe        → بيئة التطوير المتكاملة الرسومية
qalam-cli.exe    → واجهة سطر الأوامر
qalam-term.exe   → الطرفية المستقلة (اختياري)
baa-pkg.exe      → مدير الحزم
```

---

## 🏗️ الهيكل العام

```
┌─────────────────────────────────────────────────────────────────┐
│                         قلم - Qalam IDE                         │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────┐ ┌──────────────────────────────────┐ ┌──────────┐ │
│  │         │ │            المحرر                 │ │          │ │
│  │         │ │     ┌──────────────────────┐     │ │  مستكشف  │ │
│  │         │ │     │   صحيح الرئيسية() {  │     │ │  الملفات │ │
│  │ شريط    │ │  ١  │       اطبع "مرحباً". │     │ │          │ │
│  │ الأدوات │ │  ٢  │       إرجع ٠.        │     │ │  ├─ src/ │ │
│  │         │ │  ٣  │   }                  │     │ │  │ └─app │ │
│  │         │ │     └──────────────────────┘     │ │  └─ res/ │ │
│  │         │ ├──────────────────────────────────┤ │          │ │
│  │         │ │         الطرفية المدمجة          │ │          │ │
│  │         │ │  qalam> baa بناء main.baa        │ │          │ │
│  │         │ │  ✓ تم البناء بنجاح               │ │          │ │
│  └─────────┘ └──────────────────────────────────┘ └──────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  سطر: ٥ │ عمود: ١٢ │ UTF-8 │ باء │ Tab: ٤ │ RTL          قلم   │
└─────────────────────────────────────────────────────────────────┘
```

### طبقات النظام

```
┌─────────────────────────────────────────────────────────────────┐
│                        واجهة المستخدم                            │
│    المحرر │ الطرفية │ المستكشف │ القوائم │ الحوارات │ الإعدادات   │
├─────────────────────────────────────────────────────────────────┤
│                        طبقة الخدمات                              │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐        │
│  │ Lexer  │ │ Parser │ │ Symbols│ │ Errors │ │  Bidi  │        │
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────┘        │
├─────────────────────────────────────────────────────────────────┤
│                         طبقة النواة                              │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐   │
│  │ Gap Buffer │ │   Undo     │ │  Session   │ │  Settings  │   │
│  └────────────┘ └────────────┘ └────────────┘ └────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                      تجريد المنصة                                │
│         ┌─────────────────────────────────────────┐             │
│         │   Win32 API │ GDI+ │ DirectWrite │ ConPTY │           │
│         └─────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📊 خارطة المراحل

```
┌─────────────────────────────────────────────────────────────────┐
│                      مراحل تطوير قلم                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ══════════════════ المرحلة الأولى: الأساسيات ══════════════════ │
│                                                                 │
│  [١] CLI Foundation        ████████░░░░░░░░░░░░  أساس CLI       │
│  [٢] Syntax Highlighter    ████████░░░░░░░░░░░░  تلوين الصياغة  │
│  [٣] GUI Core              ████████░░░░░░░░░░░░  نواة الواجهة   │
│                                                                 │
│  ══════════════════ المرحلة الثانية: المحرر ═══════════════════ │
│                                                                 │
│  [٤] Text Engine           ████████████░░░░░░░░  محرك النصوص   │
│  [٥] Integrated Terminal   ████████████░░░░░░░░  الطرفية       │
│  [٦] IDE Features          ████████████░░░░░░░░  ميزات IDE     │
│                                                                 │
│  ══════════════════ المرحلة الثالثة: التكامل ══════════════════ │
│                                                                 │
│  [٧] Compiler Integration  ████████████████░░░░  تكامل المترجم │
│  [٨] Debugger              ████████████████░░░░  المنقح        │
│  [٩] Advanced Features     ████████████████░░░░  متقدم         │
│                                                                 │
│  ══════════════════ المرحلة الرابعة: التوسع ═══════════════════ │
│                                                                 │
│  [١٠] Package Manager      ████████████████████  مدير الحزم    │
│  [١١] Plugin System        ████████████████████  الإضافات      │
│  [١٢] Testing Framework    ████████████████████  الاختبارات    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## المرحلة ١: أساس CLI (`qalam-cli`) 🔧

**الهدف:** واجهة سطر أوامر تدعم العربية بالكامل

**التقنيات:** C, Win32 Console API, UTF-16

### 1.1 إعداد الطرفية

```c
// === qalam_cli.h ===

#ifndef QALAM_CLI_H
#define QALAM_CLI_H

#include <windows.h>
#include <stdbool.h>

// معلومات الطرفية
typedef struct {
    HANDLE hStdOut;
    HANDLE hStdIn;
    HANDLE hStdErr;
    
    DWORD original_out_mode;
    DWORD original_in_mode;
    UINT original_cp_out;
    UINT original_cp_in;
    
    bool vt_supported;            // دعم VT100
    bool arabic_font_set;         // تم تعيين خط عربي
    
    wchar_t font_name[LF_FACESIZE];
    int font_size;
} QalamConsole;

// تهيئة الطرفية
bool qalam_console_init(QalamConsole* console);
void qalam_console_cleanup(QalamConsole* console);

// الإخراج
void qalam_print(const wchar_t* text);
void qalam_print_colored(const wchar_t* text, int color);
void qalam_print_error(const wchar_t* text);
void qalam_print_success(const wchar_t* text);
void qalam_print_warning(const wchar_t* text);

// RTL
void qalam_print_rtl(const wchar_t* text);
void qalam_set_rtl_mode(bool enabled);

#endif
```

```c
// === qalam_cli.c ===

#include "qalam_cli.h"
#include <stdio.h>

// ألوان ANSI
#define ANSI_RESET      L"\033[0m"
#define ANSI_RED        L"\033[31m"
#define ANSI_GREEN      L"\033[32m"
#define ANSI_YELLOW     L"\033[33m"
#define ANSI_BLUE       L"\033[34m"
#define ANSI_MAGENTA    L"\033[35m"
#define ANSI_CYAN       L"\033[36m"
#define ANSI_WHITE      L"\033[37m"
#define ANSI_BOLD       L"\033[1m"

bool qalam_console_init(QalamConsole* console) {
    if (!console) return false;
    
    // الحصول على المقابض
    console->hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    console->hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    console->hStdErr = GetStdHandle(STD_ERROR_HANDLE);
    
    if (console->hStdOut == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // حفظ الإعدادات الأصلية
    GetConsoleMode(console->hStdOut, &console->original_out_mode);
    GetConsoleMode(console->hStdIn, &console->original_in_mode);
    console->original_cp_out = GetConsoleOutputCP();
    console->original_cp_in = GetConsoleCP();
    
    // تعيين UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // تفعيل VT100 للألوان
    DWORD out_mode = console->original_out_mode;
    out_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    out_mode |= ENABLE_PROCESSED_OUTPUT;
    
    console->vt_supported = SetConsoleMode(console->hStdOut, out_mode);
    
    // تعيين خط يدعم العربية
    CONSOLE_FONT_INFOEX cfi = {0};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    
    // محاولة الخطوط بالترتيب
    const wchar_t* fonts[] = {
        L"Cascadia Code",
        L"Cascadia Mono", 
        L"Consolas",
        L"Courier New"
    };
    
    for (int i = 0; i < 4; i++) {
        wcscpy_s(cfi.FaceName, LF_FACESIZE, fonts[i]);
        if (SetCurrentConsoleFontEx(console->hStdOut, FALSE, &cfi)) {
            wcscpy_s(console->font_name, LF_FACESIZE, fonts[i]);
            console->arabic_font_set = true;
            break;
        }
    }
    
    return true;
}

void qalam_console_cleanup(QalamConsole* console) {
    if (!console) return;
    
    // استعادة الإعدادات الأصلية
    SetConsoleMode(console->hStdOut, console->original_out_mode);
    SetConsoleMode(console->hStdIn, console->original_in_mode);
    SetConsoleOutputCP(console->original_cp_out);
    SetConsoleCP(console->original_cp_in);
}

void qalam_print(const wchar_t* text) {
    if (!text) return;
    
    DWORD written;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), 
                  text, (DWORD)wcslen(text), &written, NULL);
}

void qalam_print_colored(const wchar_t* text, int color) {
    const wchar_t* color_code;
    
    switch (color) {
        case 1: color_code = ANSI_RED; break;
        case 2: color_code = ANSI_GREEN; break;
        case 3: color_code = ANSI_YELLOW; break;
        case 4: color_code = ANSI_BLUE; break;
        case 5: color_code = ANSI_MAGENTA; break;
        case 6: color_code = ANSI_CYAN; break;
        default: color_code = ANSI_WHITE; break;
    }
    
    wprintf(L"%ls%ls%ls", color_code, text, ANSI_RESET);
}

void qalam_print_error(const wchar_t* text) {
    wprintf(L"%ls%ls✗ خطأ: %ls%ls\n", 
            ANSI_BOLD, ANSI_RED, text, ANSI_RESET);
}

void qalam_print_success(const wchar_t* text) {
    wprintf(L"%ls%ls✓ %ls%ls\n", 
            ANSI_BOLD, ANSI_GREEN, text, ANSI_RESET);
}

void qalam_print_warning(const wchar_t* text) {
    wprintf(L"%ls%ls⚠ تحذير: %ls%ls\n", 
            ANSI_BOLD, ANSI_YELLOW, text, ANSI_RESET);
}
```

### 1.2 الأوامر العربية

```c
// === commands.h ===

typedef enum {
    CMD_UNKNOWN,
    CMD_BUILD,      // بناء
    CMD_RUN,        // تشغيل
    CMD_CHECK,      // تدقيق
    CMD_HELP,       // مساعدة
    CMD_VERSION,    // إصدار
    CMD_CLEAN,      // تنظيف
    CMD_INIT,       // تهيئة
    CMD_NEW,        // جديد
} QalamCommand;

typedef struct {
    const wchar_t* arabic;
    const wchar_t* english;
    QalamCommand cmd;
    const wchar_t* description;
} CommandMapping;

static const CommandMapping COMMANDS[] = {
    {L"بناء",    L"build",   CMD_BUILD,   L"تجميع الملف المصدري"},
    {L"تشغيل",   L"run",     CMD_RUN,     L"تجميع وتشغيل البرنامج"},
    {L"تدقيق",   L"check",   CMD_CHECK,   L"فحص الأخطاء النحوية فقط"},
    {L"مساعدة",  L"help",    CMD_HELP,    L"عرض المساعدة"},
    {L"إصدار",   L"version", CMD_VERSION, L"عرض رقم الإصدار"},
    {L"تنظيف",   L"clean",   CMD_CLEAN,   L"حذف ملفات البناء"},
    {L"تهيئة",   L"init",    CMD_INIT,    L"تهيئة مشروع جديد"},
    {L"جديد",    L"new",     CMD_NEW,     L"إنشاء ملف جديد"},
    {NULL, NULL, CMD_UNKNOWN, NULL}
};

QalamCommand parse_command(const wchar_t* arg);
void show_help(void);
void show_version(void);
```

```c
// === commands.c ===

#include "commands.h"
#include "qalam_cli.h"

QalamCommand parse_command(const wchar_t* arg) {
    if (!arg) return CMD_UNKNOWN;
    
    for (int i = 0; COMMANDS[i].arabic != NULL; i++) {
        if (wcscmp(arg, COMMANDS[i].arabic) == 0 ||
            wcscmp(arg, COMMANDS[i].english) == 0) {
            return COMMANDS[i].cmd;
        }
    }
    
    return CMD_UNKNOWN;
}

void show_help(void) {
    qalam_print(L"\n");
    qalam_print_colored(L"    قلم - Qalam", 6);
    qalam_print(L" │ بيئة تطوير لغة باء\n\n");
    
    qalam_print(L"  ╭─────────────────────────────────────────╮\n");
    qalam_print(L"  │              الاستخدام                  │\n");
    qalam_print(L"  ╰─────────────────────────────────────────╯\n\n");
    
    qalam_print(L"    qalam <أمر> [خيارات] [ملف]\n\n");
    
    qalam_print(L"  ╭─────────────────────────────────────────╮\n");
    qalam_print(L"  │              الأوامر                    │\n");
    qalam_print(L"  ╰─────────────────────────────────────────╯\n\n");
    
    for (int i = 0; COMMANDS[i].arabic != NULL; i++) {
        wprintf(L"    %ls%-8ls%ls │ %-8ls │ %ls\n",
                ANSI_CYAN, COMMANDS[i].arabic, ANSI_RESET,
                COMMANDS[i].english, COMMANDS[i].description);
    }
    
    qalam_print(L"\n  ╭─────────────────────────────────────────╮\n");
    qalam_print(L"  │              أمثلة                      │\n");
    qalam_print(L"  ╰─────────────────────────────────────────╯\n\n");
    
    qalam_print(L"    qalam بناء program.baa\n");
    qalam_print(L"    qalam تشغيل program.baa\n");
    qalam_print(L"    qalam تهيئة مشروعي\n\n");
}

void show_version(void) {
    qalam_print(L"\n");
    qalam_print_colored(L"  قلم", 6);
    qalam_print(L" - Qalam IDE\n");
    qalam_print(L"  الإصدار: 0.1.0\n");
    qalam_print(L"  للغة باء الإصدار: 0.2.6\n\n");
}
```

### 1.3 مهام المرحلة ١

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| إعداد الطرفية | ضبط Windows Console لـ UTF-8 | ⬜ |
| كشف الخطوط | اكتشاف واقتراح خطوط عربية | ⬜ |
| محاذاة RTL | محاذاة النص من اليمين لليسار | ⬜ |
| ألوان ANSI | دعم الألوان (Windows 10+ VT100) | ⬜ |
| رسائل عربية | رسائل الخطأ والنجاح بالعربية | ⬜ |
| معالجة الإدخال | دعم لوحة المفاتيح العربية | ⬜ |
| تحليل الأوامر | تحليل الأوامر العربية والإنجليزية | ⬜ |

**المخرج:** `qalam-cli.exe`

---

## المرحلة ٢: ملوِّن الصياغة (`qalam-highlight`) 🎨

**الهدف:** أداة CLI لتلوين كود باء

**التقنيات:** C, ANSI Escape Codes, Lexer

### 2.1 خريطة الألوان

```c
// === highlight.h ===

#ifndef QALAM_HIGHLIGHT_H
#define QALAM_HIGHLIGHT_H

#include <stddef.h>
#include <stdbool.h>

// أنواع الرموز
typedef enum {
    TOKEN_KEYWORD,        // كلمات مفتاحية: صحيح، إذا، طالما
    TOKEN_TYPE,           // أنواع: صحيح، نص، حرف
    TOKEN_STRING,         // نصوص: "مرحباً"
    TOKEN_CHAR,           // حروف: 'أ'
    TOKEN_NUMBER,         // أرقام: ١٢٣، 123
    TOKEN_COMMENT,        // تعليقات: // ...
    TOKEN_OPERATOR,       // عوامل: +، -، *، /
    TOKEN_FUNCTION,       // دوال: الرئيسية، جمع
    TOKEN_IDENTIFIER,     // معرفات: س، متغير
    TOKEN_PUNCTUATION,    // علامات: {، }، (، )
    TOKEN_PREPROCESSOR,   // معالج: #تضمين
    TOKEN_ERROR,          // خطأ
    TOKEN_WHITESPACE,     // مسافات
    TOKEN_NEWLINE,        // سطر جديد
} TokenType;

// ألوان السمة
typedef struct {
    const wchar_t* keyword;       // أزرق
    const wchar_t* type;          // بنفسجي
    const wchar_t* string;        // أصفر
    const wchar_t* character;     // برتقالي
    const wchar_t* number;        // أخضر
    const wchar_t* comment;       // رمادي
    const wchar_t* operator_;     // أحمر
    const wchar_t* function;      // سماوي
    const wchar_t* identifier;    // أبيض
    const wchar_t* preprocessor;  // وردي
    const wchar_t* error;         // أحمر خلفية
    const wchar_t* reset;         // إعادة تعيين
} HighlightTheme;

// السمة الداكنة الافتراضية
static const HighlightTheme THEME_DARK = {
    .keyword      = L"\033[1;34m",      // أزرق غامق
    .type         = L"\033[35m",        // بنفسجي
    .string       = L"\033[33m",        // أصفر
    .character    = L"\033[38;5;208m",  // برتقالي
    .number       = L"\033[32m",        // أخضر
    .comment      = L"\033[90m",        // رمادي
    .operator_    = L"\033[31m",        // أحمر
    .function     = L"\033[36m",        // سماوي
    .identifier   = L"\033[37m",        // أبيض
    .preprocessor = L"\033[38;5;213m",  // وردي
    .error        = L"\033[41;37m",     // خلفية حمراء
    .reset        = L"\033[0m",
};

// السمة الفاتحة
static const HighlightTheme THEME_LIGHT = {
    .keyword      = L"\033[34m",        // أزرق
    .type         = L"\033[35m",        // بنفسجي
    .string       = L"\033[38;5;130m",  // بني
    .character    = L"\033[38;5;166m",  // برتقالي غامق
    .number       = L"\033[38;5;28m",   // أخضر غامق
    .comment      = L"\033[37m",        // رمادي فاتح
    .operator_    = L"\033[31m",        // أحمر
    .function     = L"\033[38;5;30m",   // سماوي غامق
    .identifier   = L"\033[30m",        // أسود
    .preprocessor = L"\033[38;5;127m",  // وردي غامق
    .error        = L"\033[41;37m",     // خلفية حمراء
    .reset        = L"\033[0m",
};

#endif
```

### 2.2 الكلمات المفتاحية

```c
// === keywords.c ===

// الكلمات المفتاحية في باء
static const wchar_t* KEYWORDS[] = {
    // التحكم
    L"إذا", L"وإلا", L"طالما", L"لكل", 
    L"توقف", L"استمر", L"إرجع",
    L"اختر", L"حالة", L"افتراضي",
    
    // الأنواع
    L"صحيح", L"نص", L"حرف",
    
    // المعالج القبلي
    L"تضمين",
    
    // القيم
    L"صواب", L"خطأ", L"عدم",
    
    NULL
};

// الدوال المدمجة
static const wchar_t* BUILTINS[] = {
    L"اطبع", L"الرئيسية",
    NULL
};

bool is_keyword(const wchar_t* word) {
    for (int i = 0; KEYWORDS[i] != NULL; i++) {
        if (wcscmp(word, KEYWORDS[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool is_builtin(const wchar_t* word) {
    for (int i = 0; BUILTINS[i] != NULL; i++) {
        if (wcscmp(word, BUILTINS[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool is_type(const wchar_t* word) {
    return wcscmp(word, L"صحيح") == 0 ||
           wcscmp(word, L"نص") == 0 ||
           wcscmp(word, L"حرف") == 0;
}
```

### 2.3 محرك التلوين

```c
// === highlighter.c ===

#include "highlight.h"
#include "keywords.c"

typedef struct {
    const wchar_t* source;
    size_t position;
    size_t length;
    const HighlightTheme* theme;
    bool show_line_numbers;
    int current_line;
} Highlighter;

void highlighter_init(Highlighter* h, const wchar_t* source, 
                      const HighlightTheme* theme) {
    h->source = source;
    h->position = 0;
    h->length = wcslen(source);
    h->theme = theme ? theme : &THEME_DARK;
    h->show_line_numbers = false;
    h->current_line = 1;
}

wchar_t peek(Highlighter* h) {
    if (h->position >= h->length) return L'\0';
    return h->source[h->position];
}

wchar_t advance(Highlighter* h) {
    return h->source[h->position++];
}

bool is_arabic(wchar_t c) {
    return (c >= 0x0600 && c <= 0x06FF) ||  // Arabic
           (c >= 0x0750 && c <= 0x077F) ||  // Arabic Supplement
           (c >= 0xFB50 && c <= 0xFDFF) ||  // Arabic Presentation Forms-A
           (c >= 0xFE70 && c <= 0xFEFF);    // Arabic Presentation Forms-B
}

bool is_arabic_digit(wchar_t c) {
    return c >= L'٠' && c <= L'٩';
}

bool is_identifier_start(wchar_t c) {
    return is_arabic(c) || (c >= L'a' && c <= L'z') || 
           (c >= L'A' && c <= L'Z') || c == L'_';
}

bool is_identifier_char(wchar_t c) {
    return is_identifier_start(c) || 
           (c >= L'0' && c <= L'9') || 
           is_arabic_digit(c);
}

void highlight_string(Highlighter* h) {
    wprintf(L"%ls\"", h->theme->string);
    advance(h);  // تخطي "
    
    while (peek(h) != L'\0' && peek(h) != L'"') {
        if (peek(h) == L'\\') {
            wprintf(L"%lc", advance(h));
        }
        wprintf(L"%lc", advance(h));
    }
    
    if (peek(h) == L'"') {
        wprintf(L"\"%ls", h->theme->reset);
        advance(h);
    }
}

void highlight_comment(Highlighter* h) {
    wprintf(L"%ls", h->theme->comment);
    
    while (peek(h) != L'\0' && peek(h) != L'\n') {
        wprintf(L"%lc", advance(h));
    }
    
    wprintf(L"%ls", h->theme->reset);
}

void highlight_number(Highlighter* h) {
    wprintf(L"%ls", h->theme->number);
    
    while (is_arabic_digit(peek(h)) || 
           (peek(h) >= L'0' && peek(h) <= L'9') ||
           peek(h) == L'.') {
        wprintf(L"%lc", advance(h));
    }
    
    wprintf(L"%ls", h->theme->reset);
}

void highlight_identifier(Highlighter* h) {
    wchar_t word[256];
    int i = 0;
    
    while (is_identifier_char(peek(h)) && i < 255) {
        word[i++] = advance(h);
    }
    word[i] = L'\0';
    
    const wchar_t* color;
    if (is_keyword(word)) {
        color = h->theme->keyword;
    } else if (is_type(word)) {
        color = h->theme->type;
    } else if (is_builtin(word)) {
        color = h->theme->function;
    } else {
        color = h->theme->identifier;
    }
    
    wprintf(L"%ls%ls%ls", color, word, h->theme->reset);
}

void highlight_file(const wchar_t* source, const HighlightTheme* theme,
                    bool line_numbers) {
    Highlighter h;
    highlighter_init(&h, source, theme);
    h.show_line_numbers = line_numbers;
    
    if (line_numbers) {
        wprintf(L"%ls%4d │%ls ", 
                L"\033[90m", h.current_line, L"\033[0m");
    }
    
    while (peek(&h) != L'\0') {
        wchar_t c = peek(&h);
        
        // تعليق
        if (c == L'/' && h.source[h.position + 1] == L'/') {
            highlight_comment(&h);
        }
        // نص
        else if (c == L'"') {
            highlight_string(&h);
        }
        // حرف
        else if (c == L'\'') {
            wprintf(L"%ls", h.theme->character);
            wprintf(L"%lc", advance(&h));
            if (peek(&h) != L'\0') wprintf(L"%lc", advance(&h));
            if (peek(&h) == L'\'') wprintf(L"%lc", advance(&h));
            wprintf(L"%ls", h.theme->reset);
        }
        // رقم
        else if (is_arabic_digit(c) || (c >= L'0' && c <= L'9')) {
            highlight_number(&h);
        }
        // معالج قبلي
        else if (c == L'#') {
            wprintf(L"%ls#", h.theme->preprocessor);
            advance(&h);
            while (is_identifier_char(peek(&h))) {
                wprintf(L"%lc", advance(&h));
            }
            wprintf(L"%ls", h.theme->reset);
        }
        // معرف
        else if (is_identifier_start(c)) {
            highlight_identifier(&h);
        }
        // سطر جديد
        else if (c == L'\n') {
            wprintf(L"\n");
            advance(&h);
            h.current_line++;
            if (line_numbers && peek(&h) != L'\0') {
                wprintf(L"%ls%4d │%ls ", 
                        L"\033[90m", h.current_line, L"\033[0m");
            }
        }
        // عامل
        else if (wcschr(L"+-*/%=<>!&|^~", c)) {
            wprintf(L"%ls%lc%ls", 
                    h.theme->operator_, advance(&h), h.theme->reset);
        }
        // علامات ترقيم
        else if (wcschr(L"{}[]();,.", c)) {
            wprintf(L"%lc", advance(&h));
        }
        // غير ذلك
        else {
            wprintf(L"%lc", advance(&h));
        }
    }
    
    wprintf(L"\n");
}
```

### 2.4 الاستخدام

```powershell
# تلوين أساسي
qalam-highlight program.baa

# مع أرقام الأسطر
qalam-highlight -n program.baa

# السمة الفاتحة
qalam-highlight --theme light program.baa

# تصدير HTML
qalam-highlight --html program.baa > output.html
```

### 2.5 مهام المرحلة ٢

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| تكامل Lexer | استخدام `src/lexer.c` | ⬜ |
| تصنيف الرموز | ربط الرموز بالألوان | ⬜ |
| إخراج ANSI | توليد الألوان للطرفية | ⬜ |
| إخراج HTML | تصدير HTML للتوثيق | ⬜ |
| أرقام الأسطر | عرض اختياري لأرقام الأسطر | ⬜ |
| دعم السمات | سمات فاتحة/داكنة | ⬜ |

**المخرج:** `qalam-highlight.exe`

---

## المرحلة ٣: نواة GUI (`qalam-core`) 🖼️

**الهدف:** نافذة Windows أساسية مع دعم RTL

**التقنيات:** C, Win32 API, DirectWrite/GDI+

### 3.1 هيكل النافذة الرئيسية

```c
// === window.h ===

#ifndef QALAM_WINDOW_H
#define QALAM_WINDOW_H

#include <windows.h>
#include <stdbool.h>

// أبعاد افتراضية
#define QALAM_DEFAULT_WIDTH   1280
#define QALAM_DEFAULT_HEIGHT  720
#define QALAM_MIN_WIDTH       800
#define QALAM_MIN_HEIGHT      600

// معرفات العناصر
#define ID_EDITOR       1001
#define ID_TERMINAL     1002
#define ID_EXPLORER     1003
#define ID_TOOLBAR      1004
#define ID_STATUSBAR    1005
#define ID_TABBAR       1006

// معرفات القوائم
#define IDM_FILE_NEW        2001
#define IDM_FILE_OPEN       2002
#define IDM_FILE_SAVE       2003
#define IDM_FILE_SAVEAS     2004
#define IDM_FILE_CLOSE      2005
#define IDM_FILE_EXIT       2006

#define IDM_EDIT_UNDO       2101
#define IDM_EDIT_REDO       2102
#define IDM_EDIT_CUT        2103
#define IDM_EDIT_COPY       2104
#define IDM_EDIT_PASTE      2105
#define IDM_EDIT_SELECTALL  2106
#define IDM_EDIT_FIND       2107
#define IDM_EDIT_REPLACE    2108

#define IDM_BUILD_BUILD     2201
#define IDM_BUILD_RUN       2202
#define IDM_BUILD_STOP      2203
#define IDM_BUILD_CLEAN     2204

#define IDM_VIEW_EXPLORER   2301
#define IDM_VIEW_TERMINAL   2302
#define IDM_VIEW_RTL        2303
#define IDM_VIEW_FULLSCREEN 2304

#define IDM_HELP_DOCS       2401
#define IDM_HELP_ABOUT      2402

// حالة الألواح
typedef struct {
    bool explorer_visible;
    bool terminal_visible;
    
    int explorer_width;      // عرض مستكشف الملفات
    int terminal_height;     // ارتفاع الطرفية
    
    bool is_dragging_explorer;
    bool is_dragging_terminal;
} PanelState;

// النافذة الرئيسية
typedef struct {
    HWND hwnd;               // مقبض النافذة
    HWND hwnd_editor;        // المحرر
    HWND hwnd_terminal;      // الطرفية
    HWND hwnd_explorer;      // المستكشف
    HWND hwnd_toolbar;       // شريط الأدوات
    HWND hwnd_statusbar;     // شريط الحالة
    HWND hwnd_tabbar;        // شريط التبويبات
    
    HMENU hmenu;             // القائمة
    HACCEL haccel;           // اختصارات لوحة المفاتيح
    
    PanelState panels;
    
    bool rtl_layout;         // تخطيط RTL
    bool is_maximized;
    bool is_fullscreen;
    
    HFONT hfont_ui;          // خط الواجهة
    HFONT hfont_editor;      // خط المحرر
    
    // الألوان
    COLORREF clr_background;
    COLORREF clr_foreground;
    COLORREF clr_accent;
} QalamWindow;

// الدوال
bool qalam_window_create(QalamWindow* win, HINSTANCE hInstance);
void qalam_window_destroy(QalamWindow* win);
void qalam_window_layout(QalamWindow* win);
void qalam_window_set_rtl(QalamWindow* win, bool rtl);

#endif
```

### 3.2 إنشاء النافذة

```c
// === window.c ===

#include "window.h"

static const wchar_t* WINDOW_CLASS = L"QalamMainWindow";
static const wchar_t* WINDOW_TITLE = L"قلم - Qalam IDE";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, 
                             WPARAM wParam, LPARAM lParam);

bool qalam_window_create(QalamWindow* win, HINSTANCE hInstance) {
    if (!win) return false;
    
    // تسجيل فئة النافذة
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    // حساب الموقع (وسط الشاشة)
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int x = (screen_w - QALAM_DEFAULT_WIDTH) / 2;
    int y = (screen_h - QALAM_DEFAULT_HEIGHT) / 2;
    
    // تحديد النمط حسب RTL
    DWORD exStyle = WS_EX_APPWINDOW;
    if (win->rtl_layout) {
        exStyle |= WS_EX_LAYOUTRTL | WS_EX_RTLREADING;
    }
    
    // إنشاء النافذة
    win->hwnd = CreateWindowExW(
        exStyle,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        x, y, QALAM_DEFAULT_WIDTH, QALAM_DEFAULT_HEIGHT,
        NULL, NULL, hInstance, win
    );
    
    if (!win->hwnd) {
        return false;
    }
    
    // تخزين المؤشر
    SetWindowLongPtr(win->hwnd, GWLP_USERDATA, (LONG_PTR)win);
    
    // إنشاء القائمة
    win->hmenu = qalam_create_menu(win->rtl_layout);
    SetMenu(win->hwnd, win->hmenu);
    
    // إنشاء شريط الأدوات
    qalam_create_toolbar(win);
    
    // إنشاء شريط الحالة
    qalam_create_statusbar(win);
    
    // إنشاء الألواح
    qalam_create_panels(win);
    
    // ترتيب التخطيط
    qalam_window_layout(win);
    
    // تحميل الاختصارات
    win->haccel = qalam_create_accelerators();
    
    // إظهار النافذة
    ShowWindow(win->hwnd, SW_SHOW);
    UpdateWindow(win->hwnd);
    
    return true;
}
```

### 3.3 القوائم العربية

```c
// === menu.c ===

HMENU qalam_create_menu(bool rtl) {
    HMENU hMenu = CreateMenu();
    
    // === قائمة ملف ===
    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, IDM_FILE_NEW,    L"جديد\tCtrl+N");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_OPEN,   L"فتح...\tCtrl+O");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_SAVE,   L"حفظ\tCtrl+S");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_SAVEAS, L"حفظ باسم...\tCtrl+Shift+S");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_CLOSE,  L"إغلاق\tCtrl+W");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_EXIT,   L"خروج\tAlt+F4");
    
    // === قائمة تحرير ===
    HMENU hEdit = CreatePopupMenu();
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_UNDO,      L"تراجع\tCtrl+Z");
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_REDO,      L"إعادة\tCtrl+Y");
    AppendMenuW(hEdit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_CUT,       L"قص\tCtrl+X");
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_COPY,      L"نسخ\tCtrl+C");
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_PASTE,     L"لصق\tCtrl+V");
    AppendMenuW(hEdit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_SELECTALL, L"تحديد الكل\tCtrl+A");
    AppendMenuW(hEdit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_FIND,      L"بحث...\tCtrl+F");
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_REPLACE,   L"استبدال...\tCtrl+H");
    
    // === قائمة عرض ===
    HMENU hView = CreatePopupMenu();
    AppendMenuW(hView, MF_STRING | MF_CHECKED, IDM_VIEW_EXPLORER, 
                L"مستكشف الملفات\tCtrl+B");
    AppendMenuW(hView, MF_STRING | MF_CHECKED, IDM_VIEW_TERMINAL, 
                L"الطرفية\tCtrl+`");
    AppendMenuW(hView, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hView, MF_STRING, IDM_VIEW_RTL, 
                L"تبديل اتجاه الواجهة");
    AppendMenuW(hView, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hView, MF_STRING, IDM_VIEW_FULLSCREEN, 
                L"ملء الشاشة\tF11");
    
    // === قائمة بناء ===
    HMENU hBuild = CreatePopupMenu();
    AppendMenuW(hBuild, MF_STRING, IDM_BUILD_BUILD, L"بناء\tF7");
    AppendMenuW(hBuild, MF_STRING, IDM_BUILD_RUN,   L"تشغيل\tF5");
    AppendMenuW(hBuild, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hBuild, MF_STRING, IDM_BUILD_STOP,  L"إيقاف\tShift+F5");
    AppendMenuW(hBuild, MF_STRING, IDM_BUILD_CLEAN, L"تنظيف");
    
    // === قائمة مساعدة ===
    HMENU hHelp = CreatePopupMenu();
    AppendMenuW(hHelp, MF_STRING, IDM_HELP_DOCS,  L"التوثيق\tF1");
    AppendMenuW(hHelp, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hHelp, MF_STRING, IDM_HELP_ABOUT, L"حول قلم...");
    
    // ترتيب القوائم حسب RTL
    if (rtl) {
        // من اليمين لليسار
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelp,  L"مساعدة");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hBuild, L"بناء");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hView,  L"عرض");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hEdit,  L"تحرير");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFile,  L"ملف");
    } else {
        // من اليسار لليمين
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFile,  L"ملف");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hEdit,  L"تحرير");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hView,  L"عرض");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hBuild, L"بناء");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelp,  L"مساعدة");
    }
    
    return hMenu;
}
```

### 3.4 شريط الحالة

```c
// === statusbar.c ===

void qalam_create_statusbar(QalamWindow* win) {
    win->hwnd_statusbar = CreateWindowExW(
        0,
        STATUSCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        win->hwnd,
        (HMENU)ID_STATUSBAR,
        GetModuleHandle(NULL),
        NULL
    );
    
    // تقسيم شريط الحالة
    int parts[] = {150, 250, 350, 450, -1};
    SendMessage(win->hwnd_statusbar, SB_SETPARTS, 5, (LPARAM)parts);
    
    // تعيين النص الافتراضي
    qalam_statusbar_update(win, 1, 1, L"UTF-8", L"باء", true);
}

void qalam_statusbar_update(QalamWindow* win, 
                            int line, int col,
                            const wchar_t* encoding,
                            const wchar_t* language,
                            bool rtl_mode) {
    wchar_t buf[64];
    
    // الموقع
    swprintf(buf, 64, L"سطر: %d، عمود: %d", line, col);
    SendMessageW(win->hwnd_statusbar, SB_SETTEXTW, 0, (LPARAM)buf);
    
    // الترميز
    SendMessageW(win->hwnd_statusbar, SB_SETTEXTW, 1, (LPARAM)encoding);
    
    // اللغة
    SendMessageW(win->hwnd_statusbar, SB_SETTEXTW, 2, (LPARAM)language);
    
    // RTL
    SendMessageW(win->hwnd_statusbar, SB_SETTEXTW, 3, 
                 (LPARAM)(rtl_mode ? L"RTL ←" : L"→ LTR"));
    
    // الاسم
    SendMessageW(win->hwnd_statusbar, SB_SETTEXTW, 4, (LPARAM)L"قلم");
}
```

### 3.5 اختصارات لوحة المفاتيح

```c
// === accelerators.c ===

HACCEL qalam_create_accelerators(void) {
    ACCEL accels[] = {
        // ملف
        {FCONTROL | FVIRTKEY, 'N', IDM_FILE_NEW},
        {FCONTROL | FVIRTKEY, 'O', IDM_FILE_OPEN},
        {FCONTROL | FVIRTKEY, 'S', IDM_FILE_SAVE},
        {FCONTROL | FSHIFT | FVIRTKEY, 'S', IDM_FILE_SAVEAS},
        {FCONTROL | FVIRTKEY, 'W', IDM_FILE_CLOSE},
        
        // تحرير
        {FCONTROL | FVIRTKEY, 'Z', IDM_EDIT_UNDO},
        {FCONTROL | FVIRTKEY, 'Y', IDM_EDIT_REDO},
        {FCONTROL | FVIRTKEY, 'X', IDM_EDIT_CUT},
        {FCONTROL | FVIRTKEY, 'C', IDM_EDIT_COPY},
        {FCONTROL | FVIRTKEY, 'V', IDM_EDIT_PASTE},
        {FCONTROL | FVIRTKEY, 'A', IDM_EDIT_SELECTALL},
        {FCONTROL | FVIRTKEY, 'F', IDM_EDIT_FIND},
        {FCONTROL | FVIRTKEY, 'H', IDM_EDIT_REPLACE},
        
        // بناء
        {FVIRTKEY, VK_F7, IDM_BUILD_BUILD},
        {FVIRTKEY, VK_F5, IDM_BUILD_RUN},
        {FSHIFT | FVIRTKEY, VK_F5, IDM_BUILD_STOP},
        
        // عرض
        {FCONTROL | FVIRTKEY, 'B', IDM_VIEW_EXPLORER},
        {FCONTROL | FVIRTKEY, VK_OEM_3, IDM_VIEW_TERMINAL}, // `
        {FVIRTKEY, VK_F11, IDM_VIEW_FULLSCREEN},
        
        // مساعدة
        {FVIRTKEY, VK_F1, IDM_HELP_DOCS},
    };
    
    return CreateAcceleratorTable(accels, 
                                  sizeof(accels) / sizeof(ACCEL));
}
```

### 3.6 مهام المرحلة ٣

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| إنشاء النافذة | WinMain, RegisterClass, CreateWindow | ⬜ |
| شريط القوائم | قوائم عربية (ملف، تحرير، عرض، بناء، مساعدة) | ⬜ |
| شريط الأدوات | أزرار بناء، تشغيل، إيقاف، حفظ | ⬜ |
| شريط الحالة | سطر/عمود، ترميز، حالة الملف | ⬜ |
| الألواح المقسمة | محرر/طرفية/مستكشف قابلة للتحجيم | ⬜ |
| تخطيط RTL | خيار تخطيط من اليمين لليسار | ⬜ |
| تحميل الخطوط | DirectWrite مع دعم العربية | ⬜ |
| الاختصارات | تسجيل اختصارات لوحة المفاتيح | ⬜ |

**المخرج:** تطبيق نافذة أساسي مع واجهة عربية

---

## 📊 ملخص المراحل 1-3

| المرحلة | المكون | الحالة | المخرج |
|---------|--------|--------|--------|
| ١ | CLI Foundation | ⬜ لم يبدأ | `qalam-cli.exe` |
| ٢ | Syntax Highlighter | ⬜ لم يبدأ | `qalam-highlight.exe` |
| ٣ | GUI Core | ⬜ لم يبدأ | نافذة أساسية |

---

**التالي:** [QALAM_ROADMAP_2.md](QALAM_ROADMAP_2.md) - المراحل 4-6 (المحرر، الطرفية، IDE)