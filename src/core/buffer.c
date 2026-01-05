/**
 * @file buffer.c
 * @brief Qalam IDE - Gap Buffer Implementation
 * 
 * Implements the text buffer using a gap buffer data structure.
 * All text is stored internally as UTF-16 (wchar_t) for Windows compatibility.
 * The public API accepts/returns UTF-8 encoded text.
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 * 
 * @note Thread Safety: Functions in this file are NOT thread-safe.
 *       Thread synchronization must be handled at a higher level.
 */

#include "editor.h"
#include "qalam.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*=============================================================================
 * Configuration Constants
 *============================================================================*/

/** Initial buffer capacity in wchar_t units */
#define QALAM_BUFFER_INITIAL_CAPACITY   4096

/** Initial gap size in wchar_t units */
#define QALAM_BUFFER_INITIAL_GAP_SIZE   2048

/** Growth increment when gap is exhausted */
#define QALAM_BUFFER_GAP_GROW_SIZE      2048

/** Maximum supported buffer size (100 MB in wchar_t) */
#define QALAM_BUFFER_MAX_SIZE           (100 * 1024 * 1024 / sizeof(wchar_t))

/*=============================================================================
 * Internal Buffer Structure
 *============================================================================*/

/**
 * @brief Internal gap buffer structure
 * 
 * The gap buffer stores text in a contiguous array with a "gap" of unused
 * space at the cursor position. This allows O(1) insertions and deletions
 * at the cursor, with O(n) cost to move the cursor.
 * 
 * Layout: [text before gap][---GAP---][text after gap]
 *         ^                ^          ^               ^
 *         0            gap_start   gap_end        capacity
 */
struct QalamBuffer {
    wchar_t* data;              /**< The buffer array */
    size_t capacity;            /**< Total allocated size in wchar_t */
    size_t gap_start;           /**< Start of gap (cursor position) */
    size_t gap_end;             /**< End of gap (exclusive) */
    
    /* Cursor state */
    size_t cursor_line;         /**< Current line (0-based) */
    size_t cursor_column;       /**< Current column (0-based) */
    
    /* Line tracking */
    size_t line_count;          /**< Number of lines (at least 1) */
    
    /* Selection state */
    QalamSelection selection;   /**< Current selection */
    
    /* File metadata */
    wchar_t filepath[MAX_PATH]; /**< Associated file path */
    bool modified;              /**< Has unsaved changes */
    bool readonly;              /**< Read-only flag */
};

/*=============================================================================
 * Internal Helper Functions - Forward Declarations
 *============================================================================*/

static inline size_t buffer_gap_size(const QalamBuffer* buffer);
static inline size_t buffer_content_length(const QalamBuffer* buffer);
static size_t buffer_logical_to_physical(const QalamBuffer* buffer, size_t pos);
static wchar_t buffer_char_at_internal(const QalamBuffer* buffer, size_t pos);
static void buffer_move_gap_to(QalamBuffer* buffer, size_t pos);
static QalamResult buffer_ensure_gap_size(QalamBuffer* buffer, size_t needed);
static void buffer_update_line_count(QalamBuffer* buffer);
static size_t buffer_count_newlines_in_range(const QalamBuffer* buffer, size_t start, size_t len);
static void buffer_update_cursor_from_offset(QalamBuffer* buffer);
static size_t buffer_offset_from_line_column(const QalamBuffer* buffer, size_t line, size_t column);
static size_t buffer_get_line_start_offset(const QalamBuffer* buffer, size_t line);
static size_t buffer_get_line_length(const QalamBuffer* buffer, size_t line);
static bool is_high_surrogate(wchar_t ch);
static bool is_low_surrogate(wchar_t ch);
static int utf8_to_utf16(const char* utf8, size_t utf8_len, wchar_t* utf16, size_t utf16_size);
static int utf16_to_utf8(const wchar_t* utf16, size_t utf16_len, char* utf8, size_t utf8_size);

/*=============================================================================
 * Internal Helper Functions - Implementation
 *============================================================================*/

/**
 * @brief Get the current gap size
 */
static inline size_t buffer_gap_size(const QalamBuffer* buffer) {
    return buffer->gap_end - buffer->gap_start;
}

/**
 * @brief Get the content length (excluding gap)
 */
static inline size_t buffer_content_length(const QalamBuffer* buffer) {
    return buffer->capacity - buffer_gap_size(buffer);
}

/**
 * @brief Convert logical position to physical position in buffer array
 */
static size_t buffer_logical_to_physical(const QalamBuffer* buffer, size_t pos) {
    if (pos < buffer->gap_start) {
        return pos;
    } else {
        return pos + buffer_gap_size(buffer);
    }
}

/**
 * @brief Get character at logical position (internal, no bounds check)
 */
static wchar_t buffer_char_at_internal(const QalamBuffer* buffer, size_t pos) {
    size_t phys = buffer_logical_to_physical(buffer, pos);
    return buffer->data[phys];
}

/**
 * @brief Check if character is a high surrogate (UTF-16)
 */
static bool is_high_surrogate(wchar_t ch) {
    return (ch >= 0xD800 && ch <= 0xDBFF);
}

/**
 * @brief Check if character is a low surrogate (UTF-16)
 */
static bool is_low_surrogate(wchar_t ch) {
    return (ch >= 0xDC00 && ch <= 0xDFFF);
}

/**
 * @brief Move the gap to a specific logical position
 * 
 * This is the key operation that makes cursor-local operations O(1).
 */
