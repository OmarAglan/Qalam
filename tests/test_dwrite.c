/**
 * @file test_dwrite.c
 * @brief Qalam IDE - DirectWrite Unit Tests
 * 
 * Tests for the DirectWrite text rendering system using the new pure C API:
 * - Factory initialization/shutdown
 * - Text format creation
 * - Arabic text layout creation
 * - Text measurement
 * - Hit testing (point to position, position to point)
 * 
 * @version 0.0.2
 * @copyright (c) 2026 Qalam Project
 */

#include "dwrite_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>

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
    
    /* Verify initialized */
    ASSERT(qalam_dwrite_is_initialized());
    
    /* Shutdown */
    qalam_dwrite_shutdown();
    
    /* After shutdown, should not be initialized */
    ASSERT(!qalam_dwrite_is_initialized());
    
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
    ASSERT(qalam_dwrite_is_initialized());
    
    /* Second init (should increment refcount) */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    ASSERT(qalam_dwrite_is_initialized());
    
    /* First shutdown (should decrement refcount, still initialized) */
    qalam_dwrite_shutdown();
    ASSERT(qalam_dwrite_is_initialized());
    
    /* Second shutdown (refcount reaches 0, not initialized) */
    qalam_dwrite_shutdown();
    ASSERT(!qalam_dwrite_is_initialized());
    
    TEST_PASSED();
}

/**
 * @brief Test that operations fail when not initialized
 */
TEST(factory_not_initialized) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    
    /* Ensure not initialized */
    ASSERT(!qalam_dwrite_is_initialized());
    
    /* Try to create text format without initialization */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, &format);
    
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
    QalamDWriteTextFormat* format = NULL;
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create text format */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, &format);
    ASSERT_OK(result);
    ASSERT_NOT_NULL(format);
    
    /* Destroy format */
    qalam_dwrite_text_format_destroy(format);
    
    /* Cleanup */
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test Arabic text format creation
 */
