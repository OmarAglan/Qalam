# قلم - Qalam IDE
## المراحل 4-6: المحرر، الطرفية، ميزات IDE

**السابق:** [QALAM_ROADMAP_1.md](QALAM_ROADMAP_1.md) - النظرة العامة والمراحل 1-3

---

## المرحلة ٤: محرك تحرير النصوص ✏️

**الهدف:** محرر نصوص كامل الميزات مع دعم RTL حقيقي

**التقنيات:** Gap Buffer, Win32, DirectWrite, Uniscribe

### 4.1 هيكل Buffer النص

```c
// === buffer.h ===

#ifndef QALAM_BUFFER_H
#define QALAM_BUFFER_H

#include <windows.h>
#include <stddef.h>
#include <stdbool.h>

#define INITIAL_GAP_SIZE    4096
#define GAP_GROW_SIZE       2048
#define MAX_LINE_LENGTH     10000
#define UNDO_STACK_SIZE     1000

// === Gap Buffer ===
// هيكل بيانات فعال للتحرير
// [text before gap][....GAP....][text after gap]

typedef struct {
    wchar_t* data;            // البيانات
    size_t size;              // الحجم الكلي
    size_t gap_start;         // بداية الفجوة
    size_t gap_end;           // نهاية الفجوة
    
    // تتبع الأسطر
    size_t* line_starts;      // مواقع بداية كل سطر
    size_t line_count;        // عدد الأسطر
    size_t line_capacity;     // السعة المخصصة
    
    // معلومات الملف
    wchar_t filepath[MAX_PATH];
    bool is_modified;
    bool is_readonly;
    int encoding;             // UTF-8, UTF-16, etc.
    int line_ending;          // LF, CRLF, CR
} QalamBuffer;

// === إنشاء وتدمير ===
QalamBuffer* buffer_create(void);
void buffer_destroy(QalamBuffer* buf);
bool buffer_load_file(QalamBuffer* buf, const wchar_t* path);
bool buffer_save_file(QalamBuffer* buf, const wchar_t* path);

// === العمليات الأساسية ===
void buffer_insert(QalamBuffer* buf, size_t pos, const wchar_t* text, size_t len);
void buffer_delete(QalamBuffer* buf, size_t pos, size_t len);
void buffer_insert_char(QalamBuffer* buf, size_t pos, wchar_t ch);
void buffer_delete_char(QalamBuffer* buf, size_t pos);

// === الاستعلام ===
size_t buffer_length(QalamBuffer* buf);
wchar_t buffer_char_at(QalamBuffer* buf, size_t pos);
void buffer_get_text(QalamBuffer* buf, size_t start, size_t len, wchar_t* out);
size_t buffer_get_line(QalamBuffer* buf, size_t line_num, wchar_t* out, size_t max_len);

// === تحويل الموقع ===
size_t buffer_pos_to_line(QalamBuffer* buf, size_t pos);
size_t buffer_pos_to_column(QalamBuffer* buf, size_t pos);
size_t buffer_line_col_to_pos(QalamBuffer* buf, size_t line, size_t col);
size_t buffer_line_start(QalamBuffer* buf, size_t line);
size_t buffer_line_end(QalamBuffer* buf, size_t line);
size_t buffer_line_length(QalamBuffer* buf, size_t line);

#endif
```

### 4.2 تنفيذ Gap Buffer

```c
// === buffer.c ===

#include "buffer.h"
#include <stdlib.h>
#include <string.h>

QalamBuffer* buffer_create(void) {
    QalamBuffer* buf = calloc(1, sizeof(QalamBuffer));
    if (!buf) return NULL;
    
    buf->size = INITIAL_GAP_SIZE;
    buf->data = malloc(buf->size * sizeof(wchar_t));
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->gap_start = 0;
    buf->gap_end = buf->size;
    
    // سطر واحد فارغ
    buf->line_capacity = 1024;
    buf->line_starts = malloc(buf->line_capacity * sizeof(size_t));
    buf->line_starts[0] = 0;
    buf->line_count = 1;
    
    buf->is_modified = false;
    buf->encoding = 65001;  // UTF-8
    buf->line_ending = 0;   // LF
    
    return buf;
}

void buffer_destroy(QalamBuffer* buf) {
    if (!buf) return;
    free(buf->data);
    free(buf->line_starts);
    free(buf);
}

// حجم الفجوة
static size_t gap_size(QalamBuffer* buf) {
    return buf->gap_end - buf->gap_start;
}

// طول النص الفعلي
size_t buffer_length(QalamBuffer* buf) {
    return buf->size - gap_size(buf);
}

// تحريك الفجوة لموقع معين
static void move_gap(QalamBuffer* buf, size_t pos) {
    if (pos == buf->gap_start) return;
    
    size_t gs = gap_size(buf);
    
    if (pos < buf->gap_start) {
        // تحريك لليسار
        size_t move_len = buf->gap_start - pos;
        memmove(buf->data + buf->gap_end - move_len,
                buf->data + pos,
                move_len * sizeof(wchar_t));
        buf->gap_start = pos;
        buf->gap_end = pos + gs;
    } else {
        // تحريك لليمين
        size_t move_len = pos - buf->gap_start;
        memmove(buf->data + buf->gap_start,
                buf->data + buf->gap_end,
                move_len * sizeof(wchar_t));
        buf->gap_start = pos;
        buf->gap_end = pos + gs;
    }
}

// توسيع الفجوة
static void grow_gap(QalamBuffer* buf, size_t needed) {
    if (gap_size(buf) >= needed) return;
    
    size_t new_size = buf->size + needed + GAP_GROW_SIZE;
    wchar_t* new_data = malloc(new_size * sizeof(wchar_t));
    
    // نسخ ما قبل الفجوة
    memcpy(new_data, buf->data, buf->gap_start * sizeof(wchar_t));
    
    // نسخ ما بعد الفجوة
    size_t after_gap = buf->size - buf->gap_end;
    memcpy(new_data + new_size - after_gap,
           buf->data + buf->gap_end,
           after_gap * sizeof(wchar_t));
    
    free(buf->data);
    buf->data = new_data;
    buf->gap_end = new_size - after_gap;
    buf->size = new_size;
}

void buffer_insert(QalamBuffer* buf, size_t pos, 
                   const wchar_t* text, size_t len) {
    if (!buf || !text || len == 0) return;
    
    grow_gap(buf, len);
    move_gap(buf, pos);
    
    memcpy(buf->data + buf->gap_start, text, len * sizeof(wchar_t));
    buf->gap_start += len;
    buf->is_modified = true;
    
    // تحديث الأسطر
    buffer_recompute_lines(buf);
}

void buffer_delete(QalamBuffer* buf, size_t pos, size_t len) {
    if (!buf || len == 0) return;
    
    move_gap(buf, pos);
    buf->gap_end += len;
    buf->is_modified = true;
    
    buffer_recompute_lines(buf);
}

wchar_t buffer_char_at(QalamBuffer* buf, size_t pos) {
    if (pos >= buffer_length(buf)) return L'\0';
    
    if (pos < buf->gap_start) {
        return buf->data[pos];
    } else {
        return buf->data[pos + gap_size(buf)];
    }
}
```