static void buffer_move_gap_to(QalamBuffer* buffer, size_t pos) {
    if (pos == buffer->gap_start) {
        return; /* Already there */
    }
    
    size_t gs = buffer_gap_size(buffer);
    
    if (pos < buffer->gap_start) {
        /* Move gap left: shift text right into gap */
        size_t move_count = buffer->gap_start - pos;
        memmove(
            buffer->data + buffer->gap_end - move_count,
            buffer->data + pos,
            move_count * sizeof(wchar_t)
        );
        buffer->gap_start = pos;
        buffer->gap_end = pos + gs;
    } else {
        /* Move gap right: shift text left into gap */
        size_t move_count = pos - buffer->gap_start;
        memmove(
            buffer->data + buffer->gap_start,
            buffer->data + buffer->gap_end,
            move_count * sizeof(wchar_t)
        );
        buffer->gap_start = pos;
        buffer->gap_end = pos + gs;
    }
}

/**
 * @brief Ensure gap has at least 'needed' space
 * 
 * If gap is too small, reallocate buffer with doubled capacity plus growth increment.
 */
static QalamResult buffer_ensure_gap_size(QalamBuffer* buffer, size_t needed) {
    size_t current_gap = buffer_gap_size(buffer);
    
    if (current_gap >= needed) {
        return QALAM_OK;
    }
    
    /* Calculate new capacity: double current, plus ensure enough for needed + growth */
    size_t content_len = buffer_content_length(buffer);
    size_t new_capacity = buffer->capacity * 2;
    size_t min_capacity = content_len + needed + QALAM_BUFFER_GAP_GROW_SIZE;
    
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    
    /* Check maximum size */
    if (new_capacity > QALAM_BUFFER_MAX_SIZE) {
        if (min_capacity > QALAM_BUFFER_MAX_SIZE) {
            return QALAM_ERROR_OUT_OF_MEMORY;
        }
        new_capacity = QALAM_BUFFER_MAX_SIZE;
    }
    
    /* Allocate new buffer */
    wchar_t* new_data = (wchar_t*)malloc(new_capacity * sizeof(wchar_t));
    if (!new_data) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy text before gap */
    if (buffer->gap_start > 0) {
        memcpy(new_data, buffer->data, buffer->gap_start * sizeof(wchar_t));
    }
    
    /* Copy text after gap to end of new buffer */
    size_t after_gap_len = buffer->capacity - buffer->gap_end;
    if (after_gap_len > 0) {
        memcpy(
            new_data + new_capacity - after_gap_len,
            buffer->data + buffer->gap_end,
            after_gap_len * sizeof(wchar_t)
        );
    }
    
    /* Update buffer */
    free(buffer->data);
    buffer->data = new_data;
    buffer->gap_end = new_capacity - after_gap_len;
    buffer->capacity = new_capacity;
    
    return QALAM_OK;
}

/**
 * @brief Count newlines in buffer content
 */
static void buffer_update_line_count(QalamBuffer* buffer) {
    size_t count = 1; /* At least one line */
    size_t len = buffer_content_length(buffer);
    
    for (size_t i = 0; i < len; i++) {
        wchar_t ch = buffer_char_at_internal(buffer, i);
        if (ch == L'\n') {
            count++;
        }
    }
    
    buffer->line_count = count;
}

/**
 * @brief Count newlines in a range of buffer content
 */
static size_t buffer_count_newlines_in_range(const QalamBuffer* buffer, size_t start, size_t len) {
    size_t count = 0;
    size_t end = start + len;
    size_t content_len = buffer_content_length(buffer);
    
    if (end > content_len) {
        end = content_len;
    }
    
    for (size_t i = start; i < end; i++) {
        if (buffer_char_at_internal(buffer, i) == L'\n') {
            count++;
        }
    }
    
    return count;
}

/**
 * @brief Update cursor line/column from gap_start position
 */
