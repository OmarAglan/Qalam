/**
 * @file editor.h
 * @brief Qalam IDE - Editor and Buffer Interface
 * 
 * Defines the text buffer (gap buffer implementation) and cursor
 * management APIs for the Qalam editor core.
 * 
 * @version 0.1.0-dev
 * @copyright (c) 2026 Qalam Project
 */

#ifndef QALAM_EDITOR_H
#define QALAM_EDITOR_H

#include "qalam.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Buffer Types and Structures
 *============================================================================*/

/**
 * @brief Cursor position within a buffer
 * 
 * Represents a logical position in the text as line and column.
 * All values are 0-based indices.
 */
typedef struct QalamCursor {
    size_t line;                    /**< Line number (0-based) */
    size_t column;                  /**< Column/character offset (0-based) */
    size_t offset;                  /**< Absolute byte offset in buffer */
    size_t visual_column;           /**< Visual column (accounts for tabs, RTL) */
} QalamCursor;

/**
 * @brief Text selection range
 */
typedef struct QalamSelection {
    QalamCursor start;              /**< Selection start position */
    QalamCursor end;                /**< Selection end position */
    bool is_active;                 /**< Whether selection is active */
    bool is_rectangular;            /**< Block/rectangular selection mode */
} QalamSelection;

/**
 * @brief Buffer statistics
 */
typedef struct QalamBufferStats {
    size_t total_bytes;             /**< Total bytes in buffer */
    size_t total_chars;             /**< Total Unicode characters */
    size_t total_lines;             /**< Total number of lines */
    size_t gap_size;                /**< Current gap size (internal) */
    size_t capacity;                /**< Total buffer capacity */
    bool is_modified;               /**< Buffer has unsaved changes */
    bool is_readonly;               /**< Buffer is read-only */
} QalamBufferStats;

/**
 * @brief Text direction hint for a line
 */
typedef enum QalamTextDirection {
    QALAM_DIR_AUTO = 0,             /**< Auto-detect from content */
    QALAM_DIR_LTR,                  /**< Left-to-right */
    QALAM_DIR_RTL,                  /**< Right-to-left (Arabic, Hebrew) */
} QalamTextDirection;

/**
 * @brief Line information structure
 */
typedef struct QalamLineInfo {
    size_t line_number;             /**< Line number (0-based) */
    size_t start_offset;            /**< Byte offset of line start */
    size_t length_bytes;            /**< Length in bytes (excluding newline) */
    size_t length_chars;            /**< Length in characters */
    QalamTextDirection direction;   /**< Detected text direction */
    bool has_rtl_chars;             /**< Contains RTL characters */
    bool has_ltr_chars;             /**< Contains LTR characters */
} QalamLineInfo;

/*=============================================================================
 * Buffer Creation and Destruction
 *============================================================================*/

/**
 * @brief Create a new empty buffer
 * 
 * @param[out] buffer Pointer to receive the new buffer handle
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_create(QalamBuffer** buffer);

/**
 * @brief Create a buffer with initial capacity
 * 
 * @param[out] buffer Pointer to receive the new buffer handle
 * @param initial_capacity Initial buffer capacity in bytes
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_create_with_capacity(QalamBuffer** buffer, size_t initial_capacity);

/**
 * @brief Create a buffer from UTF-8 text
 * 
 * @param[out] buffer Pointer to receive the new buffer handle
 * @param text UTF-8 encoded text to initialize with
 * @param length Length of text in bytes, or 0 for null-terminated
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_create_from_text(QalamBuffer** buffer, const char* text, size_t length);

/**
 * @brief Create a buffer from a file
 * 
 * @param[out] buffer Pointer to receive the new buffer handle
 * @param filepath Path to the file (UTF-8 or wide string)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_create_from_file(QalamBuffer** buffer, const wchar_t* filepath);

/**
 * @brief Destroy a buffer and free its resources
 * 
 * @param buffer Buffer to destroy (may be NULL)
 */
void qalam_buffer_destroy(QalamBuffer* buffer);