### 4.3 نظام التراجع/الإعادة (Undo/Redo)

```c
// === undo.h ===

typedef enum {
    EDIT_INSERT,
    EDIT_DELETE,
    EDIT_REPLACE,
} EditType;

typedef struct {
    EditType type;
    size_t position;
    wchar_t* text;            // النص المُدرج أو المحذوف
    size_t length;
    size_t cursor_before;     // موقع المؤشر قبل
    size_t cursor_after;      // موقع المؤشر بعد
    bool is_grouped;          // جزء من مجموعة
    ULONGLONG timestamp;      // للتجميع الزمني
} EditAction;

typedef struct {
    EditAction* actions;
    size_t count;
    size_t capacity;
    size_t current;           // الموقع الحالي في المكدس
} UndoStack;

// === الدوال ===
UndoStack* undo_create(void);
void undo_destroy(UndoStack* stack);
void undo_push(UndoStack* stack, EditAction* action);
EditAction* undo_pop(UndoStack* stack);
EditAction* redo_pop(UndoStack* stack);
bool undo_can_undo(UndoStack* stack);
bool undo_can_redo(UndoStack* stack);
void undo_begin_group(UndoStack* stack);
void undo_end_group(UndoStack* stack);
void undo_clear(UndoStack* stack);
```

### 4.4 إدارة المؤشر (Cursor)

```c
// === cursor.h ===

typedef struct {
    size_t position;          // الموقع في البافر
    size_t line;              // رقم السطر (من ٠)
    size_t column;            // رقم العمود (من ٠)
    size_t preferred_column;  // العمود المفضل للتنقل العمودي
    
    // التحديد
    bool has_selection;
    size_t selection_start;
    size_t selection_end;
    size_t selection_anchor;  // نقطة بداية التحديد
    
    // RTL
    bool rtl_context;         // السياق الحالي RTL
    int bidi_level;           // مستوى Bidi
} QalamCursor;

typedef struct {
    QalamCursor primary;      // المؤشر الرئيسي
    QalamCursor* secondary;   // مؤشرات إضافية (multi-cursor)
    size_t secondary_count;
} CursorManager;

// === حركة المؤشر ===
void cursor_move_left(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);
void cursor_move_right(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);
void cursor_move_up(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);
void cursor_move_down(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);

void cursor_move_word_left(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);
void cursor_move_word_right(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);

void cursor_move_line_start(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);
void cursor_move_line_end(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);

void cursor_move_document_start(QalamCursor* cur, bool extend_selection);
void cursor_move_document_end(QalamCursor* cur, QalamBuffer* buf, bool extend_selection);

// === التحديد ===
void cursor_select_all(QalamCursor* cur, QalamBuffer* buf);
void cursor_select_word(QalamCursor* cur, QalamBuffer* buf);
void cursor_select_line(QalamCursor* cur, QalamBuffer* buf);
void cursor_clear_selection(QalamCursor* cur);
bool cursor_has_selection(QalamCursor* cur);
void cursor_get_selection(QalamCursor* cur, size_t* start, size_t* end);
```

### 4.5 دعم RTL والنص ثنائي الاتجاه (Bidi)