static void buffer_update_cursor_from_offset(QalamBuffer* buffer) {
    size_t line = 0;
    size_t col = 0;
    
    for (size_t i = 0; i < buffer->gap_start; i++) {
        wchar_t ch = buffer_char_at_internal(buffer, i);
        if (ch == L'\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
    
    buffer->cursor_line = line;
    buffer->cursor_column = col;
}

/**
 * @brief Get offset from line and column
 */
static size_t buffer_offset_from_line_column(const QalamBuffer* buffer, size_t line, size_t column) {
    size_t current_line = 0;
    size_t offset = 0;
    size_t len = buffer_content_length(buffer);
    
    /* Find start of target line */
    while (current_line < line && offset < len) {
        if (buffer_char_at_internal(buffer, offset) == L'\n') {
            current_line++;
        }
        offset++;
    }
    
    /* Move to column */
    size_t col = 0;
    while (col < column && offset < len) {
        wchar_t ch = buffer_char_at_internal(buffer, offset);
        if (ch == L'\n') {
            break; /* Don't go past end of line */
        }
        offset++;
        col++;
    }
    
    return offset;
}

/**
 * @brief Get the offset of the start of a line
 */
static size_t buffer_get_line_start_offset(const QalamBuffer* buffer, size_t line) {
    if (line == 0) {
        return 0;
    }
    
    size_t current_line = 0;
    size_t offset = 0;
    size_t len = buffer_content_length(buffer);
    
    while (offset < len && current_line < line) {
        if (buffer_char_at_internal(buffer, offset) == L'\n') {
            current_line++;
        }
        offset++;
    }
    
    return offset;
}

/**
 * @brief Get the length of a line (excluding newline)
 */
static size_t buffer_get_line_length(const QalamBuffer* buffer, size_t line) {
    size_t start = buffer_get_line_start_offset(buffer, line);
    size_t len = buffer_content_length(buffer);
    size_t line_len = 0;
    
    while (start + line_len < len) {
        wchar_t ch = buffer_char_at_internal(buffer, start + line_len);
        if (ch == L'\n') {
            break;
        }
        line_len++;
    }
    
    return line_len;
}

/**
 * @brief Convert UTF-8 to UTF-16
 * @return Number of wchar_t written, or 0 on error
 */
static int utf8_to_utf16(const char* utf8, size_t utf8_len, wchar_t* utf16, size_t utf16_size) {
    if (!utf8 || utf8_len == 0) {
        return 0;
    }
    
    /* Use Windows API for conversion */
    int result = MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8,
        (int)utf8_len,
        utf16,
        (int)utf16_size
    );
    
    return result;
}

/**
 * @brief Convert UTF-16 to UTF-8
 * @return Number of bytes written, or 0 on error
 */
static int utf16_to_utf8(const wchar_t* utf16, size_t utf16_len, char* utf8, size_t utf8_size) {
    if (!utf16 || utf16_len == 0) {
        return 0;
    }
    
    /* Use Windows API for conversion */
    int result = WideCharToMultiByte(
        CP_UTF8,
        0,
        utf16,
        (int)utf16_len,
        utf8,
        (int)utf8_size,
        NULL,
        NULL
    );
    
    return result;
}

/*=============================================================================
 * Buffer Creation and Destruction
 *============================================================================*/

/**
 * @brief Create a new empty buffer
 */
QalamResult qalam_buffer_create(QalamBuffer** buffer) {
    return qalam_buffer_create_with_capacity(buffer, QALAM_BUFFER_INITIAL_CAPACITY);
}

/**
 * @brief Create a buffer with initial capacity
 */
QalamResult qalam_buffer_create_with_capacity(QalamBuffer** buffer, size_t initial_capacity) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Enforce minimum capacity */
    if (initial_capacity < QALAM_BUFFER_INITIAL_CAPACITY) {
        initial_capacity = QALAM_BUFFER_INITIAL_CAPACITY;
    }
    
    /* Allocate buffer structure */
    QalamBuffer* buf = (QalamBuffer*)calloc(1, sizeof(QalamBuffer));
    if (!buf) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Allocate data array */
    buf->data = (wchar_t*)malloc(initial_capacity * sizeof(wchar_t));
    if (!buf->data) {
        free(buf);
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize gap to span entire buffer */
    buf->capacity = initial_capacity;
    buf->gap_start = 0;
    buf->gap_end = initial_capacity;
    
    /* Initialize state */
    buf->cursor_line = 0;
    buf->cursor_column = 0;
    buf->line_count = 1; /* Empty buffer has one line */
    buf->modified = false;
    buf->readonly = false;
    buf->filepath[0] = L'\0';
    
    /* Initialize selection as inactive */
    buf->selection.is_active = false;
    buf->selection.is_rectangular = false;
    
    *buffer = buf;
    return QALAM_OK;
}

/**
 * @brief Create a buffer from UTF-8 text
 */
QalamResult qalam_buffer_create_from_text(QalamBuffer** buffer, const char* text, size_t length) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!text) {
        return qalam_buffer_create(buffer);
    }
    
    /* Handle null-terminated string */
    if (length == 0) {
        length = strlen(text);
    }
    
    if (length == 0) {
        return qalam_buffer_create(buffer);
    }
    
    /* Calculate required UTF-16 size */
    int utf16_len = MultiByteToWideChar(CP_UTF8, 0, text, (int)length, NULL, 0);
    if (utf16_len <= 0) {
        return QALAM_ERROR_ENCODING;
    }
    
    /* Create buffer with appropriate capacity */
    size_t capacity = (size_t)utf16_len + QALAM_BUFFER_INITIAL_GAP_SIZE;
    QalamResult result = qalam_buffer_create_with_capacity(buffer, capacity);
    if (result != QALAM_OK) {
        return result;
    }
    
    QalamBuffer* buf = *buffer;
    
    /* Convert and insert text */
    int converted = MultiByteToWideChar(
        CP_UTF8, 0,
        text, (int)length,
        buf->data, utf16_len
    );
    
    if (converted <= 0) {
        qalam_buffer_destroy(buf);
        *buffer = NULL;
        return QALAM_ERROR_ENCODING;
    }
    
    /* Adjust gap */
    buf->gap_start = (size_t)converted;
    
    /* Update line count */
    buffer_update_line_count(buf);
    
    return QALAM_OK;
}

/**
 * @brief Create a buffer from a file
 */
QalamResult qalam_buffer_create_from_file(QalamBuffer** buffer, const wchar_t* filepath) {
    if (!buffer || !filepath) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Open file */
    HANDLE hFile = CreateFileW(
        filepath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return QALAM_ERROR_FILE_NOT_FOUND;
    }
    
    /* Get file size */
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return QALAM_ERROR_FILE_READ;
    }
    
    /* Check for reasonable size */
    if (fileSize.QuadPart > 100 * 1024 * 1024) { /* 100 MB limit */
        CloseHandle(hFile);
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Allocate read buffer */
    size_t file_bytes = (size_t)fileSize.QuadPart;
    char* file_data = NULL;
    
    if (file_bytes > 0) {
        file_data = (char*)malloc(file_bytes + 1);
        if (!file_data) {
            CloseHandle(hFile);
            return QALAM_ERROR_OUT_OF_MEMORY;
        }
        
        /* Read file */
        DWORD bytesRead;
        if (!ReadFile(hFile, file_data, (DWORD)file_bytes, &bytesRead, NULL)) {
            free(file_data);
            CloseHandle(hFile);
            return QALAM_ERROR_FILE_READ;
        }
        
        file_data[bytesRead] = '\0';
        file_bytes = bytesRead;
    }
    
    CloseHandle(hFile);
    
    /* Create buffer from text */
    QalamResult result;
    if (file_data && file_bytes > 0) {
        result = qalam_buffer_create_from_text(buffer, file_data, file_bytes);
        free(file_data);
    } else {
        result = qalam_buffer_create(buffer);
    }
    
    if (result == QALAM_OK && *buffer) {
        /* Store filepath */
        wcsncpy((*buffer)->filepath, filepath, MAX_PATH - 1);
        (*buffer)->filepath[MAX_PATH - 1] = L'\0';
        (*buffer)->modified = false;
    }
    
    return result;
}

