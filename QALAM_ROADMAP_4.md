# QALAM_ROADMAP_4.md - المراحل 10-12 والمواصفات التقنية

```markdown
# قلم - Qalam IDE
## المراحل 10-12: مدير الحزم، الإضافات، الاختبارات، المواصفات

**السابق:** [QALAM_ROADMAP_3.md](QALAM_ROADMAP_3.md) - المراحل 7-9

---

## المرحلة ١٠: مدير الحزم (`baa-pkg`) 📦

**الهدف:** نظام إدارة التبعيات والحزم

**التقنيات:** C, JSON, HTTP/HTTPS, ZIP

### 10.1 هيكل مدير الحزم

```c
// === package.h ===

#ifndef QALAM_PACKAGE_H
#define QALAM_PACKAGE_H

#include <windows.h>
#include <stdbool.h>

#define PKG_NAME_MAX        64
#define PKG_VERSION_MAX     32
#define PKG_MAX_DEPS        64

// إصدار دلالي (Semantic Version)
typedef struct {
    int major;
    int minor;
    int patch;
    wchar_t prerelease[32];
} SemVer;

// تبعية
typedef struct {
    wchar_t name[PKG_NAME_MAX];
    wchar_t version_constraint[PKG_VERSION_MAX];  // ^1.0.0, ~2.1, >=3.0
} Dependency;

// بيان الحزمة (Manifest)
typedef struct {
    wchar_t name[PKG_NAME_MAX];
    SemVer version;
    wchar_t description[256];
    wchar_t author[64];
    wchar_t license[32];
    wchar_t repository[256];
    wchar_t homepage[256];
    
    wchar_t main_file[MAX_PATH];
    wchar_t* keywords[16];
    size_t keyword_count;
    
    Dependency dependencies[PKG_MAX_DEPS];
    size_t dependency_count;
    
    Dependency dev_dependencies[PKG_MAX_DEPS];
    size_t dev_dependency_count;
    
    // سكربتات
    struct {
        wchar_t name[32];
        wchar_t command[256];
    } scripts[16];
    size_t script_count;
    
} PackageManifest;

// حزمة مُثبتة
typedef struct {
    wchar_t name[PKG_NAME_MAX];
    SemVer version;
    wchar_t install_path[MAX_PATH];
    bool is_dev;
} InstalledPackage;

// مدير الحزم
typedef struct {
    wchar_t project_dir[MAX_PATH];
    wchar_t cache_dir[MAX_PATH];
    wchar_t registry_url[256];
    
    PackageManifest manifest;
    
    InstalledPackage* installed;
    size_t installed_count;
    
    // Callbacks
    void (*on_progress)(const wchar_t* msg, int percent, void* user_data);
    void (*on_error)(const wchar_t* error, void* user_data);
    void* user_data;
    
} PackageManager;

// === الدوال ===
PackageManager* pkg_create(void);
void pkg_destroy(PackageManager* pm);

bool pkg_init(PackageManager* pm, const wchar_t* project_dir);
bool pkg_load_manifest(PackageManager* pm);
bool pkg_save_manifest(PackageManager* pm);

bool pkg_install(PackageManager* pm, const wchar_t* name, const wchar_t* version);
bool pkg_uninstall(PackageManager* pm, const wchar_t* name);
bool pkg_update(PackageManager* pm, const wchar_t* name);
bool pkg_update_all(PackageManager* pm);

bool pkg_install_all(PackageManager* pm);  // تثبيت جميع التبعيات
bool pkg_publish(PackageManager* pm);

PackageManifest* pkg_search(PackageManager* pm, const wchar_t* query, size_t* count);
PackageManifest* pkg_info(PackageManager* pm, const wchar_t* name);