```c
// === bidi.h ===

#ifndef QALAM_BIDI_H
#define QALAM_BIDI_H

#include <windows.h>
#include <usp10.h>  // Uniscribe

// اتجاه النص
typedef enum {
    DIR_LTR = 0,
    DIR_RTL = 1,
    DIR_NEUTRAL = 2,
} TextDirection;

// جزء من النص له اتجاه واحد
typedef struct {
    size_t start;             // بداية الجزء
    size_t length;            // الطول
    TextDirection direction;  // الاتجاه
    int embedding_level;      // مستوى التضمين
} BidiRun;

// تخطيط سطر كامل
typedef struct {
    BidiRun* runs;
    size_t run_count;
    
    // تحويل بين المنطقي والبصري
    size_t* logical_to_visual;
    size_t* visual_to_logical;
    
    TextDirection base_direction;
} BidiLayout;

// === الدوال ===
BidiLayout* bidi_analyze(const wchar_t* text, size_t length, 
                         TextDirection base_dir);
void bidi_destroy(BidiLayout* layout);

// تحويل الموقع
size_t bidi_logical_to_visual(BidiLayout* layout, size_t logical_pos);
size_t bidi_visual_to_logical(BidiLayout* layout, size_t visual_pos);

// كشف الاتجاه
TextDirection bidi_detect_direction(const wchar_t* text, size_t length);
bool bidi_is_rtl_char(wchar_t ch);

// حركة المؤشر البصرية
size_t bidi_move_visual_left(BidiLayout* layout, size_t pos);
size_t bidi_move_visual_right(BidiLayout* layout, size_t pos);

#endif
```

```c
// === bidi.c ===

#include "bidi.h"

bool bidi_is_rtl_char(wchar_t ch) {
    // Arabic
    if (ch >= 0x0600 && ch <= 0x06FF) return true;
    if (ch >= 0x0750 && ch <= 0x077F) return true;
    if (ch >= 0xFB50 && ch <= 0xFDFF) return true;
    if (ch >= 0xFE70 && ch <= 0xFEFF) return true;
    
    // Hebrew
    if (ch >= 0x0590 && ch <= 0x05FF) return true;
    
    // Arabic Extended
    if (ch >= 0x08A0 && ch <= 0x08FF) return true;
    
    return false;
}

bool bidi_is_ltr_char(wchar_t ch) {
    // Latin
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= 'a' && ch <= 'z') return true;
    
    return false;
}

TextDirection bidi_detect_direction(const wchar_t* text, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (bidi_is_rtl_char(text[i])) return DIR_RTL;
        if (bidi_is_ltr_char(text[i])) return DIR_LTR;
    }
    return DIR_NEUTRAL;
}

BidiLayout* bidi_analyze(const wchar_t* text, size_t length, 
                         TextDirection base_dir) {
    BidiLayout* layout = calloc(1, sizeof(BidiLayout));
    if (!layout) return NULL;
    
    layout->base_direction = base_dir;
    
    // تخصيص مصفوفات التحويل
    layout->logical_to_visual = malloc(length * sizeof(size_t));
    layout->visual_to_logical = malloc(length * sizeof(size_t));
    
    // استخدام Uniscribe للتحليل
    SCRIPT_CONTROL control = {0};
    SCRIPT_STATE state = {0};
    state.uBidiLevel = (base_dir == DIR_RTL) ? 1 : 0;
    
    // الحصول على عدد الأجزاء
    int max_items = (int)length + 1;
    SCRIPT_ITEM* items = malloc(max_items * sizeof(SCRIPT_ITEM));
    int item_count;
    
    HRESULT hr = ScriptItemize(text, (int)length, max_items,
                               &control, &state, items, &item_count);
    
    if (SUCCEEDED(hr)) {
        layout->run_count = item_count;
        layout->runs = malloc(item_count * sizeof(BidiRun));
        
        for (int i = 0; i < item_count; i++) {
            layout->runs[i].start = items[i].iCharPos;
            layout->runs[i].length = (i + 1 < item_count) 
                ? items[i + 1].iCharPos - items[i].iCharPos
                : length - items[i].iCharPos;
            layout->runs[i].embedding_level = items[i].a.s.uBidiLevel;
            layout->runs[i].direction = (items[i].a.s.uBidiLevel & 1) 
                ? DIR_RTL : DIR_LTR;
        }
        
        // حساب الترتيب البصري
        BYTE* levels = malloc(item_count);
        int* visual_order = malloc(item_count * sizeof(int));
        
        for (int i = 0; i < item_count; i++) {
            levels[i] = (BYTE)layout->runs[i].embedding_level;
        }
        
        ScriptLayout(item_count, levels, visual_order, NULL);
        
        // بناء خرائط التحويل
        size_t visual_pos = 0;
        for (int v = 0; v < item_count; v++) {
            int logical_run = visual_order[v];
            BidiRun* run = &layout->runs[logical_run];
            
            if (run->direction == DIR_RTL) {
                // عكس داخل الجزء RTL
                for (size_t j = run->length; j > 0; j--) {
                    size_t log_pos = run->start + j - 1;
                    layout->logical_to_visual[log_pos] = visual_pos;
                    layout->visual_to_logical[visual_pos] = log_pos;
                    visual_pos++;
                }
            } else {
                for (size_t j = 0; j < run->length; j++) {
                    size_t log_pos = run->start + j;
                    layout->logical_to_visual[log_pos] = visual_pos;
                    layout->visual_to_logical[visual_pos] = log_pos;
                    visual_pos++;
                }
            }
        }
        
        free(levels);
        free(visual_order);
    }
    
    free(items);
    return layout;
}
```

### 4.6 مهام المرحلة ٤

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| Gap Buffer | إدراج/حذف فعال | ⬜ |
| Unicode | دعم UTF-16 Surrogate Pairs | ⬜ |
| تتبع الأسطر | بحث سريع عن رقم السطر | ⬜ |
| إدارة المؤشر | موقع المؤشر مع RTL | ⬜ |
| التحديد | تحديد بالماوس ولوحة المفاتيح | ⬜ |
| نسخ/لصق | عمليات الحافظة | ⬜ |
| تراجع/إعادة | تاريخ التحرير مع التجميع | ⬜ |
| التفاف الأسطر | التفاف ناعم اختياري | ⬜ |
| خوارزمية Bidi | معالجة النص المختلط | ⬜ |
| IME | دعم لوحات المفاتيح العربية | ⬜ |