/**
 * @brief Destroy a buffer and free its resources
 */
void qalam_buffer_destroy(QalamBuffer* buffer) {
    if (!buffer) {
        return;
    }
    
    if (buffer->data) {
        /* Zero out data before freeing (security) */
        memset(buffer->data, 0, buffer->capacity * sizeof(wchar_t));
        free(buffer->data);
    }
    
    memset(buffer, 0, sizeof(QalamBuffer));
    free(buffer);
}

/*=============================================================================
 * Buffer Content Operations
 *============================================================================*/

/**
 * @brief Insert text at current cursor position
 */
QalamResult qalam_buffer_insert(QalamBuffer* buffer, const char* text, size_t length) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!text || length == 0) {
        return QALAM_OK; /* Nothing to insert */
    }
    
    /* Handle null-terminated string */
    if (length == 0) {
        length = strlen(text);
    }
    
    /* Convert UTF-8 to UTF-16 */
    int utf16_len = MultiByteToWideChar(CP_UTF8, 0, text, (int)length, NULL, 0);
    if (utf16_len <= 0) {
        return QALAM_ERROR_ENCODING;
    }
    
    /* Ensure gap is large enough */
    QalamResult result = buffer_ensure_gap_size(buffer, (size_t)utf16_len);
    if (result != QALAM_OK) {
        return result;
    }
    
    /* Convert directly into gap */
    int converted = MultiByteToWideChar(
        CP_UTF8, 0,
        text, (int)length,
        buffer->data + buffer->gap_start, utf16_len
    );
    
    if (converted <= 0) {
        return QALAM_ERROR_ENCODING;
    }
    
    /* Count newlines in inserted text for line count update */
    size_t newlines = 0;
    for (int i = 0; i < converted; i++) {
        if (buffer->data[buffer->gap_start + i] == L'\n') {
            newlines++;
        }
    }
    
    /* Advance gap start */
    buffer->gap_start += converted;
    buffer->line_count += newlines;
    buffer->modified = true;
    
    /* Update cursor position */
    buffer_update_cursor_from_offset(buffer);
    
    return QALAM_OK;
}

/**
 * @brief Insert text at specified position
 */
QalamResult qalam_buffer_insert_at(QalamBuffer* buffer, size_t offset, const char* text, size_t length) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t content_len = buffer_content_length(buffer);
    if (offset > content_len) {
        return QALAM_ERROR_INVALID_POSITION;
    }
    
    /* Move gap to insertion point */
    buffer_move_gap_to(buffer, offset);
    
    /* Insert using standard function */
    return qalam_buffer_insert(buffer, text, length);
}

/**
 * @brief Delete characters at current cursor position
 */
QalamResult qalam_buffer_delete(QalamBuffer* buffer, int count) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (count == 0) {
        return QALAM_OK;
    }
    
    size_t content_len = buffer_content_length(buffer);
    
    if (count > 0) {
        /* Delete forward */
        size_t after_gap = buffer->capacity - buffer->gap_end;
        size_t to_delete = (size_t)count;
        
        if (to_delete > after_gap) {
            to_delete = after_gap;
        }
        
        if (to_delete == 0) {
            return QALAM_OK; /* Nothing after cursor */
        }
        
        /* Count newlines being deleted */
        size_t newlines = 0;
        for (size_t i = 0; i < to_delete; i++) {
            /* Handle surrogate pairs - don't split them */
            wchar_t ch = buffer->data[buffer->gap_end + i];
            if (ch == L'\n') {
                newlines++;
            }
            /* If we hit a low surrogate at position 0, skip it */
            if (i == to_delete - 1 && is_high_surrogate(ch) && 
                buffer->gap_end + i + 1 < buffer->capacity) {
                wchar_t next = buffer->data[buffer->gap_end + i + 1];
                if (is_low_surrogate(next)) {
                    to_delete++; /* Include the low surrogate */
                }
            }
        }
        
        /* Expand gap to consume deleted text */
        buffer->gap_end += to_delete;
        buffer->line_count -= newlines;
        if (buffer->line_count < 1) {
            buffer->line_count = 1;
        }
    } else {
        /* Delete backward (negative count = backspace) */
        size_t to_delete = (size_t)(-count);
        
        if (to_delete > buffer->gap_start) {
            to_delete = buffer->gap_start;
        }
        
        if (to_delete == 0) {
            return QALAM_OK; /* Nothing before cursor */
        }
        
        /* Handle surrogate pairs - don't split them */
        if (to_delete > 0) {
            size_t check_pos = buffer->gap_start - to_delete;
            wchar_t ch = buffer->data[check_pos];
            if (is_low_surrogate(ch) && check_pos > 0) {
                wchar_t prev = buffer->data[check_pos - 1];
                if (is_high_surrogate(prev)) {
                    to_delete++; /* Include the high surrogate */
                }
            }
        }
        
        /* Count newlines being deleted */
        size_t newlines = 0;
        for (size_t i = 0; i < to_delete; i++) {
            if (buffer->data[buffer->gap_start - 1 - i] == L'\n') {
                newlines++;
            }
        }
        
        /* Shrink gap_start to consume deleted text */
        buffer->gap_start -= to_delete;
        buffer->line_count -= newlines;
        if (buffer->line_count < 1) {
            buffer->line_count = 1;
        }
    }
    
    buffer->modified = true;
    buffer_update_cursor_from_offset(buffer);
    
    return QALAM_OK;
}