#endif
```

### 10.2 ملف البيان (`baa.json`)

```json
{
    "الاسم": "تطبيقي",
    "الإصدار": "1.0.0",
    "الوصف": "تطبيق باء رائع",
    "المؤلف": "أحمد محمد <ahmed@example.com>",
    "الترخيص": "MIT",
    "الرئيسي": "src/main.baa",
    
    "كلمات_مفتاحية": ["أداة", "سطر_أوامر"],
    
    "تبعيات": {
        "مكتبة-الرياضيات": "^2.0.0",
        "مكتبة-نصوص": "~1.5.0"
    },
    
    "تبعيات-تطوير": {
        "أداة-اختبار": "^1.0.0"
    },
    
    "سكربتات": {
        "بناء": "baa بناء src/main.baa -o bin/app.exe",
        "تشغيل": "bin/app.exe",
        "اختبار": "baa تشغيل tests/test.baa",
        "تنظيف": "rmdir /s /q bin"
    },
    
    "مستودع": {
        "نوع": "git",
        "رابط": "https://github.com/user/project"
    }
}
```

### 10.3 الأوامر

```powershell
# تهيئة مشروع جديد
baa-pkg تهيئة
baa-pkg init

# تثبيت حزمة
baa-pkg تثبيت مكتبة-الرياضيات
baa-pkg install math-lib

# تثبيت بإصدار محدد
baa-pkg تثبيت مكتبة-الرياضيات@2.0.0

# تثبيت جميع التبعيات
baa-pkg تثبيت
baa-pkg install

# إزالة حزمة
baa-pkg إزالة مكتبة-الرياضيات
baa-pkg remove math-lib

# تحديث حزمة
baa-pkg تحديث مكتبة-الرياضيات
baa-pkg update math-lib

# تحديث الكل
baa-pkg تحديث
baa-pkg update

# البحث
baa-pkg بحث رياضيات
baa-pkg search math

# معلومات حزمة
baa-pkg معلومات مكتبة-الرياضيات
baa-pkg info math-lib

# نشر حزمة
baa-pkg نشر
baa-pkg publish

# تشغيل سكربت
baa-pkg تشغيل بناء
baa-pkg run build

# قائمة الحزم المثبتة
baa-pkg قائمة
baa-pkg list
```

### 10.4 مهام المرحلة ١٠

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| تحليل البيان | قراءة `baa.json` | ⬜ |
| حل التبعيات | Dependency Resolution | ⬜ |
| تحميل الحزم | HTTP download | ⬜ |
| التخزين المؤقت | Local cache | ⬜ |
| فك الضغط | ZIP extraction | ⬜ |
| سجل الحزم | Package registry | ⬜ |
| النشر | Package publishing | ⬜ |
| السكربتات | تشغيل السكربتات | ⬜ |

---

## المرحلة ١١: نظام الإضافات 🔌

**الهدف:** قابلية التوسع عبر الإضافات

**التقنيات:** DLL Loading, Plugin API

### 11.1 واجهة الإضافات

```c
// === plugin_api.h ===

#ifndef QALAM_PLUGIN_API_H
#define QALAM_PLUGIN_API_H

#include <windows.h>

#define QALAM_PLUGIN_API_VERSION 1

// معلومات الإضافة
typedef struct {
    int api_version;
    wchar_t name[64];
    wchar_t version[16];
    wchar_t author[64];
    wchar_t description[256];
} PluginInfo;