TEST(arabic_text_format_create) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic text format */
    result = qalam_dwrite_text_format_create_arabic(
        L"Segoe UI",
        16.0f,
        &format
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(format);
    
    /* Destroy format */
    qalam_dwrite_text_format_destroy(format);
    
    /* Cleanup */
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test text format creation with various font weights
 */
TEST(text_format_weights) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteFontWeight weights[] = {
        QALAM_DWRITE_FONT_WEIGHT_THIN,
        QALAM_DWRITE_FONT_WEIGHT_LIGHT,
        QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        QALAM_DWRITE_FONT_WEIGHT_MEDIUM,
        QALAM_DWRITE_FONT_WEIGHT_BOLD,
        QALAM_DWRITE_FONT_WEIGHT_BLACK
    };
    int num_weights = sizeof(weights) / sizeof(weights[0]);
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Test each weight */
    for (int i = 0; i < num_weights; i++) {
        QalamDWriteFontParams params = {
            .family = L"Segoe UI",
            .size = 12.0f,
            .weight = weights[i],
            .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
            .is_rtl = false
        };
        
        result = qalam_dwrite_text_format_create(&params, &format);
        ASSERT_OK(result);
        ASSERT_NOT_NULL(format);
        
        qalam_dwrite_text_format_destroy(format);
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
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    const wchar_t* text = L"Hello, World!";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, &format);
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_text_layout_create(
        text, (uint32_t)wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(layout);
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test RTL text layout creation with Arabic text
 */
TEST(rtl_text_layout_create) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    const wchar_t* text = L"مرحبا بالعالم";  /* "Hello World" in Arabic */
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic format */
    result = qalam_dwrite_text_format_create_arabic(
        L"Segoe UI",
        14.0f,
        &format
    );
    ASSERT_OK(result);
    
    /* Create layout (RTL is configured via format) */
    result = qalam_dwrite_text_layout_create(
        text, (uint32_t)wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(layout);
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test layout with empty text
 */
TEST(text_layout_empty) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    const wchar_t* text = L"";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, &format);
    ASSERT_OK(result);
    
    /* Create layout with empty text */
    result = qalam_dwrite_text_layout_create(
        text, 0, format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    ASSERT_NOT_NULL(layout);
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
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
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    QalamDWriteTextMetrics metrics;
    const wchar_t* text = L"Hello, World!";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, &format);
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_text_layout_create(
        text, (uint32_t)wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Measure text */
    result = qalam_dwrite_text_layout_get_metrics(layout, &metrics);
    ASSERT_OK(result);
    
    /* Verify reasonable metrics */
    ASSERT(metrics.width > 0);       /* Should have some width */
    ASSERT(metrics.height > 0);      /* Should have some height */
    ASSERT(metrics.line_count >= 1); /* At least one line */
    
    /* Width should be reasonable for the text */
    ASSERT(metrics.width < 500.0f);  /* Not too wide for "Hello, World!" */
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test measurement of Arabic text
 */
TEST(arabic_text_measure) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    QalamDWriteTextMetrics metrics;
    const wchar_t* text = L"مرحبا";  /* "Hello" in Arabic */
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic format */
    result = qalam_dwrite_text_format_create_arabic(
        L"Segoe UI", 14.0f, &format
    );
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_text_layout_create(
        text, (uint32_t)wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Measure text */
    result = qalam_dwrite_text_layout_get_metrics(layout, &metrics);
    ASSERT_OK(result);
    
    /* Verify reasonable metrics */
    ASSERT(metrics.width > 0);
    ASSERT(metrics.height > 0);
    ASSERT(metrics.line_count >= 1);
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
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
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    QalamDWriteHitTestResult hit_result;
    const wchar_t* text = L"ABCDEFGHIJ";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, &format);
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_text_layout_create(
        text, (uint32_t)wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Hit test at the origin (should hit first character) */
    result = qalam_dwrite_text_layout_hit_test_point(
        layout, 0.0f, 0.0f, &hit_result
    );
    ASSERT_OK(result);
    ASSERT(hit_result.text_position == 0);  /* Should be at first character */
    
    /* Hit test in the middle (should hit some middle character) */
    QalamDWriteTextMetrics metrics;
    qalam_dwrite_text_layout_get_metrics(layout, &metrics);
    
    result = qalam_dwrite_text_layout_hit_test_point(
        layout, metrics.width / 2.0f, metrics.height / 2.0f, &hit_result
    );
    ASSERT_OK(result);
    ASSERT(hit_result.text_position > 0);               /* Should be past first character */
    ASSERT(hit_result.text_position < wcslen(text));    /* Should be before end */
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test hit test from text position to coordinates
 */
TEST(hit_test_position) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    float x, y;
    QalamDWriteHitTestResult hit_result;
    const wchar_t* text = L"ABCDEFGHIJ";
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create format */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, &format);
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_text_layout_create(
        text, (uint32_t)wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Get position of first character (leading edge) */
    result = qalam_dwrite_text_layout_hit_test_position(
        layout, 0, false, &x, &y, &hit_result
    );
    ASSERT_OK(result);
    ASSERT(x >= 0.0f);  /* Should be at or after origin */
    ASSERT(x < 50.0f);  /* Should be near the start */
    
    /* Get position of last character (trailing edge) */
    result = qalam_dwrite_text_layout_hit_test_position(
        layout, (uint32_t)wcslen(text) - 1, true, &x, &y, &hit_result
    );
    ASSERT_OK(result);
    ASSERT(x > 50.0f);  /* Should be well past the start */
    
    /* Verify positions are monotonically increasing */
    float prev_x = 0.0f;
    for (uint32_t i = 0; i < wcslen(text); i++) {
        result = qalam_dwrite_text_layout_hit_test_position(
            layout, i, false, &x, &y, NULL
        );
        ASSERT_OK(result);
        ASSERT(x >= prev_x);  /* Each position should be >= previous */
        prev_x = x;
    }
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/**
 * @brief Test hit testing for RTL Arabic text
 */
TEST(hit_test_rtl) {
    QalamResult result;
    QalamDWriteTextFormat* format = NULL;
    QalamDWriteTextLayout* layout = NULL;
    float x, y;
    const wchar_t* text = L"مرحبا";  /* "Hello" in Arabic - 5 characters */
    
    /* Initialize */
    result = qalam_dwrite_init();
    ASSERT_OK(result);
    
    /* Create Arabic format */
    result = qalam_dwrite_text_format_create_arabic(
        L"Segoe UI", 14.0f, &format
    );
    ASSERT_OK(result);
    
    /* Create layout */
    result = qalam_dwrite_text_layout_create(
        text, (uint32_t)wcslen(text), format,
        1000.0f, 100.0f,
        &layout
    );
    ASSERT_OK(result);
    
    /* Get position of first character */
    result = qalam_dwrite_text_layout_hit_test_position(
        layout, 0, false, &x, &y, NULL
    );
    ASSERT_OK(result);
    
    float first_char_x = x;
    
    /* Get position of last character */
    result = qalam_dwrite_text_layout_hit_test_position(
        layout, (uint32_t)wcslen(text) - 1, true, &x, &y, NULL
    );
    ASSERT_OK(result);
    
    float last_char_x = x;
    
    /* In RTL, positions may differ - just verify we got valid results */
    ASSERT(first_char_x >= 0.0f);
    ASSERT(last_char_x >= 0.0f);
    
    /* Cleanup */
    qalam_dwrite_text_layout_destroy(layout);
    qalam_dwrite_text_format_destroy(format);
    qalam_dwrite_shutdown();
    
    TEST_PASSED();
}

/*=============================================================================
 * Test Cases: Color Utilities
 *============================================================================*/

/**
 * @brief Test color creation utilities
 */
TEST(color_utilities) {
    QalamDWriteColor color;
    
    /* Test RGB color creation */
    color = qalam_dwrite_color_from_rgb(255, 128, 0);
    ASSERT(color.r >= 0.99f && color.r <= 1.01f);
    ASSERT(color.g >= 0.49f && color.g <= 0.51f);
    ASSERT(color.b >= -0.01f && color.b <= 0.01f);
    ASSERT(color.a >= 0.99f && color.a <= 1.01f);
    
    /* Test RGBA color creation */
    color = qalam_dwrite_color_from_rgba(128, 128, 128, 128);
    ASSERT(color.r >= 0.49f && color.r <= 0.51f);
    ASSERT(color.a >= 0.49f && color.a <= 0.51f);
    
    /* Test hex color creation */
    color = qalam_dwrite_color_from_hex(0xFF0000);  /* Red */
    ASSERT(color.r >= 0.99f && color.r <= 1.01f);
    ASSERT(color.g >= -0.01f && color.g <= 0.01f);
    ASSERT(color.b >= -0.01f && color.b <= 0.01f);
    
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
    
    /* Test null format output pointer */
    QalamDWriteFontParams params = {
        .family = L"Cascadia Code",
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    result = qalam_dwrite_text_format_create(&params, NULL);
    ASSERT_EQ(QALAM_ERROR_NULL_POINTER, result);
    
    /* Test null font family */
    QalamDWriteFontParams null_params = {
        .family = NULL,
        .size = 14.0f,
        .weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL,
        .style = QALAM_DWRITE_FONT_STYLE_NORMAL,
        .is_rtl = false
    };
    
    QalamDWriteTextFormat* format = NULL;
    result = qalam_dwrite_text_format_create(&null_params, &format);
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

void run_utility_tests(void) {
    printf("\n=== Utility Tests ===\n");
    RUN_TEST(color_utilities);
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
    printf("Using New Pure C API (dwrite_api.h)\n");
    printf("========================================\n");
    
    /* Run all test suites */
    run_factory_tests();
    run_format_tests();
    run_layout_tests();
    run_measurement_tests();
    run_hit_test_tests();
    run_utility_tests();
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