---

## المرحلة ٥: الطرفية المدمجة 🖥️

**الهدف:** طرفية عربية مدمجة تحل مشاكل CMD/PowerShell

**التقنيات:** ConPTY API, Win32, ANSI Parser

### 5.1 هيكل الطرفية

```c
// === terminal.h ===

#ifndef QALAM_TERMINAL_H
#define QALAM_TERMINAL_H

#include <windows.h>
#include <stdbool.h>

#define TERMINAL_MAX_COLS       256
#define TERMINAL_SCROLLBACK     10000
#define TERMINAL_TAB_SIZE       8

// خلية واحدة في الطرفية
typedef struct {
    wchar_t ch;               // الحرف
    BYTE fg_color;            // لون النص (0-255)
    BYTE bg_color;            // لون الخلفية
    BYTE attrs;               // Bold, Italic, Underline, etc.
} TerminalCell;

// سطر في الطرفية
typedef struct {
    TerminalCell* cells;
    size_t length;
    bool wrapped;             // ملتف من السطر السابق
    bool is_rtl;              // سطر RTL
} TerminalLine;

// حالة محلل ANSI
typedef struct {
    enum {
        ANSI_NORMAL,
        ANSI_ESCAPE,
        ANSI_CSI,
        ANSI_OSC,
    } state;
    
    int params[16];
    int param_count;
    wchar_t intermediate;
} AnsiParser;

// الطرفية الرئيسية
typedef struct {
    HWND hwnd;                // نافذة الطرفية
    
    // === المخزن ===
    TerminalLine** lines;     // أسطر الإخراج
    size_t line_count;
    size_t scrollback_size;
    size_t scroll_offset;     // موقع التمرير
    
    // === الشاشة ===
    int cols;                 // عدد الأعمدة
    int rows;                 // عدد الصفوف
    int cursor_x;             // عمود المؤشر
    int cursor_y;             // صف المؤشر
    bool cursor_visible;
    
    // === الألوان ===
    COLORREF palette[256];
    BYTE current_fg;
    BYTE current_bg;
    BYTE current_attrs;
    
    // === العملية ===
    HPCON hpc;                // Pseudo Console handle
    HANDLE hPipeIn;           // أنبوب الإدخال
    HANDLE hPipeOut;          // أنبوب الإخراج
    HANDLE hProcess;          // العملية
    HANDLE hThread;           // خيط القراءة
    DWORD process_id;
    bool is_running;
    
    // === الإدخال ===
    wchar_t* input_buffer;
    size_t input_length;
    size_t input_capacity;
    size_t input_cursor;
    
    // === التاريخ ===
    wchar_t** history;
    size_t history_count;
    size_t history_capacity;
    size_t history_index;
    
    // === محلل ANSI ===
    AnsiParser ansi;
    
    // === الخط ===
    HFONT hfont;
    int char_width;
    int char_height;
    
    // === RTL ===
    bool rtl_mode;
    bool auto_detect_rtl;
    
} QalamTerminal;

// === إنشاء وتدمير ===
QalamTerminal* terminal_create(HWND parent, int x, int y, int w, int h);
void terminal_destroy(QalamTerminal* term);

// === التشغيل ===
bool terminal_start_process(QalamTerminal* term, const wchar_t* command);
bool terminal_start_shell(QalamTerminal* term);  // CMD أو PowerShell
void terminal_stop_process(QalamTerminal* term);

// === الإدخال/الإخراج ===
void terminal_write(QalamTerminal* term, const wchar_t* text);
void terminal_send_input(QalamTerminal* term, const wchar_t* text);
void terminal_send_key(QalamTerminal* term, WORD vk, bool ctrl, bool alt, bool shift);

// === العرض ===
void terminal_paint(QalamTerminal* term, HDC hdc);
void terminal_resize(QalamTerminal* term, int cols, int rows);
void terminal_scroll(QalamTerminal* term, int delta);
void terminal_clear(QalamTerminal* term);

// === التاريخ ===
void terminal_history_add(QalamTerminal* term, const wchar_t* command);
const wchar_t* terminal_history_prev(QalamTerminal* term);
const wchar_t* terminal_history_next(QalamTerminal* term);

#endif
```

### 5.2 إنشاء Pseudo Console (ConPTY)