/**
 * @brief Delete a range of text
 */
QalamResult qalam_buffer_delete_range(QalamBuffer* buffer, size_t start_offset, size_t end_offset) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (start_offset > end_offset) {
        /* Swap */
        size_t temp = start_offset;
        start_offset = end_offset;
        end_offset = temp;
    }
    
    size_t content_len = buffer_content_length(buffer);
    
    if (start_offset > content_len) {
        return QALAM_ERROR_INVALID_RANGE;
    }
    
    if (end_offset > content_len) {
        end_offset = content_len;
    }
    
    size_t delete_len = end_offset - start_offset;
    if (delete_len == 0) {
        return QALAM_OK;
    }
    
    /* Count newlines in range being deleted */
    size_t newlines = buffer_count_newlines_in_range(buffer, start_offset, delete_len);
    
    /* Move gap to start of range, then delete forward */
    buffer_move_gap_to(buffer, start_offset);
    buffer->gap_end += delete_len;
    
    buffer->line_count -= newlines;
    if (buffer->line_count < 1) {
        buffer->line_count = 1;
    }
    
    buffer->modified = true;
    buffer_update_cursor_from_offset(buffer);
    
    return QALAM_OK;
}

/**
 * @brief Replace text in a range
 */
QalamResult qalam_buffer_replace(QalamBuffer* buffer, size_t start_offset, size_t end_offset, 
                                  const char* text, size_t length) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Delete range first */
    QalamResult result = qalam_buffer_delete_range(buffer, start_offset, end_offset);
    if (result != QALAM_OK) {
        return result;
    }
    
    /* Insert new text at start position */
    buffer_move_gap_to(buffer, start_offset);
    return qalam_buffer_insert(buffer, text, length);
}

/*=============================================================================
 * Cursor Operations
 *============================================================================*/

/**
 * @brief Get the current cursor position
 */
QalamResult qalam_buffer_get_cursor(const QalamBuffer* buffer, QalamCursor* cursor) {
    if (!buffer || !cursor) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    cursor->line = buffer->cursor_line;
    cursor->column = buffer->cursor_column;
    cursor->offset = buffer->gap_start;
    cursor->visual_column = buffer->cursor_column; /* TODO: handle tabs/RTL */
    
    return QALAM_OK;
}

/**
 * @brief Set cursor position by line and column
 */
QalamResult qalam_buffer_set_cursor(QalamBuffer* buffer, size_t line, size_t column) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (line >= buffer->line_count) {
        line = buffer->line_count - 1;
    }
    
    /* Get line length to clamp column */
    size_t line_len = buffer_get_line_length(buffer, line);
    if (column > line_len) {
        column = line_len;
    }
    
    /* Calculate offset */
    size_t offset = buffer_offset_from_line_column(buffer, line, column);
    
    /* Move gap to new position */
    buffer_move_gap_to(buffer, offset);
    
    buffer->cursor_line = line;
    buffer->cursor_column = column;
    
    return QALAM_OK;
}

/**
 * @brief Set cursor position by byte offset
 */
QalamResult qalam_buffer_set_cursor_offset(QalamBuffer* buffer, size_t offset) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t content_len = buffer_content_length(buffer);
    if (offset > content_len) {
        offset = content_len;
    }
    
    /* Handle surrogate pairs - don't land in the middle */
    if (offset > 0 && offset < content_len) {
        wchar_t ch = buffer_char_at_internal(buffer, offset);
        if (is_low_surrogate(ch)) {
            offset--; /* Move to high surrogate */
        }
    }
    
    buffer_move_gap_to(buffer, offset);
    buffer_update_cursor_from_offset(buffer);
    
    return QALAM_OK;
}

/**
 * @brief Move cursor relative to current position
 */
QalamResult qalam_buffer_move_cursor(QalamBuffer* buffer, int delta_line, int delta_column) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Calculate new line */
    size_t new_line = buffer->cursor_line;
    if (delta_line < 0) {
        size_t abs_delta = (size_t)(-delta_line);
        if (abs_delta > new_line) {
            new_line = 0;
        } else {
            new_line -= abs_delta;
        }
    } else {
        new_line += (size_t)delta_line;
        if (new_line >= buffer->line_count) {
            new_line = buffer->line_count - 1;
        }
    }
    
    /* Calculate new column */
    size_t new_column = buffer->cursor_column;
    if (delta_column < 0) {
        size_t abs_delta = (size_t)(-delta_column);
        if (abs_delta > new_column) {
            new_column = 0;
        } else {
            new_column -= abs_delta;
        }
    } else {
        new_column += (size_t)delta_column;
    }
    
    return qalam_buffer_set_cursor(buffer, new_line, new_column);
}

/**
 * @brief Move cursor to start of buffer
 */
QalamResult qalam_buffer_cursor_to_start(QalamBuffer* buffer) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    buffer_move_gap_to(buffer, 0);
    buffer->cursor_line = 0;
    buffer->cursor_column = 0;
    
    return QALAM_OK;
}

/**
 * @brief Move cursor to end of buffer
 */
QalamResult qalam_buffer_cursor_to_end(QalamBuffer* buffer) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t content_len = buffer_content_length(buffer);
    buffer_move_gap_to(buffer, content_len);
    buffer_update_cursor_from_offset(buffer);
    
    return QALAM_OK;
}

/**
 * @brief Move cursor to start of current line
 */
QalamResult qalam_buffer_cursor_to_line_start(QalamBuffer* buffer) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t line_start = buffer_get_line_start_offset(buffer, buffer->cursor_line);
    buffer_move_gap_to(buffer, line_start);
    buffer->cursor_column = 0;
    
    return QALAM_OK;
}