/*=============================================================================
 * Buffer Content Operations
 *============================================================================*/

/**
 * @brief Insert text at current cursor position
 * 
 * @param buffer Target buffer
 * @param text UTF-8 text to insert
 * @param length Length in bytes, or 0 for null-terminated
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_insert(QalamBuffer* buffer, const char* text, size_t length);

/**
 * @brief Insert text at specified position
 * 
 * @param buffer Target buffer
 * @param offset Byte offset for insertion
 * @param text UTF-8 text to insert
 * @param length Length in bytes, or 0 for null-terminated
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_insert_at(QalamBuffer* buffer, size_t offset, const char* text, size_t length);

/**
 * @brief Delete characters at current cursor position
 * 
 * @param buffer Target buffer
 * @param count Number of characters to delete (positive = forward, negative = backward)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_delete(QalamBuffer* buffer, int count);

/**
 * @brief Delete a range of text
 * 
 * @param buffer Target buffer
 * @param start_offset Start byte offset
 * @param end_offset End byte offset
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_delete_range(QalamBuffer* buffer, size_t start_offset, size_t end_offset);

/**
 * @brief Replace text in a range
 * 
 * @param buffer Target buffer
 * @param start_offset Start byte offset
 * @param end_offset End byte offset
 * @param text Replacement text (UTF-8)
 * @param length Length of replacement text
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_replace(QalamBuffer* buffer, size_t start_offset, size_t end_offset, 
                                  const char* text, size_t length);

/*=============================================================================
 * Cursor Operations
 *============================================================================*/

/**
 * @brief Get the current cursor position
 * 
 * @param buffer Target buffer
 * @param[out] cursor Pointer to receive cursor position
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_get_cursor(const QalamBuffer* buffer, QalamCursor* cursor);

/**
 * @brief Set cursor position by line and column
 * 
 * @param buffer Target buffer
 * @param line Line number (0-based)
 * @param column Column number (0-based)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_set_cursor(QalamBuffer* buffer, size_t line, size_t column);

/**
 * @brief Set cursor position by byte offset
 * 
 * @param buffer Target buffer
 * @param offset Byte offset in buffer
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_set_cursor_offset(QalamBuffer* buffer, size_t offset);

/**
 * @brief Move cursor relative to current position
 * 
 * @param buffer Target buffer
 * @param delta_line Lines to move (positive = down, negative = up)
 * @param delta_column Columns to move (positive = right, negative = left)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_move_cursor(QalamBuffer* buffer, int delta_line, int delta_column);

/**
 * @brief Move cursor to start of buffer
 * 
 * @param buffer Target buffer
 * @return QALAM_OK on success
 */
QalamResult qalam_buffer_cursor_to_start(QalamBuffer* buffer);

/**
 * @brief Move cursor to end of buffer
 * 
 * @param buffer Target buffer
 * @return QALAM_OK on success
 */
QalamResult qalam_buffer_cursor_to_end(QalamBuffer* buffer);

/**
 * @brief Move cursor to start of current line
 * 
 * @param buffer Target buffer
 * @return QALAM_OK on success
 */
QalamResult qalam_buffer_cursor_to_line_start(QalamBuffer* buffer);

/**
 * @brief Move cursor to end of current line
 * 
 * @param buffer Target buffer
 * @return QALAM_OK on success
 */
QalamResult qalam_buffer_cursor_to_line_end(QalamBuffer* buffer);

/*=============================================================================
 * Content Retrieval
 *============================================================================*/

