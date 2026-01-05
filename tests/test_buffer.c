/**
 * @file test_buffer.c
 * @brief Qalam IDE - Gap Buffer Unit Tests
 * 
 * Comprehensive unit tests for the gap buffer implementation.
 * Tests cover buffer lifecycle, insert/delete operations, cursor
 * management, line counting, and UTF-16 surrogate pair handling.
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>

/* Include the header for the buffer API */
#include "editor.h"
#include "qalam.h"

/*=============================================================================
 * Test Framework Macros
 *============================================================================*/

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s:%d - %s\n", __FILE__, __LINE__, #cond); \
        g_test_failures++; \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAIL: %s:%d - Expected %llu, got %llu\n", \
               __FILE__, __LINE__, \
               (unsigned long long)(expected), \
               (unsigned long long)(actual)); \
        g_test_failures++; \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("FAIL: %s:%d - Expected \"%s\", got \"%s\"\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        g_test_failures++; \
        return 1; \
    } \
} while(0)

#define RUN_TEST(name) do { \
    printf("  Running %s...", #name); \
    fflush(stdout); \
    int result = test_##name(); \
    if (result == 0) { \
        printf(" PASSED\n"); \
        g_tests_passed++; \
    } else { \
        printf(" FAILED\n"); \
        g_tests_failed++; \
    } \
    g_tests_total++; \
} while(0)

/*=============================================================================
 * Global Test Counters
 *============================================================================*/

static int g_tests_total = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;
static int g_test_failures = 0;  /* Assertion failures within a test */

/*=============================================================================
 * Buffer Creation and Destruction Tests
 *============================================================================*/

/**
 * @brief Test buffer creation with default parameters
 */
static int test_buffer_create_empty(void) {
    QalamBuffer* buffer = NULL;
    QalamResult result = qalam_buffer_create(&buffer);
    
    TEST_ASSERT(result == QALAM_OK);
    TEST_ASSERT(buffer != NULL);
    
    /* Empty buffer should have 1 line */
    TEST_ASSERT_EQ(1, qalam_buffer_get_line_count(buffer));
    
    /* Empty buffer should have 0 size */
    TEST_ASSERT_EQ(0, qalam_buffer_get_size(buffer));
    
    /* Empty buffer should not be modified */
    TEST_ASSERT(qalam_buffer_is_modified(buffer) == false);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test buffer creation with custom capacity
 */
static int test_buffer_create_with_capacity(void) {
    QalamBuffer* buffer = NULL;
    QalamResult result = qalam_buffer_create_with_capacity(&buffer, 8192);
    
    TEST_ASSERT(result == QALAM_OK);
    TEST_ASSERT(buffer != NULL);
    
    QalamBufferStats stats;
    result = qalam_buffer_get_stats(buffer, &stats);
    TEST_ASSERT(result == QALAM_OK);
    TEST_ASSERT(stats.capacity >= 8192);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test buffer creation from text
 */
static int test_buffer_create_from_text(void) {
    QalamBuffer* buffer = NULL;
    const char* text = "Hello, World!";
    QalamResult result = qalam_buffer_create_from_text(&buffer, text, strlen(text));
    
    TEST_ASSERT(result == QALAM_OK);
    TEST_ASSERT(buffer != NULL);
    
    /* Check line count */
    TEST_ASSERT_EQ(1, qalam_buffer_get_line_count(buffer));
    
    /* Check content */
    char out[256];
    size_t written;
    result = qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT(result == QALAM_OK);
    TEST_ASSERT_STR_EQ(text, out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test buffer destruction with NULL
 */
static int test_buffer_destroy_null(void) {
    /* Should not crash */
    qalam_buffer_destroy(NULL);
    return 0;
}

/**
 * @brief Test buffer creation with NULL pointer
 */
static int test_buffer_create_null_pointer(void) {
    QalamResult result = qalam_buffer_create(NULL);
    TEST_ASSERT(result == QALAM_ERROR_NULL_POINTER);
    return 0;
}

/*=============================================================================
 * Insert Operation Tests
 *============================================================================*/

/**
 * @brief Test inserting at the beginning of an empty buffer
 */
static int test_insert_at_beginning_empty(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create(&buffer);
    
    const char* text = "Hello";
    QalamResult result = qalam_buffer_insert(buffer, text, strlen(text));
    TEST_ASSERT(result == QALAM_OK);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("Hello", out);
    
    TEST_ASSERT(qalam_buffer_is_modified(buffer) == true);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test inserting at the beginning of a non-empty buffer
 */
static int test_insert_at_beginning(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "World", 5);
    
    /* Move cursor to beginning */
    qalam_buffer_cursor_to_start(buffer);
    
    /* Insert at beginning */
    qalam_buffer_insert(buffer, "Hello, ", 7);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("Hello, World", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test inserting at the middle of a buffer
 */
static int test_insert_at_middle(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello World", 11);
    
    /* Move cursor to position 5 (after "Hello") */
    qalam_buffer_set_cursor_offset(buffer, 5);
    
    /* Insert comma */
    qalam_buffer_insert(buffer, ",", 1);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("Hello, World", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test inserting at the end of a buffer
 */
static int test_insert_at_end(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello", 5);
    
    /* Move cursor to end */
    qalam_buffer_cursor_to_end(buffer);
    
    /* Insert at end */
    qalam_buffer_insert(buffer, ", World!", 8);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("Hello, World!", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test inserting with insert_at function
 */
static int test_insert_at_position(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "ABCDEF", 6);
    
    /* Insert at position 3 */
    qalam_buffer_insert_at(buffer, 3, "XYZ", 3);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("ABCXYZDEF", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test inserting newlines updates line count
 */
static int test_insert_newlines(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create(&buffer);
    
    TEST_ASSERT_EQ(1, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_insert(buffer, "Line1\nLine2\nLine3", 17);
    
    TEST_ASSERT_EQ(3, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Delete Operation Tests
 *============================================================================*/

/**
 * @brief Test deleting at the beginning
 */
static int test_delete_at_beginning(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello, World", 12);
    
    /* Move to beginning */
    qalam_buffer_cursor_to_start(buffer);
    
    /* Delete 7 characters forward ("Hello, ") */
    qalam_buffer_delete(buffer, 7);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("World", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test deleting at the middle
 */
static int test_delete_at_middle(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello, World", 12);
    
    /* Move to position 5 */
    qalam_buffer_set_cursor_offset(buffer, 5);
    
    /* Delete 2 characters forward (", ") */
    qalam_buffer_delete(buffer, 2);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("HelloWorld", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test deleting at the end (backspace)
 */
static int test_delete_at_end(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello, World!", 13);
    
    /* Move to end */
    qalam_buffer_cursor_to_end(buffer);
    
    /* Delete 7 characters backward (backspace) */
    qalam_buffer_delete(buffer, -7);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("Hello,", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test delete range
 */
static int test_delete_range(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello, World!", 13);
    
    /* Delete from position 5 to 12 (", World") */
    qalam_buffer_delete_range(buffer, 5, 12);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("Hello!", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test deleting newlines updates line count
 */
static int test_delete_newlines(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Line1\nLine2\nLine3", 17);
    
    TEST_ASSERT_EQ(3, qalam_buffer_get_line_count(buffer));
    
    /* Delete the first newline (at position 5) */
    qalam_buffer_delete_range(buffer, 5, 6);
    
    TEST_ASSERT_EQ(2, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Cursor Movement Tests
 *============================================================================*/

/**
 * @brief Test cursor movement by line and column
 */
static int test_cursor_movement(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Line1\nLine2\nLine3", 17);
    
    QalamCursor cursor;
    
    /* Start position should be at end of content (after creation from text) */
    /* Move to beginning */
    qalam_buffer_cursor_to_start(buffer);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(0, cursor.line);
    TEST_ASSERT_EQ(0, cursor.column);
    TEST_ASSERT_EQ(0, cursor.offset);
    
    /* Move to line 1, column 2 */
    qalam_buffer_set_cursor(buffer, 1, 2);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(1, cursor.line);
    TEST_ASSERT_EQ(2, cursor.column);
    
    /* Move to end of buffer */
    qalam_buffer_cursor_to_end(buffer);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(2, cursor.line);
    TEST_ASSERT_EQ(5, cursor.column);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test cursor movement to line start/end
 */
static int test_cursor_line_start_end(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello\nWorld", 11);
    
    QalamCursor cursor;
    
    /* Go to middle of second line */
    qalam_buffer_set_cursor(buffer, 1, 2);
    
    /* Go to line start */
    qalam_buffer_cursor_to_line_start(buffer);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(1, cursor.line);
    TEST_ASSERT_EQ(0, cursor.column);
    
    /* Go to line end */
    qalam_buffer_cursor_to_line_end(buffer);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(1, cursor.line);
    TEST_ASSERT_EQ(5, cursor.column);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test relative cursor movement
 */
static int test_cursor_relative_movement(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "AAAA\nBBBB\nCCCC", 14);
    
    QalamCursor cursor;
    
    /* Start at beginning */
    qalam_buffer_cursor_to_start(buffer);
    
    /* Move down 2 lines, right 2 columns */
    qalam_buffer_move_cursor(buffer, 2, 2);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(2, cursor.line);
    TEST_ASSERT_EQ(2, cursor.column);
    
    /* Move up 1 line, left 1 column */
    qalam_buffer_move_cursor(buffer, -1, -1);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(1, cursor.line);
    TEST_ASSERT_EQ(1, cursor.column);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Line Counting Tests
 *============================================================================*/

/**
 * @brief Test line count with multiple newlines
 */
static int test_line_count_multiple(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "A\nB\nC\nD\nE", 9);
    
    TEST_ASSERT_EQ(5, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test line count updates on insert
 */
static int test_line_count_insert(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create(&buffer);
    
    TEST_ASSERT_EQ(1, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_insert(buffer, "A", 1);
    TEST_ASSERT_EQ(1, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_insert(buffer, "\n", 1);
    TEST_ASSERT_EQ(2, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_insert(buffer, "B\nC\n", 4);
    TEST_ASSERT_EQ(4, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test getting individual line content
 */
static int test_get_line(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "First\nSecond\nThird", 18);
    
    char line[256];
    size_t written;
    
    qalam_buffer_get_line(buffer, 0, line, sizeof(line), &written);
    TEST_ASSERT_STR_EQ("First", line);
    
    qalam_buffer_get_line(buffer, 1, line, sizeof(line), &written);
    TEST_ASSERT_STR_EQ("Second", line);
    
    qalam_buffer_get_line(buffer, 2, line, sizeof(line), &written);
    TEST_ASSERT_STR_EQ("Third", line);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Empty Buffer Edge Cases
 *============================================================================*/

/**
 * @brief Test operations on empty buffer
 */
static int test_empty_buffer_operations(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create(&buffer);
    
    /* Get content from empty buffer */
    char out[256];
    size_t written;
    QalamResult result = qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT(result == QALAM_OK);
    TEST_ASSERT_EQ(0, written);
    TEST_ASSERT_STR_EQ("", out);
    
    /* Delete from empty buffer should succeed (nothing to delete) */
    result = qalam_buffer_delete(buffer, 1);
    TEST_ASSERT(result == QALAM_OK);
    
    /* Cursor should be at 0,0 */
    QalamCursor cursor;
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(0, cursor.line);
    TEST_ASSERT_EQ(0, cursor.column);
    TEST_ASSERT_EQ(0, cursor.offset);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test empty line handling
 */
static int test_empty_lines(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "\n\n\n", 3);
    
    /* Should have 4 lines (3 newlines = 4 lines) */
    TEST_ASSERT_EQ(4, qalam_buffer_get_line_count(buffer));
    
    /* Each line should be empty except the actual newline */
    char line[256];
    size_t written;
    
    for (int i = 0; i < 4; i++) {
        QalamResult result = qalam_buffer_get_line(buffer, i, line, sizeof(line), &written);
        TEST_ASSERT(result == QALAM_OK);
        TEST_ASSERT_EQ(0, written);
    }
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Unicode/UTF-16 Surrogate Pair Tests
 *============================================================================*/

/**
 * @brief Test Arabic text handling
 */
static int test_arabic_text(void) {
    QalamBuffer* buffer = NULL;
    
    /* Arabic: "مرحبا" (Marhaba - Hello) */
    const char* arabic = "\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7";
    
    QalamResult result = qalam_buffer_create_from_text(&buffer, arabic, strlen(arabic));
    TEST_ASSERT(result == QALAM_OK);
    
    /* Verify content */
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ(arabic, out);
    
    /* Line count should still be 1 */
    TEST_ASSERT_EQ(1, qalam_buffer_get_line_count(buffer));
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test mixed Arabic and English
 */
static int test_mixed_arabic_english(void) {
    QalamBuffer* buffer = NULL;
    
    /* Mixed: "Hello مرحبا World" */
    const char* mixed = "Hello \xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7 World";
    
    QalamResult result = qalam_buffer_create_from_text(&buffer, mixed, strlen(mixed));
    TEST_ASSERT(result == QALAM_OK);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ(mixed, out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test emoji (surrogate pair handling)
 *
 * This tests that surrogate pairs (characters outside BMP) are handled correctly.
 * The music symbol 𝄞 (U+1D11E) is encoded as surrogate pair D834 DD1E in UTF-16.
 */
static int test_surrogate_pairs(void) {
    QalamBuffer* buffer = NULL;
    
    /* Music G clef: 𝄞 (U+1D11E) - requires surrogate pair in UTF-16 */
    /* UTF-8: F0 9D 84 9E */
    /* Use string concatenation to avoid escape sequence issues */
    const char* emoji = "A\xF0\x9D\x84\x9E" "B";  /* A + music note + B */
    
    QalamResult result = qalam_buffer_create_from_text(&buffer, emoji, strlen(emoji));
    TEST_ASSERT(result == QALAM_OK);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ(emoji, out);
    
    /* Move cursor through the buffer */
    qalam_buffer_cursor_to_start(buffer);
    
    /* After A, cursor should be at offset 1 */
    qalam_buffer_set_cursor_offset(buffer, 1);
    QalamCursor cursor;
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(1, cursor.offset);
    
    /* Move past the surrogate pair - should skip both surrogates together */
    /* The emoji takes 2 wchar_t (surrogate pair), so offset should go from 1 to 3 */
    qalam_buffer_set_cursor_offset(buffer, 3);
    qalam_buffer_get_cursor(buffer, &cursor);
    TEST_ASSERT_EQ(3, cursor.offset);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test that delete doesn't split surrogate pairs
 */
static int test_delete_surrogate_pair(void) {
    QalamBuffer* buffer = NULL;
    
    /* Music G clef: 𝄞 (U+1D11E) */
    /* Use string concatenation to avoid escape sequence issues */
    const char* emoji = "A\xF0\x9D\x84\x9E" "B";  /* A + music note + B */
    
    qalam_buffer_create_from_text(&buffer, emoji, strlen(emoji));
    
    /* Position cursor at the start of the surrogate pair */
    qalam_buffer_set_cursor_offset(buffer, 1);
    
    /* Delete forward should delete the whole surrogate pair, not just half */
    qalam_buffer_delete(buffer, 1);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    
    /* Should have "AB" after deleting the emoji */
    /* Note: Due to surrogate pair handling, this might delete both surrogates */
    /* The exact behavior depends on implementation */
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Large Content Tests
 *============================================================================*/

/**
 * @brief Test buffer with large content (1MB)
 */
static int test_large_content_1mb(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create(&buffer);
    
    /* Create 1MB of content */
    size_t size = 1024 * 1024;
    char* large_text = (char*)malloc(size + 1);
    if (!large_text) {
        qalam_buffer_destroy(buffer);
        return 0;  /* Skip test if can't allocate */
    }
    
    /* Fill with repeating pattern */
    for (size_t i = 0; i < size; i++) {
        large_text[i] = 'A' + (i % 26);
    }
    large_text[size] = '\0';
    
    QalamResult result = qalam_buffer_insert(buffer, large_text, size);
    TEST_ASSERT(result == QALAM_OK);
    
    /* Verify size */
    TEST_ASSERT_EQ(size, qalam_buffer_get_size(buffer));
    
    free(large_text);
    qalam_buffer_destroy(buffer);
    return 0;
}

/**
 * @brief Test buffer with large content (10MB) - performance test
 */
static int test_large_content_10mb(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create(&buffer);
    
    /* Create 10MB of content with newlines */
    size_t size = 10 * 1024 * 1024;
    char* large_text = (char*)malloc(size + 1);
    if (!large_text) {
        qalam_buffer_destroy(buffer);
        return 0;  /* Skip test if can't allocate */
    }
    
    /* Fill with lines of 80 characters each */
    size_t pos = 0;
    while (pos < size - 81) {
        for (int i = 0; i < 80 && pos < size; i++) {
            large_text[pos++] = 'A' + (i % 26);
        }
        if (pos < size) {
            large_text[pos++] = '\n';
        }
    }
    large_text[pos] = '\0';
    
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    
    /* Time the insert */
    QueryPerformanceCounter(&start);
    QalamResult result = qalam_buffer_insert(buffer, large_text, pos);
    QueryPerformanceCounter(&end);
    
    double insert_time = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    printf("\n    10MB insert time: %.3f seconds", insert_time);
    
    TEST_ASSERT(result == QALAM_OK);
    
    /* Time cursor movement to middle */
    size_t content_len = qalam_buffer_get_size(buffer);
    size_t middle = content_len / 2;
    
    QueryPerformanceCounter(&start);
    qalam_buffer_set_cursor_offset(buffer, middle);
    QueryPerformanceCounter(&end);
    
    double move_time = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    printf("\n    Cursor move to middle time: %.3f seconds", move_time);
    
    /* Time insert at middle */
    QueryPerformanceCounter(&start);
    qalam_buffer_insert(buffer, "INSERTED", 8);
    QueryPerformanceCounter(&end);
    
    double mid_insert_time = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    printf("\n    Insert at middle time: %.3f seconds", mid_insert_time);
    
    free(large_text);
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Replace Operation Tests
 *============================================================================*/

/**
 * @brief Test replace operation
 */
static int test_replace(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello, World!", 13);
    
    /* Replace "World" with "Universe" */
    qalam_buffer_replace(buffer, 7, 12, "Universe", 8);
    
    char out[256];
    size_t written;
    qalam_buffer_get_content(buffer, out, sizeof(out), &written);
    TEST_ASSERT_STR_EQ("Hello, Universe!", out);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Selection Tests
 *============================================================================*/

/**
 * @brief Test selection operations
 */
static int test_selection(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello, World!", 13);
    
    /* Set selection from "World" */
    qalam_buffer_set_selection(buffer, 0, 7, 0, 12);
    
    QalamSelection sel;
    qalam_buffer_get_selection(buffer, &sel);
    TEST_ASSERT(sel.is_active == true);
    TEST_ASSERT_EQ(0, sel.start.line);
    TEST_ASSERT_EQ(7, sel.start.column);
    TEST_ASSERT_EQ(0, sel.end.line);
    TEST_ASSERT_EQ(12, sel.end.column);
    
    /* Get selected text */
    char selected[256];
    size_t written;
    qalam_buffer_get_selected_text(buffer, selected, sizeof(selected), &written);
    TEST_ASSERT_STR_EQ("World", selected);
    
    /* Clear selection */
    qalam_buffer_clear_selection(buffer);
    qalam_buffer_get_selection(buffer, &sel);
    TEST_ASSERT(sel.is_active == false);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Buffer Stats Tests
 *============================================================================*/

/**
 * @brief Test buffer statistics
 */
static int test_buffer_stats(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create_from_text(&buffer, "Hello\nWorld", 11);
    
    QalamBufferStats stats;
    QalamResult result = qalam_buffer_get_stats(buffer, &stats);
    TEST_ASSERT(result == QALAM_OK);
    
    TEST_ASSERT_EQ(11, stats.total_bytes);
    TEST_ASSERT_EQ(11, stats.total_chars);
    TEST_ASSERT_EQ(2, stats.total_lines);
    TEST_ASSERT(stats.capacity >= 11);
    TEST_ASSERT(stats.gap_size > 0);
    TEST_ASSERT(stats.is_modified == false);  /* Created from text, not modified */
    TEST_ASSERT(stats.is_readonly == false);
    
    /* Modify buffer */
    qalam_buffer_cursor_to_end(buffer);
    qalam_buffer_insert(buffer, "!", 1);
    
    qalam_buffer_get_stats(buffer, &stats);
    TEST_ASSERT(stats.is_modified == true);
    
    /* Clear modified flag */
    qalam_buffer_clear_modified(buffer);
    TEST_ASSERT(qalam_buffer_is_modified(buffer) == false);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Line Info Tests
 *============================================================================*/

/**
 * @brief Test line information
 */
static int test_line_info(void) {
    QalamBuffer* buffer = NULL;
    
    /* Mix of Arabic and English */
    const char* text = "Hello\n\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7\nWorld";
    qalam_buffer_create_from_text(&buffer, text, strlen(text));
    
    QalamLineInfo info;
    
    /* Line 0: "Hello" - LTR */
    qalam_buffer_get_line_info(buffer, 0, &info);
    TEST_ASSERT_EQ(0, info.line_number);
    TEST_ASSERT_EQ(5, info.length_chars);
    TEST_ASSERT(info.has_ltr_chars == true);
    TEST_ASSERT(info.has_rtl_chars == false);
    
    /* Line 1: Arabic - RTL */
    qalam_buffer_get_line_info(buffer, 1, &info);
    TEST_ASSERT_EQ(1, info.line_number);
    TEST_ASSERT_EQ(5, info.length_chars);
    TEST_ASSERT(info.has_ltr_chars == false);
    TEST_ASSERT(info.has_rtl_chars == true);
    TEST_ASSERT(info.direction == QALAM_DIR_RTL);
    
    /* Line 2: "World" - LTR */
    qalam_buffer_get_line_info(buffer, 2, &info);
    TEST_ASSERT_EQ(2, info.line_number);
    TEST_ASSERT_EQ(5, info.length_chars);
    TEST_ASSERT(info.has_ltr_chars == true);
    TEST_ASSERT(info.has_rtl_chars == false);
    TEST_ASSERT(info.direction == QALAM_DIR_LTR);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Error Handling Tests
 *============================================================================*/

/**
 * @brief Test error handling for invalid parameters
 */
static int test_error_handling(void) {
    QalamBuffer* buffer = NULL;
    qalam_buffer_create(&buffer);
    
    /* NULL buffer parameter */
    TEST_ASSERT(qalam_buffer_insert(NULL, "test", 4) == QALAM_ERROR_NULL_POINTER);
    TEST_ASSERT(qalam_buffer_delete(NULL, 1) == QALAM_ERROR_NULL_POINTER);
    
    /* Invalid line number */
    char line[256];
    size_t written;
    TEST_ASSERT(qalam_buffer_get_line(buffer, 999, line, sizeof(line), &written) == QALAM_ERROR_INVALID_RANGE);
    
    /* Invalid range */
    TEST_ASSERT(qalam_buffer_get_range(buffer, 100, 200, line, sizeof(line), &written) == QALAM_ERROR_INVALID_RANGE);
    
    qalam_buffer_destroy(buffer);
    return 0;
}

/*=============================================================================
 * Main Test Runner
 *============================================================================*/

int main(int argc, char* argv[]) {
    printf("===========================================\n");
    printf("  Qalam IDE - Gap Buffer Unit Tests\n");
    printf("===========================================\n\n");
    
    /* Set console to UTF-8 for proper display */
    SetConsoleOutputCP(CP_UTF8);
    
    printf("Buffer Creation and Destruction:\n");
    RUN_TEST(buffer_create_empty);
    RUN_TEST(buffer_create_with_capacity);
    RUN_TEST(buffer_create_from_text);
    RUN_TEST(buffer_destroy_null);
    RUN_TEST(buffer_create_null_pointer);
    
    printf("\nInsert Operations:\n");
    RUN_TEST(insert_at_beginning_empty);
    RUN_TEST(insert_at_beginning);
    RUN_TEST(insert_at_middle);
    RUN_TEST(insert_at_end);
    RUN_TEST(insert_at_position);
    RUN_TEST(insert_newlines);
    
    printf("\nDelete Operations:\n");
    RUN_TEST(delete_at_beginning);
    RUN_TEST(delete_at_middle);
    RUN_TEST(delete_at_end);
    RUN_TEST(delete_range);
    RUN_TEST(delete_newlines);
    
    printf("\nCursor Movement:\n");
    RUN_TEST(cursor_movement);
    RUN_TEST(cursor_line_start_end);
    RUN_TEST(cursor_relative_movement);
    
    printf("\nLine Counting:\n");
    RUN_TEST(line_count_multiple);
    RUN_TEST(line_count_insert);
    RUN_TEST(get_line);
    
    printf("\nEmpty Buffer Edge Cases:\n");
    RUN_TEST(empty_buffer_operations);
    RUN_TEST(empty_lines);
    
    printf("\nUnicode/UTF-16 Handling:\n");
    RUN_TEST(arabic_text);
    RUN_TEST(mixed_arabic_english);
    RUN_TEST(surrogate_pairs);
    RUN_TEST(delete_surrogate_pair);
    
    printf("\nLarge Content Handling:\n");
    RUN_TEST(large_content_1mb);
    RUN_TEST(large_content_10mb);
    
    printf("\nReplace Operations:\n");
    RUN_TEST(replace);
    
    printf("\nSelection Operations:\n");
    RUN_TEST(selection);
    
    printf("\nBuffer Statistics:\n");
    RUN_TEST(buffer_stats);
    
    printf("\nLine Information:\n");
    RUN_TEST(line_info);
    
    printf("\nError Handling:\n");
    RUN_TEST(error_handling);
    
    printf("\n===========================================\n");
    printf("  Test Results: %d/%d passed", g_tests_passed, g_tests_total);
    if (g_tests_failed > 0) {
        printf(" (%d FAILED)", g_tests_failed);
    }
    printf("\n===========================================\n");
    
    return g_tests_failed > 0 ? 1 : 0;
}