```c
// === terminal.c ===

#include "terminal.h"
#include <process.h>

// ألوان ANSI الافتراضية (سمة داكنة)
static const COLORREF DEFAULT_PALETTE[16] = {
    0x0C0C0C,  // 0: أسود
    0x0037DA,  // 1: أحمر (BGR)
    0x13A10E,  // 2: أخضر
    0xC19C00,  // 3: أصفر
    0x3B78FF,  // 4: أزرق
    0x881798,  // 5: بنفسجي
    0x3A96DD,  // 6: سماوي
    0xCCCCCC,  // 7: أبيض
    0x767676,  // 8: رمادي غامق
    0x5648E7,  // 9: أحمر فاتح
    0x0CC616,  // 10: أخضر فاتح
    0xF9F1A5,  // 11: أصفر فاتح
    0xFF783B,  // 12: أزرق فاتح
    0xE2219C,  // 13: بنفسجي فاتح
    0x61D6D6,  // 14: سماوي فاتح
    0xF2F2F2,  // 15: أبيض ساطع
};

QalamTerminal* terminal_create(HWND parent, int x, int y, int w, int h) {
    QalamTerminal* term = calloc(1, sizeof(QalamTerminal));
    if (!term) return NULL;
    
    // نسخ الألوان
    memcpy(term->palette, DEFAULT_PALETTE, sizeof(DEFAULT_PALETTE));
    
    // توسيع لـ 256 لون
    for (int i = 16; i < 256; i++) {
        if (i < 232) {
            // 216 لون (6x6x6 cube)
            int idx = i - 16;
            int r = (idx / 36) * 51;
            int g = ((idx / 6) % 6) * 51;
            int b = (idx % 6) * 51;
            term->palette[i] = RGB(r, g, b);
        } else {
            // 24 درجة رمادي
            int gray = (i - 232) * 10 + 8;
            term->palette[i] = RGB(gray, gray, gray);
        }
    }
    
    term->current_fg = 7;    // أبيض
    term->current_bg = 0;    // أسود
    
    // إنشاء نافذة الطرفية
    term->hwnd = CreateWindowExW(
        0,
        L"QalamTerminalClass",
        NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        x, y, w, h,
        parent,
        NULL,
        GetModuleHandle(NULL),
        term
    );
    
    // إنشاء الخط
    term->hfont = CreateFontW(
        16, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN,
        L"Cascadia Mono"
    );
    
    // حساب أبعاد الحرف
    HDC hdc = GetDC(term->hwnd);
    HFONT old_font = SelectObject(hdc, term->hfont);
    TEXTMETRICW tm;
    GetTextMetricsW(hdc, &tm);
    term->char_width = tm.tmAveCharWidth;
    term->char_height = tm.tmHeight;
    SelectObject(hdc, old_font);
    ReleaseDC(term->hwnd, hdc);
    
    // حساب الأبعاد
    term->cols = w / term->char_width;
    term->rows = h / term->char_height;
    
    // تخصيص المخزن
    term->scrollback_size = TERMINAL_SCROLLBACK;
    term->lines = calloc(term->scrollback_size, sizeof(TerminalLine*));
    
    // تخصيص التاريخ
    term->history_capacity = 1000;
    term->history = calloc(term->history_capacity, sizeof(wchar_t*));
    
    // تخصيص الإدخال
    term->input_capacity = 1024;
    term->input_buffer = calloc(term->input_capacity, sizeof(wchar_t));
    
    term->cursor_visible = true;
    term->auto_detect_rtl = true;
    
    return term;
}

bool terminal_start_shell(QalamTerminal* term) {
    if (!term) return false;
    
    // تحديد الشل
    wchar_t shell_path[MAX_PATH];
    
    // محاولة PowerShell أولاً
    if (GetEnvironmentVariableW(L"COMSPEC", shell_path, MAX_PATH) == 0) {
        wcscpy(shell_path, L"cmd.exe");
    }
    
    return terminal_start_process(term, shell_path);
}

bool terminal_start_process(QalamTerminal* term, const wchar_t* command) {
    if (!term || term->is_running) return false;
    
    HRESULT hr;
    
    // إنشاء الأنابيب
    HANDLE hPipeInRead, hPipeInWrite;
    HANDLE hPipeOutRead, hPipeOutWrite;
    
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    
    if (!CreatePipe(&hPipeInRead, &hPipeInWrite, &sa, 0)) return false;
    if (!CreatePipe(&hPipeOutRead, &hPipeOutWrite, &sa, 0)) {
        CloseHandle(hPipeInRead);
        CloseHandle(hPipeInWrite);
        return false;
    }
    
    // إنشاء Pseudo Console
    COORD size = {(SHORT)term->cols, (SHORT)term->rows};
    
    hr = CreatePseudoConsole(size, hPipeInRead, hPipeOutWrite, 0, &term->hpc);
    
    if (FAILED(hr)) {
        CloseHandle(hPipeInRead);
        CloseHandle(hPipeInWrite);
        CloseHandle(hPipeOutRead);
        CloseHandle(hPipeOutWrite);
        return false;
    }
    
    CloseHandle(hPipeInRead);
    CloseHandle(hPipeOutWrite);
    
    term->hPipeIn = hPipeInWrite;
    term->hPipeOut = hPipeOutRead;
    
    // إعداد العملية مع Pseudo Console
    STARTUPINFOEXW si = {0};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    
    SIZE_T attr_size;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
    si.lpAttributeList = malloc(attr_size);
    InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attr_size);
    
    UpdateProcThreadAttribute(si.lpAttributeList, 0,
        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
        term->hpc, sizeof(HPCON), NULL, NULL);
    
    // إنشاء العملية
    PROCESS_INFORMATION pi = {0};
    wchar_t cmd_line[MAX_PATH];
    wcscpy(cmd_line, command);
    
    BOOL success = CreateProcessW(
        NULL,
        cmd_line,
        NULL, NULL,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL, NULL,
        &si.StartupInfo,
        &pi
    );
    
    DeleteProcThreadAttributeList(si.lpAttributeList);
    free(si.lpAttributeList);
    
    if (!success) {
        ClosePseudoConsole(term->hpc);
        CloseHandle(term->hPipeIn);
        CloseHandle(term->hPipeOut);
        return false;
    }
    
    term->hProcess = pi.hProcess;
    term->process_id = pi.dwProcessId;
    term->is_running = true;
    
    CloseHandle(pi.hThread);
    
    // بدء خيط القراءة
    term->hThread = (HANDLE)_beginthreadex(
        NULL, 0, terminal_read_thread, term, 0, NULL);
    
    return true;
}
```

### 5.3 محلل ANSI

