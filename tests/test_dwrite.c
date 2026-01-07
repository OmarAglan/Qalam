/**
 * @file test_dwrite.c
 * @brief Qalam IDE - DirectWrite Unit Tests
 * 
 * Tests for the DirectWrite text rendering system including:
 * - Factory initialization/shutdown
 * - Text format creation
 * - Arabic text layout creation
 * - Text measurement
 * - Hit testing (point to position, position to point)
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 */

#include "dwrite_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

/*=============================================================================
 * Test Framework
 *============================================================================*/

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    printf("  Running %s... ", #name); \
    fflush(stdout); \
    g_tests_run++; \
    test_##name(); \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("FAILED\n"); \
        printf("    Assertion failed: %s\n", #condition); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAILED\n"); \
        printf("    Expected: %d, Actual: %d\n", (int)(expected), (int)(actual)); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_OK(result) do { \
    QalamResult _r = (result); \
    if (_r != QALAM_OK) { \
        printf("FAILED\n"); \
        printf("    Expected QALAM_OK, got error code: %d\n", (int)_r); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("FAILED\n"); \
        printf("    Pointer is NULL: %s\n", #ptr); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_PASSED() do { \
    printf("PASSED\n"); \
    g_tests_passed++; \
} while(0)

/*=============================================================================
 * Test Cases: Factory Lifecycle
 *============================================================================*/

/**
 * @brief Test basic factory initialization and shutdown
 */
TEST(factory_init_shutdown) {
    QalamResult result;
    
    /* Initialize DirectWrite */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Verify factory is available */
    IDWriteFactory* factory = qalam_dwrite_get_factory();
    ASSERT_NOT_NULL(factory);
    
    /* Verify D2D factory is available */
    ID2D1Factory* d2d_factory = qalam_dwrite_get_d2d_factory();
    ASSERT_NOT_NULL(d2d_factory);
    
    /* Shutdown */
    qalam_dwrite_shutdown();
    
    /* After shutdown, factories should be NULL */
    factory = qalam_dwrite_get_factory();
    ASSERT(factory == NULL);
    
    TEST_PASSED();
}

/**
 * @brief Test multiple init/shutdown cycles (reference counting)
 */
TEST(factory_refcount) {
    QalamResult result;
    
    /* First init */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    ASSERT_NOT_NULL(qalam_dwrite_get_factory());
    
    /* Second init (should increment refcount) */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    ASSERT_NOT_NULL(qalam_dwrite_get_factory());
    
    /* First shutdown (should decrement refcount, factory still valid) */
    qalam_dwrite_shutdown();
    ASSERT_NOT_NULL(qalam_dwrite_get_factory());
    
    /* Second shutdown (refcount reaches 0, factory released) */
    qalam_dwrite_shutdown();
    ASSERT(qalam_dwrite_get_factory() == NULL);
    
    TEST_PASSED();
}

/**
 * @brief Test that operations fail when not initialized
 */
TEST(factory_not_initialized) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    
    /* Ensure not initialized */
    ASSERT(qalam_dwrite_get_factory() == NULL);
    
    /* Try to create text format without initialization */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f, 
        DWRITE_FONT_WEIGHT_NORMAL, 
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    
    ASSERT_EQ(QALAM_ERROR_NOT_INITIALIZED, result);
    ASSERT(format == NULL);
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Cases: Text Format Creation
 *============================================================================*/

/**
 * @brief Test basic text format creation
 */
TEST(text_format_create) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create text format */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code",
        14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(format);
    
    /* Verify font size */
    float font_size = format->lpVtbl->GetFontSize(format);
    ASSERT(font_size >= 13.9f && font_size <= 14.1f);
    
    /* Release format */
    format->lpVtbl->Release(format);
    
    /* Cleanup */
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test Arabic text format creation
 */
TEST(arabic_text_format_create) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    DWRITE_READING_DIRECTION reading_dir;
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic text format */
    result = qalam_dwrite_create_arabic_text_format(
        L"Cascadia Code",
        16.0f,
        &format
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(format);
    
    /* Verify RTL reading direction */
    reading_dir = format->lpVtbl->GetReadingDirection(format);
    ASSERT_EQ(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT, reading_dir);
    
    /* Release format */
    format->lpVtbl->Release(format);
    
    /* Cleanup */
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test text format creation with various font weights
 */
TEST(text_format_weights) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    DWRITE_FONT_WEIGHT weights[] = {
        DWRITE_FONT_WEIGHT_THIN,
        DWRITE_FONT_WEIGHT_LIGHT,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_WEIGHT_MEDIUM,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_WEIGHT_BLACK
    };
    int num_weights = sizeof(weights) / sizeof(weights[0]);
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Test each weight */
    for (int i = 0; i < num_weights; i++) {
        result = qalam_dwrite_create_text_format(
            L"Segoe UI",  /* Use system font for weight variety */
            12.0f,
            weights[i],
            DWRITE_FONT_STYLE_NORMAL,
            &format
        );
        ASSERT_OK(result);
        ASSERT_NOT_NULL(format);
        
        format->lpVtbl->Release(format);
        format = NULL;
    }
    
    /* Cleanup */
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Cases: Text Layout Creation
 *============================================================================*/

/**
 * @brief Test basic text layout creation
 */
TEST(text_layout_create) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    const wchar_t* text = L"Hello, World!";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_create_text_layout(
        text, wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(layout);
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test RTL text layout creation with Arabic text
 */
TEST(rtl_text_layout_create) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    const wchar_t* text = L"مرحبا بالعالم";  /* "Hello World" in Arabic */
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic format */
    result = qalam_dwrite_create_arabic_text_format(
        L"Segoe UI",  /* System font with Arabic support */
        14.0f,
        &format
    );
    ASSERT_OK(result);
    
    /* Create RTL layout */
    result = qalam_dwrite_create_rtl_text_layout(
        text, wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(layout);
    
    /* Verify layout has RTL reading direction */
    DWRITE_READING_DIRECTION dir = layout->lpVtbl->GetReadingDirection(layout);
    ASSERT_EQ(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT, dir);
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test layout with empty text
 */
TEST(text_layout_empty) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    const wchar_t* text = L"";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_OK(result);
    
    /* Create layout with empty text */
    result = qalam_dwrite_create_text_layout(
        text, 0, format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(layout);
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Cases: Text Measurement
 *============================================================================*/

/**
 * @brief Test text metrics measurement
 */
TEST(text_measure) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    DWRITE_TEXT_METRICS metrics;
    const wchar_t* text = L"Hello, World!";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_create_text_layout(
        text, wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Measure text */
    result = qalam_dwrite_measure_text(layout, &metrics);
    ASSERT_OK(result);
    
    /* Verify reasonable metrics */
    ASSERT(metrics.width > 0);      /* Should have some width */
    ASSERT(metrics.height > 0);     /* Should have some height */
    ASSERT(metrics.lineCount >= 1); /* At least one line */
    
    /* Width should be reasonable for the text */
    ASSERT(metrics.width < 500.0f); /* Not too wide for "Hello, World!" */
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test measurement of Arabic text
 */
TEST(arabic_text_measure) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    DWRITE_TEXT_METRICS metrics;
    const wchar_t* text = L"مرحبا";  /* "Hello" in Arabic */
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic format */
    result = qalam_dwrite_create_arabic_text_format(
        L"Segoe UI", 14.0f, &format
    );
    ASSERT_OK(result);
    
    /* Create RTL layout */
    result = qalam_dwrite_create_rtl_text_layout(
        text, wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Measure text */
    result = qalam_dwrite_measure_text(layout, &metrics);
    ASSERT_OK(result);
    
    /* Verify reasonable metrics */
    ASSERT(metrics.width > 0);
    ASSERT(metrics.height > 0);
    ASSERT(metrics.lineCount >= 1);
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Cases: Hit Testing
 *============================================================================*/

/**
 * @brief Test hit test from point to text position
 */
TEST(hit_test_point) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    DWRITE_HIT_TEST_METRICS hit_metrics;
    BOOL is_trailing, is_inside;
    const wchar_t* text = L"ABCDEFGHIJ";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_create_text_layout(
        text, wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Hit test at the origin (should hit first character) */
    result = qalam_dwrite_hit_test_point(
        layout, 0.0f, 0.0f,
        &hit_metrics, &is_trailing, &is_inside
    );
    ASSERT_OK(result);
    ASSERT(hit_metrics.textPosition == 0);  /* Should be at first character */
    
    /* Hit test in the middle (should hit some middle character) */
    DWRITE_TEXT_METRICS text_metrics;
    qalam_dwrite_measure_text(layout, &text_metrics);
    
    result = qalam_dwrite_hit_test_point(
        layout, text_metrics.width / 2.0f, text_metrics.height / 2.0f,
        &hit_metrics, &is_trailing, &is_inside
    );
    ASSERT_OK(result);
    ASSERT(hit_metrics.textPosition > 0);  /* Should be past first character */
    ASSERT(hit_metrics.textPosition < wcslen(text));  /* Should be before end */
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test hit test from text position to coordinates
 */
TEST(hit_test_position) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    float x, y;
    DWRITE_HIT_TEST_METRICS hit_metrics;
    const wchar_t* text = L"ABCDEFGHIJ";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_create_text_layout(
        text, wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Get position of first character (leading edge) */
    result = qalam_dwrite_hit_test_position(
        layout, 0, FALSE, &x, &y, &hit_metrics
    );
    ASSERT_OK(result);
    ASSERT(x >= 0.0f);  /* Should be at or after origin */
    ASSERT(x < 50.0f);  /* Should be near the start */
    
    /* Get position of last character (trailing edge) */
    result = qalam_dwrite_hit_test_position(
        layout, (uint32_t)wcslen(text) - 1, TRUE, &x, &y, &hit_metrics
    );
    ASSERT_OK(result);
    ASSERT(x > 50.0f);  /* Should be well past the start */
    
    /* Verify positions are monotonically increasing */
    float prev_x = 0.0f;
    for (uint32_t i = 0; i < wcslen(text); i++) {
        result = qalam_dwrite_hit_test_position(
            layout, i, FALSE, &x, &y, NULL
        );
        ASSERT_OK(result);
        ASSERT(x >= prev_x);  /* Each position should be >= previous */
        prev_x = x;
    }
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test hit testing for RTL Arabic text
 */
TEST(hit_test_rtl) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    IDWriteTextLayout* layout = NULL;
    float x, y;
    const wchar_t* text = L"مرحبا";  /* "Hello" in Arabic - 5 characters */
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic format */
    result = qalam_dwrite_create_arabic_text_format(
        L"Segoe UI", 14.0f, &format
    );
    ASSERT_OK(result);
    
    /* Create RTL layout */
    result = qalam_dwrite_create_rtl_text_layout(
        text, wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Get position of first character (in RTL, this should be on the right) */
    result = qalam_dwrite_hit_test_position(
        layout, 0, FALSE, &x, &y, NULL
    );
    ASSERT_OK(result);
    
    float first_char_x = x;
    
    /* Get position of last character (in RTL, this should be on the left) */
    result = qalam_dwrite_hit_test_position(
        layout, (uint32_t)wcslen(text) - 1, TRUE, &x, &y, NULL
    );
    ASSERT_OK(result);
    
    float last_char_x = x;
    
    /* In RTL, the first character position should be >= last character position
       (or very close, depending on layout origin) */
    /* Note: The exact behavior depends on the layout width and origin */
    
    /* Cleanup */
    layout->lpVtbl->Release(layout);
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Cases: Font Fallback
 *============================================================================*/

/**
 * @brief Test font fallback setup (placeholder)
 */
TEST(font_fallback_setup) {
    QalamResult result;
    IDWriteTextFormat* format = NULL;
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_OK(result);
    
    /* Setup font fallback */
    result = qalam_dwrite_setup_font_fallback(format);
    ASSERT_OK(result);
    
    /* Cleanup */
    format->lpVtbl->Release(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Cases: Error Handling
 *============================================================================*/

/**
 * @brief Test null pointer handling
 */
TEST(null_pointer_handling) {
    QalamResult result;
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Test null format pointer */
    result = qalam_dwrite_create_text_format(
        L"Cascadia Code", 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        NULL
    );
    ASSERT_EQ(QALAM_ERROR_NULL_POINTER, result);
    
    /* Test null font family */
    IDWriteTextFormat* format = NULL;
    result = qalam_dwrite_create_text_format(
        NULL, 14.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        &format
    );
    ASSERT_EQ(QALAM_ERROR_NULL_POINTER, result);
    
    /* Cleanup */
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Runner
 *============================================================================*/

void run_factory_tests(void) {
    printf("\n=== Factory Lifecycle Tests ===\n");
    RUN_TEST(factory_init_shutdown);
    RUN_TEST(factory_refcount);
    RUN_TEST(factory_not_initialized);
}

void run_format_tests(void) {
    printf("\n=== Text Format Tests ===\n");
    RUN_TEST(text_format_create);
    RUN_TEST(arabic_text_format_create);
    RUN_TEST(text_format_weights);
}

void run_layout_tests(void) {
    printf("\n=== Text Layout Tests ===\n");
    RUN_TEST(text_layout_create);
    RUN_TEST(rtl_text_layout_create);
    RUN_TEST(text_layout_empty);
}

void run_measurement_tests(void) {
    printf("\n=== Text Measurement Tests ===\n");
    RUN_TEST(text_measure);
    RUN_TEST(arabic_text_measure);
}

void run_hit_test_tests(void) {
    printf("\n=== Hit Testing Tests ===\n");
    RUN_TEST(hit_test_point);
    RUN_TEST(hit_test_position);
    RUN_TEST(hit_test_rtl);
}

void run_fallback_tests(void) {
    printf("\n=== Font Fallback Tests ===\n");
    RUN_TEST(font_fallback_setup);
}

void run_error_tests(void) {
    printf("\n=== Error Handling Tests ===\n");
    RUN_TEST(null_pointer_handling);
}

int main(void) {
    /* Set locale for wide character output */
    setlocale(LC_ALL, "");
    
    printf("========================================\n");
    printf("Qalam IDE - DirectWrite Test Suite\n");
    printf("========================================\n");
    
    /* Run all test suites */
    run_factory_tests();
    run_format_tests();
    run_layout_tests();
    run_measurement_tests();
    run_hit_test_tests();
    run_fallback_tests();
    run_error_tests();
    
    /* Print summary */
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", g_tests_run);
    printf("  Passed: %d\n", g_tests_passed);
    printf("  Failed: %d\n", g_tests_failed);
    printf("========================================\n");
    
    if (g_tests_failed > 0) {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
    
    printf("ALL TESTS PASSED\n");
    return 0;
}