/**
 * @brief Move cursor to end of current line
 */
QalamResult qalam_buffer_cursor_to_line_end(QalamBuffer* buffer) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t line_start = buffer_get_line_start_offset(buffer, buffer->cursor_line);
    size_t line_len = buffer_get_line_length(buffer, buffer->cursor_line);
    
    buffer_move_gap_to(buffer, line_start + line_len);
    buffer->cursor_column = line_len;
    
    return QALAM_OK;
}

/*=============================================================================
 * Content Retrieval
 *============================================================================*/

/**
 * @brief Get a single line from the buffer
 */
QalamResult qalam_buffer_get_line(const QalamBuffer* buffer, size_t line_number,
                                   char* out_text, size_t out_size, size_t* bytes_written) {
    if (!buffer || !out_text || out_size == 0) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (line_number >= buffer->line_count) {
        return QALAM_ERROR_INVALID_RANGE;
    }
    
    /* Get line bounds */
    size_t line_start = buffer_get_line_start_offset(buffer, line_number);
    size_t line_len = buffer_get_line_length(buffer, line_number);
    
    if (line_len == 0) {
        out_text[0] = '\0';
        if (bytes_written) {
            *bytes_written = 0;
        }
        return QALAM_OK;
    }
    
    /* Allocate temporary buffer for UTF-16 content */
    wchar_t* temp = (wchar_t*)malloc((line_len + 1) * sizeof(wchar_t));
    if (!temp) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy line content */
    for (size_t i = 0; i < line_len; i++) {
        temp[i] = buffer_char_at_internal(buffer, line_start + i);
    }
    temp[line_len] = L'\0';
    
    /* Convert to UTF-8 */
    int utf8_len = utf16_to_utf8(temp, line_len, out_text, out_size - 1);
    free(temp);
    
    if (utf8_len <= 0) {
        out_text[0] = '\0';
        if (bytes_written) {
            *bytes_written = 0;
        }
        return QALAM_ERROR_ENCODING;
    }
    
    out_text[utf8_len] = '\0';
    if (bytes_written) {
        *bytes_written = (size_t)utf8_len;
    }
    
    return QALAM_OK;
}

/**
 * @brief Get line information
 */
QalamResult qalam_buffer_get_line_info(const QalamBuffer* buffer, size_t line_number,
                                        QalamLineInfo* info) {
    if (!buffer || !info) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (line_number >= buffer->line_count) {
        return QALAM_ERROR_INVALID_RANGE;
    }
    
    size_t line_start = buffer_get_line_start_offset(buffer, line_number);
    size_t line_len = buffer_get_line_length(buffer, line_number);
    
    info->line_number = line_number;
    info->start_offset = line_start;
    info->length_chars = line_len;
    
    /* Calculate byte length (UTF-8) */
    if (line_len > 0) {
        wchar_t* temp = (wchar_t*)malloc((line_len + 1) * sizeof(wchar_t));
        if (temp) {
            for (size_t i = 0; i < line_len; i++) {
                temp[i] = buffer_char_at_internal(buffer, line_start + i);
            }
            info->length_bytes = WideCharToMultiByte(CP_UTF8, 0, temp, (int)line_len, NULL, 0, NULL, NULL);
            free(temp);
        } else {
            info->length_bytes = line_len; /* Fallback */
        }
    } else {
        info->length_bytes = 0;
    }
    
    /* Check for RTL characters */
    info->has_rtl_chars = false;
    info->has_ltr_chars = false;
    info->direction = QALAM_DIR_AUTO;
    
    for (size_t i = 0; i < line_len; i++) {
        wchar_t ch = buffer_char_at_internal(buffer, line_start + i);
        
        /* Check for Arabic range */
        if ((ch >= 0x0600 && ch <= 0x06FF) ||  /* Arabic */
            (ch >= 0x0750 && ch <= 0x077F) ||  /* Arabic Supplement */
            (ch >= 0x08A0 && ch <= 0x08FF) ||  /* Arabic Extended-A */
            (ch >= 0xFB50 && ch <= 0xFDFF) ||  /* Arabic Presentation Forms-A */
            (ch >= 0xFE70 && ch <= 0xFEFF) ||  /* Arabic Presentation Forms-B */
            (ch >= 0x0590 && ch <= 0x05FF)) {  /* Hebrew */
            info->has_rtl_chars = true;
        }
        
        /* Check for Latin range */
        if ((ch >= 0x0041 && ch <= 0x005A) ||  /* A-Z */
            (ch >= 0x0061 && ch <= 0x007A)) {  /* a-z */
            info->has_ltr_chars = true;
        }
    }
    
    /* Determine direction */
    if (info->has_rtl_chars && !info->has_ltr_chars) {
        info->direction = QALAM_DIR_RTL;
    } else if (info->has_ltr_chars && !info->has_rtl_chars) {
        info->direction = QALAM_DIR_LTR;
    } else {
        info->direction = QALAM_DIR_AUTO;
    }
    
    return QALAM_OK;
}

/**
 * @brief Get entire buffer content
 */