// واجهة قلم للإضافات
typedef struct QalamAPI {
    // === المحرر ===
    void (*editor_insert_text)(const wchar_t* text);
    void (*editor_delete_selection)(void);
    wchar_t* (*editor_get_selection)(void);
    void (*editor_set_cursor)(size_t line, size_t col);
    void (*editor_get_cursor)(size_t* line, size_t* col);
    
    // === الملفات ===
    bool (*file_open)(const wchar_t* path);
    bool (*file_save)(void);
    const wchar_t* (*file_get_path)(void);
    
    // === الواجهة ===
    void (*ui_show_message)(const wchar_t* title, const wchar_t* msg);
    int (*ui_show_confirm)(const wchar_t* title, const wchar_t* msg);
    wchar_t* (*ui_show_input)(const wchar_t* title, const wchar_t* prompt);
    wchar_t* (*ui_show_file_dialog)(bool open, const wchar_t* filter);
    
    // === الأوامر ===
    void (*register_command)(const wchar_t* id, const wchar_t* name,
                            void (*handler)(void* user_data), void* user_data);
    void (*register_keybinding)(const wchar_t* command_id, const wchar_t* shortcut);
    
    // === القوائم ===
    void (*menu_add_item)(const wchar_t* menu, const wchar_t* id,
                         const wchar_t* label, void (*handler)(void*), void* user_data);
    void (*menu_add_separator)(const wchar_t* menu);
    
    // === الطرفية ===
    void (*terminal_write)(const wchar_t* text);
    void (*terminal_execute)(const wchar_t* command);
    void (*terminal_clear)(void);
    
    // === الإخراج ===
    void (*output_write)(const wchar_t* channel, const wchar_t* text);
    void (*output_clear)(const wchar_t* channel);
    
    // === الإعدادات ===
    wchar_t* (*settings_get)(const wchar_t* key);
    void (*settings_set)(const wchar_t* key, const wchar_t* value);
    
} QalamAPI;

// دوال الإضافة (يجب تصديرها)
typedef PluginInfo* (*PluginGetInfoFunc)(void);
typedef bool (*PluginLoadFunc)(QalamAPI* api);
typedef void (*PluginUnloadFunc)(void);

// === Macros للتصدير ===
#ifdef QALAM_PLUGIN_EXPORTS
    #define QALAM_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define QALAM_PLUGIN_EXPORT __declspec(dllimport)
#endif

#endif
```

### 11.2 مثال إضافة

```c
// === my_plugin.c ===

#include "plugin_api.h"

static QalamAPI* g_api = NULL;
static PluginInfo g_info = {
    .api_version = QALAM_PLUGIN_API_VERSION,
    .name = L"إضافتي",
    .version = L"1.0.0",
    .author = L"أحمد",
    .description = L"إضافة تجريبية لقلم"
};

QALAM_PLUGIN_EXPORT PluginInfo* qalam_plugin_info(void) {
    return &g_info;
}

static void on_hello_command(void* user_data) {
    g_api->ui_show_message(L"مرحباً", L"أهلاً من إضافتي!");
}

static void on_insert_date(void* user_data) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    wchar_t date[64];
    swprintf(date, 64, L"%04d/%02d/%02d", st.wYear, st.wMonth, st.wDay);
    
    g_api->editor_insert_text(date);
}

QALAM_PLUGIN_EXPORT bool qalam_plugin_load(QalamAPI* api) {
    g_api = api;
    
    // تسجيل الأوامر
    api->register_command(L"myPlugin.hello", L"مرحباً من إضافتي",
                         on_hello_command, NULL);
    api->register_command(L"myPlugin.insertDate", L"إدراج التاريخ",
                         on_insert_date, NULL);
    
    // إضافة للقائمة
    api->menu_add_item(L"أدوات", L"myPlugin.hello", L"مرحباً", 
                      on_hello_command, NULL);
    api->menu_add_item(L"تحرير", L"myPlugin.insertDate", L"إدراج التاريخ",
                      on_insert_date, NULL);
    
    // اختصار
    api->register_keybinding(L"myPlugin.insertDate", L"Ctrl+Shift+D");
    
    return true;
}

QALAM_PLUGIN_EXPORT void qalam_plugin_unload(void) {
    g_api = NULL;
}
```

### 11.3 مدير الإضافات

```c
// === plugin_manager.h ===

typedef struct {
    PluginInfo info;
    HMODULE handle;
    wchar_t path[MAX_PATH];
    bool is_loaded;
    bool is_enabled;
    
    PluginLoadFunc load_func;
    PluginUnloadFunc unload_func;
} LoadedPlugin;

typedef struct {
    LoadedPlugin* plugins;
    size_t count;
    size_t capacity;
    
    wchar_t plugins_dir[MAX_PATH];
    QalamAPI api;
} PluginManager;