/**
 * @brief Get a single line from the buffer
 * 
 * @param buffer Source buffer
 * @param line_number Line number (0-based)
 * @param[out] out_text Buffer to receive line text (UTF-8)
 * @param out_size Size of output buffer
 * @param[out] bytes_written Actual bytes written (optional)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_get_line(const QalamBuffer* buffer, size_t line_number,
                                   char* out_text, size_t out_size, size_t* bytes_written);

/**
 * @brief Get line information
 * 
 * @param buffer Source buffer
 * @param line_number Line number (0-based)
 * @param[out] info Pointer to receive line info
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_get_line_info(const QalamBuffer* buffer, size_t line_number,
                                        QalamLineInfo* info);

/**
 * @brief Get entire buffer content
 * 
 * @param buffer Source buffer
 * @param[out] out_text Buffer to receive content (UTF-8)
 * @param out_size Size of output buffer
 * @param[out] bytes_written Actual bytes written (optional)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_get_content(const QalamBuffer* buffer, char* out_text, 
                                      size_t out_size, size_t* bytes_written);

/**
 * @brief Get a range of content
 * 
 * @param buffer Source buffer
 * @param start_offset Start byte offset
 * @param end_offset End byte offset
 * @param[out] out_text Buffer to receive content
 * @param out_size Size of output buffer
 * @param[out] bytes_written Actual bytes written (optional)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_get_range(const QalamBuffer* buffer, size_t start_offset,
                                    size_t end_offset, char* out_text, size_t out_size,
                                    size_t* bytes_written);

/*=============================================================================
 * Buffer Statistics and Queries
 *============================================================================*/

/**
 * @brief Get buffer statistics
 * 
 * @param buffer Source buffer
 * @param[out] stats Pointer to receive statistics
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_get_stats(const QalamBuffer* buffer, QalamBufferStats* stats);

/**
 * @brief Get total number of lines
 * 
 * @param buffer Source buffer
 * @return Number of lines, or 0 on error
 */
size_t qalam_buffer_get_line_count(const QalamBuffer* buffer);

/**
 * @brief Get total byte size
 * 
 * @param buffer Source buffer
 * @return Size in bytes, or 0 on error
 */
size_t qalam_buffer_get_size(const QalamBuffer* buffer);

/**
 * @brief Check if buffer has been modified
 * 
 * @param buffer Source buffer
 * @return true if modified since last save/load
 */
bool qalam_buffer_is_modified(const QalamBuffer* buffer);

/**
 * @brief Mark buffer as unmodified
 * 
 * @param buffer Target buffer
 */
void qalam_buffer_clear_modified(QalamBuffer* buffer);

/*=============================================================================
 * File Operations
 *============================================================================*/

/**
 * @brief Save buffer to file
 * 
 * @param buffer Source buffer
 * @param filepath Path to save to
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_save(QalamBuffer* buffer, const wchar_t* filepath);

/**
 * @brief Load file into buffer (replaces content)
 * 
 * @param buffer Target buffer
 * @param filepath Path to load from
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_buffer_load(QalamBuffer* buffer, const wchar_t* filepath);

/*=============================================================================
 * Selection Operations
 *============================================================================*/

/**
 * @brief Get current selection
 * 
 * @param buffer Source buffer
 * @param[out] selection Pointer to receive selection
 * @return QALAM_OK on success
 */
QalamResult qalam_buffer_get_selection(const QalamBuffer* buffer, QalamSelection* selection);

/**
 * @brief Set selection range
 * 
 * @param buffer Target buffer
 * @param start_line Start line
 * @param start_column Start column
 * @param end_line End line
 * @param end_column End column
 * @return QALAM_OK on success
 */
QalamResult qalam_buffer_set_selection(QalamBuffer* buffer, 
                                        size_t start_line, size_t start_column,
                                        size_t end_line, size_t end_column);

/**
 * @brief Clear selection
 * 
 * @param buffer Target buffer
 */
void qalam_buffer_clear_selection(QalamBuffer* buffer);

/**
 * @brief Get selected text
 * 
 * @param buffer Source buffer
 * @param[out] out_text Buffer to receive selected text
 * @param out_size Size of output buffer
 * @param[out] bytes_written Actual bytes written
 * @return QALAM_OK on success
 */
QalamResult qalam_buffer_get_selected_text(const QalamBuffer* buffer, char* out_text,
                                            size_t out_size, size_t* bytes_written);

#ifdef __cplusplus
}
#endif

#endif /* QALAM_EDITOR_H */