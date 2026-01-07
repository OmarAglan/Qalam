/**
 * @file dwrite.c
 * @brief Qalam IDE - DirectWrite Implementation
 * 
 * Implements the DirectWrite singleton manager and text rendering utilities
 * for the Qalam IDE. Provides C-style wrappers around COM interfaces for
 * DirectWrite text formatting, layout, and measurement.
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 */

#include "dwrite_internal.h"
#include <stdio.h>
#include <stdlib.h>

/*=============================================================================
 * Global Singleton State
 *============================================================================*/

/**
 * @brief Global DirectWrite context singleton
 * 
 * This is the single instance of the DirectWrite context that manages
 * all DirectWrite resources for the application.
 */
static DWriteContext g_dwrite_context = {
    .factory = NULL,
    .system_fonts = NULL,
    .d2d_factory = NULL,
    .initialized = false,
    .ref_count = 0
};

/**
 * @brief Flag to track if critical section has been initialized
 */
static bool g_cs_initialized = false;

/*=============================================================================
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Initialize the critical section if not already done
 */
static void ensure_cs_initialized(void) {
    if (!g_cs_initialized) {
        InitializeCriticalSection(&g_dwrite_context.init_lock);
        g_cs_initialized = true;
    }
}

QalamResult dwrite_hresult_to_result(HRESULT hr) {
    if (SUCCEEDED(hr)) {
        return QALAM_OK;
    }
    
    switch (hr) {
        case E_OUTOFMEMORY:
            return QALAM_ERROR_OUT_OF_MEMORY;
        case E_INVALIDARG:
            return QALAM_ERROR_INVALID_ARGUMENT;
        case E_POINTER:
            return QALAM_ERROR_NULL_POINTER;
        case DWRITE_E_FILEFORMAT:
        case DWRITE_E_UNEXPECTED:
        case DWRITE_E_NOFONT:
        case DWRITE_E_FILENOTFOUND:
            return QALAM_ERROR_DIRECTWRITE_INIT;
        default:
            return QALAM_ERROR_UNKNOWN;
    }
}

void dwrite_log_error(HRESULT hr, const char* function, const char* message) {
    /* Simple stderr logging for now - can be replaced with proper logging */
    fprintf(stderr, "[DirectWrite Error] %s: %s (HRESULT: 0x%08lX)\n", 
            function, message, (unsigned long)hr);
}

DWriteContext* dwrite_get_context(void) {
    return &g_dwrite_context;
}

/*=============================================================================
 * Factory Lifecycle (Singleton Pattern)
 *============================================================================*/

QalamResult qalam_dwrite_init(void) {
    HRESULT hr;
    
    /* Ensure critical section is initialized */
    ensure_cs_initialized();
    
    EnterCriticalSection(&g_dwrite_context.init_lock);
    
    /* Check if already initialized */
    if (g_dwrite_context.ref_count > 0) {
        g_dwrite_context.ref_count++;
        LeaveCriticalSection(&g_dwrite_context.init_lock);
        return QALAM_OK;
    }
    
    /* Initialize COM for DirectWrite (apartment-threaded) */
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != S_FALSE && hr != RPC_E_CHANGED_MODE) {
        dwrite_log_error(hr, "qalam_dwrite_init", "Failed to initialize COM");
        LeaveCriticalSection(&g_dwrite_context.init_lock);
        return QALAM_ERROR_D2D_INIT;
    }
    
    /* Create DirectWrite factory */
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        &IID_IDWriteFactory,
        (IUnknown**)&g_dwrite_context.factory
    );
    
    if (FAILED(hr) || g_dwrite_context.factory == NULL) {
        dwrite_log_error(hr, "qalam_dwrite_init", "Failed to create DirectWrite factory");
        CoUninitialize();
        LeaveCriticalSection(&g_dwrite_context.init_lock);
        return QALAM_ERROR_DIRECTWRITE_INIT;
    }
    
    /* Get system font collection */
    hr = g_dwrite_context.factory->lpVtbl->GetSystemFontCollection(
        g_dwrite_context.factory,
        &g_dwrite_context.system_fonts,
        FALSE  /* checkForUpdates */
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_init", "Failed to get system font collection");
        /* Non-fatal - continue without system fonts cached */
        g_dwrite_context.system_fonts = NULL;
    }
    
    /* Create Direct2D factory */
    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &IID_ID2D1Factory,
        NULL,  /* factory options */
        (void**)&g_dwrite_context.d2d_factory
    );
    
    if (FAILED(hr) || g_dwrite_context.d2d_factory == NULL) {
        dwrite_log_error(hr, "qalam_dwrite_init", "Failed to create Direct2D factory");
        /* Release DirectWrite resources on failure */
        if (g_dwrite_context.system_fonts) {
            g_dwrite_context.system_fonts->lpVtbl->Release(g_dwrite_context.system_fonts);
            g_dwrite_context.system_fonts = NULL;
        }
        g_dwrite_context.factory->lpVtbl->Release(g_dwrite_context.factory);
        g_dwrite_context.factory = NULL;
        CoUninitialize();
        LeaveCriticalSection(&g_dwrite_context.init_lock);
        return QALAM_ERROR_D2D_INIT;
    }
    
    g_dwrite_context.initialized = true;
    g_dwrite_context.ref_count = 1;
    
    LeaveCriticalSection(&g_dwrite_context.init_lock);
    return QALAM_OK;
}