QalamResult qalam_buffer_get_content(const QalamBuffer* buffer, char* out_text, 
                                      size_t out_size, size_t* bytes_written) {
    if (!buffer || !out_text || out_size == 0) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t content_len = buffer_content_length(buffer);
    
    if (content_len == 0) {
        out_text[0] = '\0';
        if (bytes_written) {
            *bytes_written = 0;
        }
        return QALAM_OK;
    }
    
    /* Allocate temporary buffer for UTF-16 content */
    wchar_t* temp = (wchar_t*)malloc((content_len + 1) * sizeof(wchar_t));
    if (!temp) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy content (handle gap) */
    size_t pos = 0;
    for (size_t i = 0; i < content_len; i++) {
        temp[pos++] = buffer_char_at_internal(buffer, i);
    }
    temp[pos] = L'\0';
    
    /* Convert to UTF-8 */
    int utf8_len = utf16_to_utf8(temp, content_len, out_text, out_size - 1);
    free(temp);
    
    if (utf8_len <= 0 && content_len > 0) {
        out_text[0] = '\0';
        if (bytes_written) {
            *bytes_written = 0;
        }
        return QALAM_ERROR_ENCODING;
    }
    
    out_text[utf8_len] = '\0';
    if (bytes_written) {
        *bytes_written = (size_t)utf8_len;
    }
    
    return QALAM_OK;
}

/**
 * @brief Get a range of content
 */
QalamResult qalam_buffer_get_range(const QalamBuffer* buffer, size_t start_offset,
                                    size_t end_offset, char* out_text, size_t out_size,
                                    size_t* bytes_written) {
    if (!buffer || !out_text || out_size == 0) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t content_len = buffer_content_length(buffer);
    
    if (start_offset > content_len || end_offset > content_len) {
        return QALAM_ERROR_INVALID_RANGE;
    }
    
    if (start_offset > end_offset) {
        size_t temp = start_offset;
        start_offset = end_offset;
        end_offset = temp;
    }
    
    size_t range_len = end_offset - start_offset;
    
    if (range_len == 0) {
        out_text[0] = '\0';
        if (bytes_written) {
            *bytes_written = 0;
        }
        return QALAM_OK;
    }
    
    /* Allocate temporary buffer */
    wchar_t* temp = (wchar_t*)malloc((range_len + 1) * sizeof(wchar_t));
    if (!temp) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy range */
    for (size_t i = 0; i < range_len; i++) {
        temp[i] = buffer_char_at_internal(buffer, start_offset + i);
    }
    temp[range_len] = L'\0';
    
    /* Convert to UTF-8 */
    int utf8_len = utf16_to_utf8(temp, range_len, out_text, out_size - 1);
    free(temp);
    
    if (utf8_len <= 0 && range_len > 0) {
        out_text[0] = '\0';
        if (bytes_written) {
            *bytes_written = 0;
        }
        return QALAM_ERROR_ENCODING;
    }
    
    out_text[utf8_len] = '\0';
    if (bytes_written) {
        *bytes_written = (size_t)utf8_len;
    }
    
    return QALAM_OK;
}

/*=============================================================================
 * Buffer Statistics and Queries
 *============================================================================*/

/**
 * @brief Get buffer statistics
 */
QalamResult qalam_buffer_get_stats(const QalamBuffer* buffer, QalamBufferStats* stats) {
    if (!buffer || !stats) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t content_len = buffer_content_length(buffer);
    
    /* Calculate byte size (UTF-8) */
    size_t byte_size = 0;
    if (content_len > 0) {
        wchar_t* temp = (wchar_t*)malloc((content_len + 1) * sizeof(wchar_t));
        if (temp) {
            for (size_t i = 0; i < content_len; i++) {
                temp[i] = buffer_char_at_internal(buffer, i);
            }
            byte_size = WideCharToMultiByte(CP_UTF8, 0, temp, (int)content_len, NULL, 0, NULL, NULL);
            free(temp);
        }
    }
    
    stats->total_bytes = byte_size;
    stats->total_chars = content_len;
    stats->total_lines = buffer->line_count;
    stats->gap_size = buffer_gap_size(buffer);
    stats->capacity = buffer->capacity;
    stats->is_modified = buffer->modified;
    stats->is_readonly = buffer->readonly;
    
    return QALAM_OK;
}

/**
 * @brief Get total number of lines
 */
size_t qalam_buffer_get_line_count(const QalamBuffer* buffer) {
    if (!buffer) {
        return 0;
    }
    return buffer->line_count;
}

/**
 * @brief Get total byte size
 */
size_t qalam_buffer_get_size(const QalamBuffer* buffer) {
    if (!buffer) {
        return 0;
    }
    
    size_t content_len = buffer_content_length(buffer);
    if (content_len == 0) {
        return 0;
    }
    
    /* Calculate UTF-8 byte size */
    wchar_t* temp = (wchar_t*)malloc((content_len + 1) * sizeof(wchar_t));
    if (!temp) {
        return content_len; /* Fallback to character count */
    }
    
    for (size_t i = 0; i < content_len; i++) {
        temp[i] = buffer_char_at_internal(buffer, i);
    }
    
    int byte_size = WideCharToMultiByte(CP_UTF8, 0, temp, (int)content_len, NULL, 0, NULL, NULL);
    free(temp);
    
    return byte_size > 0 ? (size_t)byte_size : content_len;
}

/**
 * @brief Check if buffer has been modified
 */
bool qalam_buffer_is_modified(const QalamBuffer* buffer) {
    if (!buffer) {
        return false;
    }
    return buffer->modified;
}

/**
 * @brief Mark buffer as unmodified
 */
void qalam_buffer_clear_modified(QalamBuffer* buffer) {
    if (buffer) {
        buffer->modified = false;
    }
}

/*=============================================================================
 * File Operations
 *============================================================================*/

/**
 * @brief Save buffer to file
 */