```c
// === ansi_parser.c ===

void terminal_process_ansi(QalamTerminal* term, const char* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        
        switch (term->ansi.state) {
        case ANSI_NORMAL:
            if (c == '\033') {
                term->ansi.state = ANSI_ESCAPE;
            } else if (c == '\n') {
                terminal_newline(term);
            } else if (c == '\r') {
                term->cursor_x = 0;
            } else if (c == '\t') {
                term->cursor_x = (term->cursor_x + TERMINAL_TAB_SIZE) 
                                  & ~(TERMINAL_TAB_SIZE - 1);
            } else if (c == '\b') {
                if (term->cursor_x > 0) term->cursor_x--;
            } else if (c == '\a') {
                MessageBeep(MB_OK);  // صوت
            } else if ((unsigned char)c >= 32) {
                terminal_put_char(term, c);
            }
            break;
            
        case ANSI_ESCAPE:
            if (c == '[') {
                term->ansi.state = ANSI_CSI;
                term->ansi.param_count = 0;
                memset(term->ansi.params, 0, sizeof(term->ansi.params));
            } else if (c == ']') {
                term->ansi.state = ANSI_OSC;
            } else {
                term->ansi.state = ANSI_NORMAL;
            }
            break;
            
        case ANSI_CSI:
            if (c >= '0' && c <= '9') {
                term->ansi.params[term->ansi.param_count] *= 10;
                term->ansi.params[term->ansi.param_count] += c - '0';
            } else if (c == ';') {
                if (term->ansi.param_count < 15) {
                    term->ansi.param_count++;
                }
            } else if (c >= 0x40 && c <= 0x7E) {
                // نهاية التسلسل
                term->ansi.param_count++;
                terminal_execute_csi(term, c);
                term->ansi.state = ANSI_NORMAL;
            }
            break;
            
        case ANSI_OSC:
            if (c == '\a' || c == '\033') {
                term->ansi.state = ANSI_NORMAL;
            }
            break;
        }
    }
}

void terminal_execute_csi(QalamTerminal* term, char cmd) {
    int* p = term->ansi.params;
    int n = term->ansi.param_count;
    
    switch (cmd) {
    case 'm':  // SGR - تنسيق النص
        for (int i = 0; i < n; i++) {
            int code = p[i];
            
            if (code == 0) {
                // إعادة تعيين
                term->current_fg = 7;
                term->current_bg = 0;
                term->current_attrs = 0;
            } else if (code == 1) {
                term->current_attrs |= 1;  // Bold
            } else if (code == 4) {
                term->current_attrs |= 2;  // Underline
            } else if (code == 7) {
                term->current_attrs |= 4;  // Reverse
            } else if (code >= 30 && code <= 37) {
                term->current_fg = code - 30;
            } else if (code >= 40 && code <= 47) {
                term->current_bg = code - 40;
            } else if (code >= 90 && code <= 97) {
                term->current_fg = code - 90 + 8;
            } else if (code >= 100 && code <= 107) {
                term->current_bg = code - 100 + 8;
            } else if (code == 38 && i + 2 < n && p[i+1] == 5) {
                // 256 color foreground
                term->current_fg = p[i+2];
                i += 2;
            } else if (code == 48 && i + 2 < n && p[i+1] == 5) {
                // 256 color background
                term->current_bg = p[i+2];
                i += 2;
            }
        }
        break;
        
    case 'A':  // CUU - تحريك للأعلى
        term->cursor_y -= (n > 0 && p[0] > 0) ? p[0] : 1;
        if (term->cursor_y < 0) term->cursor_y = 0;
        break;
        
    case 'B':  // CUD - تحريك للأسفل
        term->cursor_y += (n > 0 && p[0] > 0) ? p[0] : 1;
        break;
        
    case 'C':  // CUF - تحريك لليمين
        term->cursor_x += (n > 0 && p[0] > 0) ? p[0] : 1;
        break;
        
    case 'D':  // CUB - تحريك لليسار
        term->cursor_x -= (n > 0 && p[0] > 0) ? p[0] : 1;
        if (term->cursor_x < 0) term->cursor_x = 0;
        break;
        
    case 'H':  // CUP - تحريك مطلق
    case 'f':
        term->cursor_y = (n > 0 && p[0] > 0) ? p[0] - 1 : 0;
        term->cursor_x = (n > 1 && p[1] > 0) ? p[1] - 1 : 0;
        break;
        
    case 'J':  // ED - مسح الشاشة
        if (p[0] == 2) {
            terminal_clear(term);
        }
        break;
        
    case 'K':  // EL - مسح السطر
        terminal_clear_line(term, p[0]);
        break;
    }
}
```

### 5.4 عرض الطرفية مع RTL