void qalam_dwrite_shutdown(void) {
    ensure_cs_initialized();
    
    EnterCriticalSection(&g_dwrite_context.init_lock);
    
    if (g_dwrite_context.ref_count <= 0) {
        LeaveCriticalSection(&g_dwrite_context.init_lock);
        return;
    }
    
    g_dwrite_context.ref_count--;
    
    if (g_dwrite_context.ref_count == 0 && g_dwrite_context.initialized) {
        /* Release Direct2D factory */
        if (g_dwrite_context.d2d_factory) {
            g_dwrite_context.d2d_factory->lpVtbl->Release(g_dwrite_context.d2d_factory);
            g_dwrite_context.d2d_factory = NULL;
        }
        
        /* Release system font collection */
        if (g_dwrite_context.system_fonts) {
            g_dwrite_context.system_fonts->lpVtbl->Release(g_dwrite_context.system_fonts);
            g_dwrite_context.system_fonts = NULL;
        }
        
        /* Release DirectWrite factory */
        if (g_dwrite_context.factory) {
            g_dwrite_context.factory->lpVtbl->Release(g_dwrite_context.factory);
            g_dwrite_context.factory = NULL;
        }
        
        /* Uninitialize COM */
        CoUninitialize();
        
        g_dwrite_context.initialized = false;
    }
    
    LeaveCriticalSection(&g_dwrite_context.init_lock);
}

IDWriteFactory* qalam_dwrite_get_factory(void) {
    if (!g_dwrite_context.initialized) {
        return NULL;
    }
    return g_dwrite_context.factory;
}

ID2D1Factory* qalam_dwrite_get_d2d_factory(void) {
    if (!g_dwrite_context.initialized) {
        return NULL;
    }
    return g_dwrite_context.d2d_factory;
}

/*=============================================================================
 * Text Format Management
 *============================================================================*/

QalamResult qalam_dwrite_create_text_format(
    const wchar_t* font_family,
    float font_size,
    DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STYLE style,
    IDWriteTextFormat** out_format)
{
    HRESULT hr;
    
    if (!font_family || !out_format) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!g_dwrite_context.initialized || !g_dwrite_context.factory) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    *out_format = NULL;
    
    /* Create text format with default locale (en-US) */
    hr = g_dwrite_context.factory->lpVtbl->CreateTextFormat(
        g_dwrite_context.factory,
        font_family,
        NULL,  /* Use system font collection */
        weight,
        style,
        DWRITE_FONT_STRETCH_NORMAL,
        font_size,
        L"en-US",
        out_format
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_create_text_format", 
                         "Failed to create text format");
        return dwrite_hresult_to_result(hr);
    }
    
    return QALAM_OK;
}