// === الدوال ===
PluginManager* plugin_manager_create(void);
void plugin_manager_destroy(PluginManager* pm);

void plugin_manager_scan(PluginManager* pm);
bool plugin_manager_load(PluginManager* pm, const wchar_t* name);
void plugin_manager_unload(PluginManager* pm, const wchar_t* name);
void plugin_manager_load_all(PluginManager* pm);
void plugin_manager_unload_all(PluginManager* pm);
```

### 11.4 مهام المرحلة ١١

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| Plugin API | تصميم الواجهة | ⬜ |
| تحميل DLL | LoadLibrary, GetProcAddress | ⬜ |
| مسح الإضافات | اكتشاف الإضافات | ⬜ |
| تسجيل الأوامر | Command registration | ⬜ |
| إعدادات الإضافات | Plugin settings | ⬜ |
| واجهة الإدارة | UI لإدارة الإضافات | ⬜ |

---

## المرحلة ١٢: إطار الاختبارات 🧪

**الهدف:** دعم كتابة وتشغيل الاختبارات

### 12.1 هيكل الاختبارات

```c
// === testing.h ===

typedef enum {
    TEST_PENDING,
    TEST_RUNNING,
    TEST_PASSED,
    TEST_FAILED,
    TEST_SKIPPED,
} TestStatus;

typedef struct {
    wchar_t name[128];
    wchar_t file[MAX_PATH];
    size_t line;
    TestStatus status;
    wchar_t error_message[512];
    wchar_t expected[256];
    wchar_t actual[256];
    double duration_ms;
} TestResult;

typedef struct {
    wchar_t name[64];
    TestResult* tests;
    size_t test_count;
    size_t passed;
    size_t failed;
    size_t skipped;
    double total_duration_ms;
} TestSuite;

typedef struct {
    TestSuite* suites;
    size_t suite_count;
    
    size_t total_tests;
    size_t total_passed;
    size_t total_failed;
    size_t total_skipped;
    
    bool is_running;
    
    void (*on_test_start)(TestResult* test, void* user_data);
    void (*on_test_complete)(TestResult* test, void* user_data);
    void (*on_suite_complete)(TestSuite* suite, void* user_data);
    void (*on_all_complete)(void* user_data);
    void* user_data;
} TestRunner;