QalamResult qalam_buffer_save(QalamBuffer* buffer, const wchar_t* filepath) {
    if (!buffer || !filepath) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    size_t content_len = buffer_content_length(buffer);
    
    /* Calculate UTF-8 size */
    size_t utf8_size = 0;
    wchar_t* content = NULL;
    char* utf8_content = NULL;
    
    if (content_len > 0) {
        content = (wchar_t*)malloc((content_len + 1) * sizeof(wchar_t));
        if (!content) {
            return QALAM_ERROR_OUT_OF_MEMORY;
        }
        
        for (size_t i = 0; i < content_len; i++) {
            content[i] = buffer_char_at_internal(buffer, i);
        }
        content[content_len] = L'\0';
        
        utf8_size = WideCharToMultiByte(CP_UTF8, 0, content, (int)content_len, NULL, 0, NULL, NULL);
        if (utf8_size > 0) {
            utf8_content = (char*)malloc(utf8_size + 1);
            if (!utf8_content) {
                free(content);
                return QALAM_ERROR_OUT_OF_MEMORY;
            }
            
            WideCharToMultiByte(CP_UTF8, 0, content, (int)content_len, utf8_content, (int)utf8_size, NULL, NULL);
            utf8_content[utf8_size] = '\0';
        }
        
        free(content);
    }
    
    /* Open file for writing */
    HANDLE hFile = CreateFileW(
        filepath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        if (utf8_content) free(utf8_content);
        return QALAM_ERROR_FILE_ACCESS;
    }
    
    /* Write content */
    QalamResult result = QALAM_OK;
    if (utf8_content && utf8_size > 0) {
        DWORD written;
        if (!WriteFile(hFile, utf8_content, (DWORD)utf8_size, &written, NULL)) {
            result = QALAM_ERROR_FILE_WRITE;
        }
    }
    
    CloseHandle(hFile);
    if (utf8_content) free(utf8_content);
    
    if (result == QALAM_OK) {
        wcsncpy(buffer->filepath, filepath, MAX_PATH - 1);
        buffer->filepath[MAX_PATH - 1] = L'\0';
        buffer->modified = false;
    }
    
    return result;
}

/**
 * @brief Load file into buffer (replaces content)
 */
QalamResult qalam_buffer_load(QalamBuffer* buffer, const wchar_t* filepath) {
    if (!buffer || !filepath) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Create temporary buffer from file */
    QalamBuffer* temp_buf = NULL;
    QalamResult result = qalam_buffer_create_from_file(&temp_buf, filepath);
    
    if (result != QALAM_OK) {
        return result;
    }
    
    /* Swap contents */
    wchar_t* old_data = buffer->data;
    
    buffer->data = temp_buf->data;
    buffer->capacity = temp_buf->capacity;
    buffer->gap_start = temp_buf->gap_start;
    buffer->gap_end = temp_buf->gap_end;
    buffer->cursor_line = 0;
    buffer->cursor_column = 0;
    buffer->line_count = temp_buf->line_count;
    buffer->modified = false;
    wcsncpy(buffer->filepath, filepath, MAX_PATH - 1);
    buffer->filepath[MAX_PATH - 1] = L'\0';
    
    /* Free old data and temp structure */
    free(old_data);
    temp_buf->data = NULL;
    qalam_buffer_destroy(temp_buf);
    
    return QALAM_OK;
}

/*=============================================================================
 * Selection Operations
 *============================================================================*/

/**
 * @brief Get current selection
 */
QalamResult qalam_buffer_get_selection(const QalamBuffer* buffer, QalamSelection* selection) {
    if (!buffer || !selection) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    *selection = buffer->selection;
    return QALAM_OK;
}

/**
 * @brief Set selection range
 */
QalamResult qalam_buffer_set_selection(QalamBuffer* buffer, 
                                        size_t start_line, size_t start_column,
                                        size_t end_line, size_t end_column) {
    if (!buffer) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Validate and clamp positions */
    if (start_line >= buffer->line_count) {
        start_line = buffer->line_count - 1;
    }
    if (end_line >= buffer->line_count) {
        end_line = buffer->line_count - 1;
    }
    
    size_t start_line_len = buffer_get_line_length(buffer, start_line);
    size_t end_line_len = buffer_get_line_length(buffer, end_line);
    
    if (start_column > start_line_len) {
        start_column = start_line_len;
    }
    if (end_column > end_line_len) {
        end_column = end_line_len;
    }
    
    /* Calculate offsets */
    size_t start_offset = buffer_offset_from_line_column(buffer, start_line, start_column);
    size_t end_offset = buffer_offset_from_line_column(buffer, end_line, end_column);
    
    buffer->selection.start.line = start_line;
    buffer->selection.start.column = start_column;
    buffer->selection.start.offset = start_offset;
    buffer->selection.start.visual_column = start_column;
    
    buffer->selection.end.line = end_line;
    buffer->selection.end.column = end_column;
    buffer->selection.end.offset = end_offset;
    buffer->selection.end.visual_column = end_column;
    
    buffer->selection.is_active = true;
    buffer->selection.is_rectangular = false;
    
    return QALAM_OK;
}

/**
 * @brief Clear selection
 */
void qalam_buffer_clear_selection(QalamBuffer* buffer) {
    if (buffer) {
        buffer->selection.is_active = false;
        memset(&buffer->selection.start, 0, sizeof(QalamCursor));
        memset(&buffer->selection.end, 0, sizeof(QalamCursor));
    }
}

/**
 * @brief Get selected text
 */
QalamResult qalam_buffer_get_selected_text(const QalamBuffer* buffer, char* out_text,
                                            size_t out_size, size_t* bytes_written) {
    if (!buffer || !out_text || out_size == 0) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!buffer->selection.is_active) {
        out_text[0] = '\0';
        if (bytes_written) {
            *bytes_written = 0;
        }
        return QALAM_OK;
    }
    
    size_t start = buffer->selection.start.offset;
    size_t end = buffer->selection.end.offset;
    
    /* Ensure start <= end */
    if (start > end) {
        size_t temp = start;
        start = end;
        end = temp;
    }
    
    return qalam_buffer_get_range(buffer, start, end, out_text, out_size, bytes_written);
}