```c
// === terminal_render.c ===

void terminal_paint(QalamTerminal* term, HDC hdc) {
    if (!term) return;
    
    RECT rc;
    GetClientRect(term->hwnd, &rc);
    
    // خلفية
    HBRUSH bg_brush = CreateSolidBrush(term->palette[0]);
    FillRect(hdc, &rc, bg_brush);
    DeleteObject(bg_brush);
    
    HFONT old_font = SelectObject(hdc, term->hfont);
    SetBkMode(hdc, TRANSPARENT);
    
    // رسم الأسطر المرئية
    int start_line = (int)term->scroll_offset;
    int visible_lines = term->rows;
    
    for (int y = 0; y < visible_lines; y++) {
        int line_idx = start_line + y;
        if (line_idx >= (int)term->line_count) break;
        
        TerminalLine* line = term->lines[line_idx];
        if (!line) continue;
        
        int screen_y = y * term->char_height;
        
        // كشف RTL تلقائي
        bool line_rtl = term->rtl_mode;
        if (term->auto_detect_rtl) {
            line_rtl = terminal_detect_line_rtl(line);
        }
        
        if (line_rtl) {
            // رسم من اليمين
            terminal_paint_line_rtl(term, hdc, line, screen_y);
        } else {
            // رسم من اليسار
            terminal_paint_line_ltr(term, hdc, line, screen_y);
        }
    }
    
    // رسم المؤشر
    if (term->cursor_visible && term->is_running) {
        int cx = term->cursor_x * term->char_width;
        int cy = (term->cursor_y - start_line) * term->char_height;
        
        if (cy >= 0 && cy < rc.bottom) {
            RECT cursor_rc = {cx, cy, cx + term->char_width, cy + term->char_height};
            
            // مؤشر وميض
            static bool blink_state = true;
            if (blink_state) {
                InvertRect(hdc, &cursor_rc);
            }
        }
    }
    
    SelectObject(hdc, old_font);
}

void terminal_paint_line_rtl(QalamTerminal* term, HDC hdc, 
                             TerminalLine* line, int y) {
    RECT rc;
    GetClientRect(term->hwnd, &rc);
    
    int x = rc.right;  // البدء من اليمين
    
    for (size_t i = 0; i < line->length; i++) {
        TerminalCell* cell = &line->cells[i];
        
        x -= term->char_width;
        
        // رسم الخلفية
        if (cell->bg_color != 0) {
            RECT cell_rc = {x, y, x + term->char_width, y + term->char_height};
            HBRUSH brush = CreateSolidBrush(term->palette[cell->bg_color]);
            FillRect(hdc, &cell_rc, brush);
            DeleteObject(brush);
        }
        
        // رسم الحرف
        if (cell->ch != L' ' && cell->ch != 0) {
            SetTextColor(hdc, term->palette[cell->fg_color]);
            TextOutW(hdc, x, y, &cell->ch, 1);
        }
    }
}
```

### 5.5 مهام المرحلة ٥

| المهمة | الوصف | الأولوية | الحالة |
|--------|-------|----------|--------|
| إنشاء Pseudo Console | استخدام ConPTY API | 🔴 عالية | ⬜ |
| عرض النص العربي | رندر صحيح للعربية | 🔴 عالية | ⬜ |
| محلل ANSI الكامل | ألوان، تنسيق، حركة المؤشر | 🔴 عالية | ⬜ |
| تاريخ الأوامر | حفظ واستدعاء الأوامر | 🟡 متوسطة | ⬜ |
| نسخ/لصق ذكي | مع الحفاظ على اتجاه النص | 🔴 عالية | ⬜ |
| أوضاع متعددة | CMD, PowerShell, Baa REPL | 🟡 متوسطة | ⬜ |
| تقسيم الطرفية | عدة طرفيات متجاورة | 🟢 منخفضة | ⬜ |
| Scrollback | تمرير للأعلى | 🟡 متوسطة | ⬜ |
| البحث في الإخراج | Ctrl+Shift+F | 🟢 منخفضة | ⬜ |

---

## المرحلة ٦: ميزات IDE 🛠️

**الهدف:** قدرات IDE احترافية

### 6.1 تلوين الصياغة المباشر

```c
// === syntax_highlight.h ===

typedef struct {
    size_t start;
    size_t length;
    TokenType type;
    COLORREF color;
} HighlightSpan;

typedef struct {
    HighlightSpan* spans;
    size_t count;
    size_t capacity;
    size_t version;           // لمعرفة إذا تغير
} LineHighlight;

typedef struct {
    LineHighlight* lines;
    size_t line_count;
    
    // ألوان السمة
    COLORREF colors[16];
    
    // الـ Lexer
    void* lexer_state;
} SyntaxHighlighter;

// === الدوال ===
SyntaxHighlighter* highlighter_create(void);
void highlighter_destroy(SyntaxHighlighter* hl);
void highlighter_set_theme(SyntaxHighlighter* hl, const wchar_t* theme);
void highlighter_update_line(SyntaxHighlighter* hl, QalamBuffer* buf, size_t line);
void highlighter_update_range(SyntaxHighlighter* hl, QalamBuffer* buf, 
                              size_t start_line, size_t end_line);
void highlighter_invalidate_from(SyntaxHighlighter* hl, size_t line);
```

### 6.2 الإكمال التلقائي

```c
// === completion.h ===

typedef enum {
    COMPLETION_KEYWORD,       // صحيح، إذا، طالما
    COMPLETION_TYPE,          // صحيح، نص، حرف
    COMPLETION_FUNCTION,      // الرئيسية، جمع
    COMPLETION_VARIABLE,      // س، متغير
    COMPLETION_SNIPPET,       // قوالب
    COMPLETION_FILE,          // ملفات #تضمين
} CompletionKind;

typedef struct {
    wchar_t label[64];        // العنوان
    wchar_t insert_text[256]; // النص المُدرج
    wchar_t detail[128];      // تفاصيل
    wchar_t docs[512];        // التوثيق
    CompletionKind kind;
    int priority;             // للترتيب
} CompletionItem;

typedef struct {
    CompletionItem* items;
    size_t count;
    size_t capacity;
    size_t selected;          // العنصر المحدد
    
    // موقع القائمة
    int x, y;
    int width, height;
    bool visible;
    
    // البادئة للتصفية
    wchar_t prefix[64];
    size_t prefix_len;
} CompletionList;

// === الدوال ===
CompletionList* completion_create(void);
void completion_destroy(CompletionList* list);
void completion_trigger(CompletionList* list, QalamBuffer* buf, 
                        size_t cursor_pos, void* symbols);
void completion_filter(CompletionList* list, const wchar_t* prefix);
void completion_select_next(CompletionList* list);
void completion_select_prev(CompletionList* list);
CompletionItem* completion_get_selected(CompletionList* list);
void completion_apply(CompletionList* list, QalamBuffer* buf, size_t cursor_pos);
```