// === الدوال ===
TestRunner* test_runner_create(void);
void test_runner_destroy(TestRunner* runner);
void test_runner_discover(TestRunner* runner, const wchar_t* dir);
void test_runner_run_all(TestRunner* runner);
void test_runner_run_suite(TestRunner* runner, const wchar_t* suite_name);
void test_runner_run_single(TestRunner* runner, const wchar_t* file, size_t line);
void test_runner_stop(TestRunner* runner);
```

### 12.2 واجهة الاختبارات

```
┌─────────────────────────────────────────────────────────────────┐
│  الاختبارات                                          [⏵] [⟳]  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ✓ اختبارات_الرياضيات (٥/٥)                    ١٢٣ مللي ثانية  │
│    ├─ ✓ اختبار_الجمع                                    ٢ مث   │
│    ├─ ✓ اختبار_الطرح                                    ١ مث   │
│    ├─ ✓ اختبار_الضرب                                    ٣ مث   │
│    ├─ ✓ اختبار_القسمة                                   ٢ مث   │
│    └─ ✓ اختبار_المضروب                                ١١٥ مث   │
│                                                                 │
│  ✗ اختبارات_النصوص (٢/٣)                        ٤٥ مللي ثانية  │
│    ├─ ✓ اختبار_الطول                                    ٥ مث   │
│    ├─ ✗ اختبار_القص                                    ٣٨ مث   │
│    │     خطأ: المتوقع "مرحبا" لكن الناتج "مرحباً"              │
│    └─ ✓ اختبار_الدمج                                    ٢ مث   │
│                                                                 │
│  ○ اختبارات_الملفات (لم يُنفَّذ)                                │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│  الإجمالي: ٧ نجح، ١ فشل، ٣ لم يُنفَّذ     المدة: ١٦٨ مللي ثانية │
└─────────────────────────────────────────────────────────────────┘
```

### 12.3 مهام المرحلة ١٢

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| اكتشاف الاختبارات | Test discovery | ⬜ |
| تشغيل الاختبارات | Test execution | ⬜ |
| عرض النتائج | Results UI | ⬜ |
| تقرير الأخطاء | Error reporting | ⬜ |
| Code Coverage | تغطية الكود | ⬜ |

---

## 📐 المواصفات التقنية

### هيكل المشروع

```
qalam/
├── src/
│   ├── core/
│   │   ├── main.c              # نقطة البداية
│   │   ├── app.c               # إدارة التطبيق
│   │   ├── settings.c          # الإعدادات
│   │   ├── session.c           # الجلسات
│   │   └── project.c           # المشاريع
│   │
│   ├── editor/
│   │   ├── buffer.c            # Gap Buffer
│   │   ├── cursor.c            # المؤشر
│   │   ├── selection.c         # التحديد
│   │   ├── undo.c              # التراجع
│   │   ├── folding.c           # الطي
│   │   ├── minimap.c           # Minimap
│   │   └── editor.c            # المحرر الرئيسي
│   │
│   ├── ui/
│   │   ├── window.c            # النافذة
│   │   ├── menu.c              # القوائم
│   │   ├── toolbar.c           # الأدوات
│   │   ├── statusbar.c         # الحالة
│   │   ├── tabs.c              # التبويبات
│   │   ├── explorer.c          # المستكشف
│   │   ├── dialogs.c           # الحوارات
│   │   └── theme.c             # السمات
│   │
│   ├── terminal/
│   │   ├── terminal.c          # الطرفية
│   │   ├── pty.c               # ConPTY
│   │   ├── ansi.c              # ANSI Parser
│   │   └── shell.c             # Shell
│   │
│   ├── language/
│   │   ├── lexer.c             # Lexer
│   │   ├── parser.c            # Parser
│   │   ├── symbols.c           # Symbols
│   │   ├── completion.c        # Completion
│   │   ├── diagnostics.c       # Diagnostics
│   │   └── snippets.c          # Snippets
│   │
│   ├── build/
│   │   ├── compiler.c          # المترجم
│   │   ├── runner.c            # التشغيل
│   │   └── errors.c            # الأخطاء
│   │
│   ├── debug/
│   │   ├── debugger.c          # المنقح
│   │   ├── breakpoints.c       # نقاط التوقف
│   │   └── variables.c         # المتغيرات
│   │
│   ├── text/
│   │   ├── unicode.c           # Unicode
│   │   ├── bidi.c              # Bidi
│   │   ├── shaping.c           # Arabic Shaping
│   │   └── font.c              # Fonts
│   │
│   ├── search/
│   │   ├── search.c            # البحث
│   │   ├── replace.c           # الاستبدال
│   │   └── regex.c             # Regex
│   │
│   ├── plugins/
│   │   ├── plugin_api.c        # API
│   │   └── plugin_manager.c    # Manager
│   │
│   └── utils/
│       ├── file.c              # الملفات
│       ├── string.c            # النصوص
│       ├── json.c              # JSON
│       ├── path.c              # المسارات
│       └── memory.c            # الذاكرة
│
├── include/qalam/
│   ├── qalam.h
│   ├── editor.h
│   ├── terminal.h
│   ├── ui.h
│   └── ...
│
├── resources/
│   ├── icons/
│   ├── themes/
│   │   ├── dark.json
│   │   └── light.json
│   ├── templates/
│   ├── fonts/
│   └── locales/
│       └── ar.json
│
├── tools/
│   ├── baa-pkg/               # مدير الحزم
│   └── baa-highlight/         # الملون
│
├── tests/
│
├── docs/
│
├── CMakeLists.txt
└── README.md
```

### الخطوط الموصى بها

| الخط | الاستخدام | دعم العربية |
|------|----------|-------------|
| Cascadia Code Arabic | المحرر | ✅ ممتاز |
| Cascadia Mono Arabic | الطرفية | ✅ ممتاز |
| Noto Sans Arabic | الواجهة | ✅ ممتاز |
| Amiri | التوثيق | ✅ تقليدي |
| Cairo | واجهة حديثة | ✅ جيد |

### الترميزات

| الصيغة | الترميز | BOM |
|--------|---------|-----|
| ملفات المصدر `.baa` | UTF-8 | اختياري |
| ملفات المشروع | UTF-8 | لا |
| الإعدادات | UTF-8 JSON | لا |
| إخراج الطرفية | UTF-8 | لا |

### متطلبات النظام

| المتطلب | الحد الأدنى | الموصى به |
|---------|-------------|-----------|
| نظام التشغيل | Windows 10 1903+ | Windows 11 |
| المعالج | 1 GHz | 2+ GHz |
| الذاكرة | 256 MB | 512+ MB |
| القرص | 100 MB | 500 MB |
| الشاشة | 1024×768 | 1920×1080 |

### متطلبات البناء

| التبعية | الإصدار | الغرض |
|---------|---------|-------|
| MSVC / MinGW-w64 | Latest | مترجم C |
| Windows SDK | 10.0.19041+ | Win32, DirectWrite, ConPTY |
| CMake | 3.16+ | نظام البناء |

---

## 🔤 ملحق: مواصفات دعم العربية

### نطاقات Unicode

| النطاق | الاسم | الاستخدام |
|--------|-------|----------|
| U+0600-U+06FF | Arabic | الكتلة الرئيسية |
| U+0750-U+077F | Arabic Supplement | حروف إضافية |
| U+08A0-U+08FF | Arabic Extended-A | امتداد |
| U+0660-U+0669 | Arabic-Indic Digits | ٠١٢٣٤٥٦٧٨٩ |
| U+FB50-U+FDFF | Arabic Presentation Forms-A | أشكال |
| U+FE70-U+FEFF | Arabic Presentation Forms-B | Ligatures |

### إعدادات RTL

```c
typedef struct {
    // === المحرر ===
    bool editor_rtl_default;          // RTL افتراضي
    bool line_numbers_on_right;       // أرقام على اليمين
    bool gutter_on_right;             // الهامش على اليمين
    bool cursor_visual_movement;      // حركة بصرية
    
    // === الواجهة ===
    bool ui_rtl_layout;               // تخطيط RTL
    bool menus_rtl;                   // قوائم RTL
    bool tabs_rtl;                    // تبويبات RTL
    bool scrollbar_on_left;           // التمرير يساراً
    
    // === الطرفية ===
    bool terminal_rtl_input;          // إدخال RTL
    bool terminal_auto_detect;        // كشف تلقائي
    
} RTLSettings;
```

### تخطيط RTL

```
┌─────────────────────────────────────────────────────────────┐
│                     بناء  عرض  تحرير  ملف                   │
├───────────────────────────────────────────────────────┬─────┤
│                                                       │     │
│     صحيح الرئيسية() {             │ ١                 │ src │
│         اطبع "مرحباً بالعالم".    │ ٢                 │ ├─a │
│         إرجع ٠.                   │ ٣                 │ └─b │
│     }                             │ ٤                 │     │
│                                                       │     │
├───────────────────────────────────────────────────────┴─────┤
│                        الطرفية                              │
│  ✓ تم البناء بنجاح                                         │
├─────────────────────────────────────────────────────────────┤
│  UTF-8 │ عمود: ١٢ │ سطر: ٣ │ RTL                      قلم   │
└─────────────────────────────────────────────────────────────┘
```

---

## 📊 متتبع التقدم

| المرحلة | المكون | الحالة | الأهمية |
|---------|--------|--------|---------|
| ١ | CLI Foundation | ⬜ لم يبدأ | 🔴 حرجة |
| ٢ | Syntax Highlighter | ⬜ لم يبدأ | 🟡 عالية |
| ٣ | GUI Core | ⬜ لم يبدأ | 🔴 حرجة |
| ٤ | Text Engine | ⬜ لم يبدأ | 🔴 حرجة |
| ٥ | Integrated Terminal | ⬜ لم يبدأ | 🔴 حرجة |
| ٦ | IDE Features | ⬜ لم يبدأ | 🟡 عالية |
| ٧ | Compiler Integration | ⬜ لم يبدأ | 🔴 حرجة |
| ٨ | Debugger | ⬜ لم يبدأ | 🟡 عالية |
| ٩ | Advanced Features | ⬜ لم يبدأ | 🟡 عالية |
| ١٠ | Package Manager | ⬜ لم يبدأ | 🟢 متوسطة |
| ١١ | Plugin System | ⬜ لم يبدأ | 🟢 متوسطة |
| ١٢ | Testing Framework | ⬜ لم يبدأ | 🟢 متوسطة |

**الرموز:**
- ⬜ لم يبدأ
- 🔄 قيد التنفيذ
- ✅ مكتمل
- ⏸️ متوقف

---

## 📚 المراجع

- [Unicode Bidirectional Algorithm (UAX #9)](https://unicode.org/reports/tr9/)
- [Win32 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/)
- [DirectWrite Documentation](https://docs.microsoft.com/en-us/windows/win32/directwrite/)
- [ConPTY Documentation](https://docs.microsoft.com/en-us/windows/console/creating-a-pseudoconsole-session)
- [Arabic Typography (W3C)](https://www.w3.org/TR/alreq/)

---

## 🗂️ فهرس الوثائق

| الملف | المحتوى |
|-------|---------|
| [QALAM_ROADMAP_1.md](QALAM_ROADMAP_1.md) | النظرة العامة، المراحل 1-3 |
| [QALAM_ROADMAP_2.md](QALAM_ROADMAP_2.md) | المراحل 4-6 |
| [QALAM_ROADMAP_3.md](QALAM_ROADMAP_3.md) | المراحل 7-9 |
| [QALAM_ROADMAP_4.md](QALAM_ROADMAP_4.md) | المراحل 10-12، المواصفات ← أنت هنا |

---

*بُني بـ ❤️ للمبرمجين العرب*

</div>
```