QalamResult qalam_dwrite_create_arabic_text_format(
    const wchar_t* font_family,
    float font_size,
    IDWriteTextFormat** out_format)
{
    HRESULT hr;
    
    if (!font_family || !out_format) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!g_dwrite_context.initialized || !g_dwrite_context.factory) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    *out_format = NULL;
    
    /* Create text format with Arabic locale */
    hr = g_dwrite_context.factory->lpVtbl->CreateTextFormat(
        g_dwrite_context.factory,
        font_family,
        NULL,  /* Use system font collection */
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        font_size,
        L"ar",  /* Arabic locale for proper shaping */
        out_format
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_create_arabic_text_format", 
                         "Failed to create Arabic text format");
        return dwrite_hresult_to_result(hr);
    }
    
    /* Set reading direction to RTL */
    hr = (*out_format)->lpVtbl->SetReadingDirection(
        *out_format,
        DWRITE_READING_DIRECTION_RIGHT_TO_LEFT
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_create_arabic_text_format", 
                         "Failed to set RTL reading direction");
        (*out_format)->lpVtbl->Release(*out_format);
        *out_format = NULL;
        return dwrite_hresult_to_result(hr);
    }
    
    /* Set paragraph alignment */
    hr = (*out_format)->lpVtbl->SetParagraphAlignment(
        *out_format,
        DWRITE_PARAGRAPH_ALIGNMENT_NEAR
    );
    
    if (FAILED(hr)) {
        /* Non-fatal - continue with default alignment */
        dwrite_log_error(hr, "qalam_dwrite_create_arabic_text_format", 
                         "Failed to set paragraph alignment (non-fatal)");
    }
    
    /* Set text alignment to trailing (right side in RTL) */
    hr = (*out_format)->lpVtbl->SetTextAlignment(
        *out_format,
        DWRITE_TEXT_ALIGNMENT_TRAILING
    );
    
    if (FAILED(hr)) {
        /* Non-fatal - continue with default alignment */
        dwrite_log_error(hr, "qalam_dwrite_create_arabic_text_format", 
                         "Failed to set text alignment (non-fatal)");
    }
    
    return QALAM_OK;
}

/*=============================================================================
 * Text Layout Creation
 *============================================================================*/

QalamResult qalam_dwrite_create_text_layout(
    const wchar_t* text,
    size_t text_length,
    IDWriteTextFormat* format,
    float max_width,
    float max_height,
    IDWriteTextLayout** out_layout)
{
    HRESULT hr;
    
    if (!text || !format || !out_layout) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!g_dwrite_context.initialized || !g_dwrite_context.factory) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    *out_layout = NULL;
    
    /* Create text layout */
    hr = g_dwrite_context.factory->lpVtbl->CreateTextLayout(
        g_dwrite_context.factory,
        text,
        (UINT32)text_length,
        format,
        max_width,
        max_height,
        out_layout
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_create_text_layout", 
                         "Failed to create text layout");
        return dwrite_hresult_to_result(hr);
    }
    
    return QALAM_OK;
}

QalamResult qalam_dwrite_create_rtl_text_layout(
    const wchar_t* text,
    size_t text_length,
    IDWriteTextFormat* format,
    float max_width,
    float max_height,
    IDWriteTextLayout** out_layout)
{
    HRESULT hr;
    QalamResult result;
    
    /* First create a standard layout */
    result = qalam_dwrite_create_text_layout(
        text, text_length, format, max_width, max_height, out_layout
    );
    
    if (result != QALAM_OK) {
        return result;
    }
    
    /* Set reading direction to RTL on the layout */
    hr = (*out_layout)->lpVtbl->SetReadingDirection(
        *out_layout,
        DWRITE_READING_DIRECTION_RIGHT_TO_LEFT
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_create_rtl_text_layout", 
                         "Failed to set RTL reading direction on layout");
        (*out_layout)->lpVtbl->Release(*out_layout);
        *out_layout = NULL;
        return dwrite_hresult_to_result(hr);
    }
    
    /* Set flow direction to RTL */
    hr = (*out_layout)->lpVtbl->SetFlowDirection(
        *out_layout,
        DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT
    );
    
    if (FAILED(hr)) {
        /* Non-fatal - some systems may not support this */
        dwrite_log_error(hr, "qalam_dwrite_create_rtl_text_layout", 
                         "Failed to set flow direction (non-fatal)");
    }
    
    return QALAM_OK;
}