### 6.3 الكلمات المفتاحية والقوالب

```c
// === snippets.c ===

typedef struct {
    const wchar_t* trigger;   // المحفز
    const wchar_t* content;   // المحتوى
    const wchar_t* desc;      // الوصف
} Snippet;

static const Snippet SNIPPETS[] = {
    // التحكم
    {L"إذا", 
     L"إذا (${1:شرط}) {\n\t${2:// كود}\n}",
     L"جملة شرطية"},
    
    {L"إذاوإلا",
     L"إذا (${1:شرط}) {\n\t${2:// كود}\n} وإلا {\n\t${3:// كود}\n}",
     L"جملة شرطية مع وإلا"},
    
    {L"طالما",
     L"طالما (${1:شرط}) {\n\t${2:// كود}\n}",
     L"حلقة طالما"},
    
    {L"لكل",
     L"لكل (صحيح ${1:س} = ${2:٠}؛ ${1:س} < ${3:ن}؛ ${1:س}++) {\n\t${4:// كود}\n}",
     L"حلقة لكل"},
    
    {L"اختر",
     L"اختر (${1:متغير}) {\n\tحالة ${2:قيمة}:\n\t\t${3:// كود}\n\t\tتوقف.\n\tافتراضي:\n\t\t${4:// كود}\n\t\tتوقف.\n}",
     L"جملة اختر"},
    
    // الدوال
    {L"دالة",
     L"صحيح ${1:اسم}(${2:صحيح معامل}) {\n\t${3:// كود}\n\tإرجع ${4:٠}.\n}",
     L"تعريف دالة"},
    
    {L"رئيسية",
     L"صحيح الرئيسية() {\n\t${1:// كود}\n\tإرجع ٠.\n}",
     L"الدالة الرئيسية"},
    
    // أخرى
    {L"طباعة",
     L"اطبع ${1:\"نص\"}.",
     L"طباعة نص"},
    
    {L"مصفوفة",
     L"صحيح ${1:قائمة}[${2:١٠}].",
     L"تعريف مصفوفة"},
    
    {NULL, NULL, NULL}
};
```

### 6.4 تشخيص الأخطاء

```c
// === diagnostics.h ===

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_INFO,
    DIAG_HINT,
} DiagnosticSeverity;

typedef struct {
    size_t line;
    size_t column_start;
    size_t column_end;
    DiagnosticSeverity severity;
    wchar_t message[256];
    wchar_t code[16];         // رمز الخطأ
} Diagnostic;

typedef struct {
    Diagnostic* items;
    size_t count;
    size_t capacity;
} DiagnosticList;

// === ألوان الخط السفلي ===
// خطأ: أحمر متموج ~~~~
// تحذير: أصفر متموج ~~~~
// معلومة: أزرق
// تلميح: رمادي منقط

// === الدوال ===
DiagnosticList* diagnostics_create(void);
void diagnostics_destroy(DiagnosticList* list);
void diagnostics_clear(DiagnosticList* list);
void diagnostics_add(DiagnosticList* list, Diagnostic* diag);
void diagnostics_parse_compiler_output(DiagnosticList* list, const wchar_t* output);
Diagnostic* diagnostics_at_position(DiagnosticList* list, size_t line, size_t col);
```

### 6.5 مهام المرحلة ٦

| المهمة | الوصف | الحالة |
|--------|-------|--------|
| **تلوين تزايدي** | إعادة تلوين المناطق المتغيرة فقط | ⬜ |
| **تخزين الرموز** | تخزين الرموز للعرض السريع | ⬜ |
| **مطابقة الأقواس** | تمييز `{}`, `[]`, `()` المتطابقة | ⬜ |
| **إكمال الكلمات المفتاحية** | اقتراح كلمات باء | ⬜ |
| **إكمال المعرفات** | اقتراح من جدول الرموز | ⬜ |
| **دعم القوالب** | قوالب الكود | ⬜ |
| **تلميحات المعاملات** | عرض معاملات الدوال | ⬜ |
| **أخطاء مباشرة** | كشف الأخطاء النحوية فوراً | ⬜ |
| **خطوط الأخطاء** | خط أحمر تحت الأخطاء | ⬜ |
| **لوحة الأخطاء** | قائمة أخطاء قابلة للنقر | ⬜ |
| **إصلاحات سريعة** | اقتراحات التصحيح | ⬜ |
| **Minimap** | خريطة مصغرة للكود | ⬜ |
| **طي الكود** | طي/فتح الكتل | ⬜ |

---

## 📊 ملخص المراحل 4-6

| المرحلة | المكون | الحالة | الأهمية |
|---------|--------|--------|---------|
| ٤ | Text Engine | ⬜ لم يبدأ | 🔴 حرجة |
| ٥ | Integrated Terminal | ⬜ لم يبدأ | 🔴 حرجة |
| ٦ | IDE Features | ⬜ لم يبدأ | 🟡 عالية |

---

**السابق:** [QALAM_ROADMAP_1.md](QALAM_ROADMAP_1.md)
**التالي:** [QALAM_ROADMAP_3.md](QALAM_ROADMAP_3.md) - المراحل 7-9