---

## ✅ اكتملت الخارطة!

لقد أنشأت 4 ملفات كاملة:

| الملف | الأسطر | المحتوى |
|-------|--------|---------|
| `QALAM_ROADMAP_1.md` | ~850 | النظرة العامة، CLI، Highlighter، GUI Core |
| `QALAM_ROADMAP_2.md` | ~950 | Text Engine، Terminal، IDE Features |
| `QALAM_ROADMAP_3.md` | ~950 | Compiler، Debugger، Advanced Features |
| `QALAM_ROADMAP_4.md` | ~750 | Package Manager، Plugins، Testing، Specs |

### 📋 ملخص المراحل الـ 12:

| # | المرحلة | الوصف |
|---|---------|-------|
| 1 | CLI Foundation | واجهة سطر أوامر عربية |
| 2 | Syntax Highlighter | تلوين الكود |
| 3 | GUI Core | نواة الواجهة الرسومية |
| 4 | Text Engine | محرك تحرير النصوص + RTL |
| 5 | **Integrated Terminal** | 🆕 طرفية مدمجة تحل مشكلة CMD |
| 6 | IDE Features | إكمال، تشخيص، Minimap |
| 7 | Compiler Integration | بناء وتشغيل |
| 8 | **Debugger** | 🆕 منقح تفاعلي |
| 9 | Advanced Features | إعدادات، جلسات، بحث |
| 10 | Package Manager | مدير الحزم |
| 11 | **Plugin System** | 🆕 نظام إضافات |
| 12 | **Testing Framework** | 🆕 إطار اختبارات |