/*=============================================================================
 * Text Measurement
 *============================================================================*/

QalamResult qalam_dwrite_measure_text(
    IDWriteTextLayout* layout,
    DWRITE_TEXT_METRICS* out_metrics)
{
    HRESULT hr;
    
    if (!layout || !out_metrics) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Get text metrics */
    hr = layout->lpVtbl->GetMetrics(layout, out_metrics);
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_measure_text", 
                         "Failed to get text metrics");
        return dwrite_hresult_to_result(hr);
    }
    
    return QALAM_OK;
}

QalamResult qalam_dwrite_hit_test_point(
    IDWriteTextLayout* layout,
    float x,
    float y,
    DWRITE_HIT_TEST_METRICS* out_metrics,
    BOOL* is_trailing,
    BOOL* is_inside)
{
    HRESULT hr;
    
    if (!layout || !out_metrics || !is_trailing || !is_inside) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Perform hit test from point coordinates */
    hr = layout->lpVtbl->HitTestPoint(
        layout,
        x,
        y,
        is_trailing,
        is_inside,
        out_metrics
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_hit_test_point", 
                         "Failed to hit test point");
        return dwrite_hresult_to_result(hr);
    }
    
    return QALAM_OK;
}

QalamResult qalam_dwrite_hit_test_position(
    IDWriteTextLayout* layout,
    uint32_t position,
    BOOL is_trailing,
    float* out_x,
    float* out_y,
    DWRITE_HIT_TEST_METRICS* out_metrics)
{
    HRESULT hr;
    DWRITE_HIT_TEST_METRICS metrics;
    float x, y;
    
    if (!layout || !out_x || !out_y) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Perform hit test from text position */
    hr = layout->lpVtbl->HitTestTextPosition(
        layout,
        position,
        is_trailing,
        &x,
        &y,
        &metrics
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_dwrite_hit_test_position", 
                         "Failed to hit test text position");
        return dwrite_hresult_to_result(hr);
    }
    
    *out_x = x;
    *out_y = y;
    
    if (out_metrics) {
        *out_metrics = metrics;
    }
    
    return QALAM_OK;
}

/*=============================================================================
 * Font Fallback
 *============================================================================*/

QalamResult qalam_dwrite_setup_font_fallback(IDWriteTextFormat* format) {
    /*
     * Font fallback configuration for Arabic + Latin mixed text.
     * 
     * DirectWrite 1.2+ (Windows 8.1+) provides IDWriteFontFallbackBuilder
     * for custom font fallback chains. For Windows 10 1903+ (our minimum
     * target), we can use this API.
     * 
     * Primary fallback chain:
     * 1. Cascadia Code - Primary monospace font with good Unicode coverage
     * 2. Amiri - High-quality Arabic typographic font
     * 3. Noto Sans Arabic - Google's Arabic font with excellent coverage
     * 4. Segoe UI - System fallback for remaining characters
     * 
     * For now, we rely on DirectWrite's built-in font fallback mechanism
     * which handles most cases well. A custom fallback can be implemented
     * later if needed for specific Arabic script requirements.
     */
    
    if (!format) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!g_dwrite_context.initialized) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    /* 
     * TODO: Implement custom font fallback using IDWriteFontFallbackBuilder
     * 
     * The implementation would involve:
     * 1. Query for IDWriteFactory2 interface
     * 2. Get system font fallback using GetSystemFontFallback()
     * 3. Create IDWriteFontFallbackBuilder
     * 4. Add mappings for Arabic Unicode ranges to preferred fonts
     * 5. Create fallback and apply to text layout
     * 
     * For now, DirectWrite's built-in fallback handles Arabic text well
     * when using fonts that support Arabic (like Cascadia Code or Segoe UI).
     */
    
    return QALAM_OK;
}