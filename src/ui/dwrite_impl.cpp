/**
 * @file dwrite_impl.cpp
 * @brief Qalam IDE - DirectWrite C++ Implementation
 * 
 * This file is compiled as C++ to use DirectWrite/Direct2D COM APIs.
 * It exposes a pure C interface defined in dwrite_api.h.
 * 
 * All COM interactions are handled here using ComPtr for automatic
 * reference counting. The rest of the codebase remains pure C.
 * 
 * @version 0.0.2
 * @copyright (c) 2026 Qalam Project
 */

// Prevent Windows.h from defining min/max macros
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>  // For ComPtr smart pointers

#include <cstdio>
#include <cstdlib>
#include <mutex>

// Include our C header - extern "C" is already in the header
#include "dwrite_api.h"

using Microsoft::WRL::ComPtr;

/* ============================================================================
 * Opaque Handle Structures
 * ============================================================================ */

/**
 * @brief DirectWrite text format wrapper
 */
struct QalamDWriteTextFormat {
    ComPtr<IDWriteTextFormat> format;
    bool is_rtl;
    
    QalamDWriteTextFormat() : is_rtl(false) {}
};

/**
 * @brief DirectWrite text layout wrapper
 */
struct QalamDWriteTextLayout {
    ComPtr<IDWriteTextLayout> layout;
    bool is_rtl;
    
    QalamDWriteTextLayout() : is_rtl(false) {}
};

/**
 * @brief Direct2D render target wrapper
 */
struct QalamDWriteRenderTarget {
    ComPtr<ID2D1HwndRenderTarget> target;
    HWND hwnd;
    
    QalamDWriteRenderTarget() : hwnd(nullptr) {}
};

/**
 * @brief Direct2D brush wrapper
 */
struct QalamDWriteBrush {
    ComPtr<ID2D1SolidColorBrush> brush;
};

/* ============================================================================
 * Global Singleton State
 * ============================================================================ */

namespace {

/**
 * @brief Global DirectWrite/D2D context
 */
struct DWriteGlobals {
    ComPtr<ID2D1Factory> d2d_factory;
    ComPtr<IDWriteFactory> dwrite_factory;
    ComPtr<IDWriteFontCollection> system_fonts;
    bool initialized;
    int ref_count;
    std::mutex init_mutex;
    
    DWriteGlobals() : initialized(false), ref_count(0) {}
};

// Global singleton instance
DWriteGlobals g_dwrite;

/**
 * @brief Convert HRESULT to QalamResult
 */
inline QalamResult hr_to_result(HRESULT hr) {
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
        case D2DERR_RECREATE_TARGET:
            return QALAM_ERROR_RENDER_TARGET;
        default:
            if (HRESULT_FACILITY(hr) == FACILITY_DWRITE) {
                return QALAM_ERROR_DIRECTWRITE_INIT;
            }
            return QALAM_ERROR_UNKNOWN;
    }
}

/**
 * @brief Log error for debugging
 */
void log_error(HRESULT hr, const char* function, const char* message) {
    fprintf(stderr, "[DirectWrite Error] %s: %s (HRESULT: 0x%08lX)\n",
            function, message, static_cast<unsigned long>(hr));
}

/**
 * @brief Convert QalamDWriteColor to D2D1_COLOR_F
 */
inline D2D1_COLOR_F to_d2d_color(QalamDWriteColor color) {
    return D2D1::ColorF(color.r, color.g, color.b, color.a);
}

} // anonymous namespace

/* ============================================================================
 * DirectWrite Context (Singleton-style)
 * ============================================================================ */

extern "C" QalamResult qalam_dwrite_init(void) {
    std::lock_guard<std::mutex> lock(g_dwrite.init_mutex);
    
    // Check if already initialized (increment ref count)
    if (g_dwrite.ref_count > 0) {
        g_dwrite.ref_count++;
        return QALAM_OK;
    }
    
    HRESULT hr;
    
    // Initialize COM (apartment-threaded)
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != S_FALSE && hr != RPC_E_CHANGED_MODE) {
        log_error(hr, "qalam_dwrite_init", "Failed to initialize COM");
        return QALAM_ERROR_D2D_INIT;
    }
    
    // Create Direct2D factory
    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        g_dwrite.d2d_factory.ReleaseAndGetAddressOf()
    );
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_init", "Failed to create D2D factory");
        CoUninitialize();
        return QALAM_ERROR_D2D_INIT;
    }
    
    // Create DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(g_dwrite.dwrite_factory.ReleaseAndGetAddressOf())
    );
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_init", "Failed to create DirectWrite factory");
        g_dwrite.d2d_factory.Reset();
        CoUninitialize();
        return QALAM_ERROR_DIRECTWRITE_INIT;
    }
    
    // Get system font collection
    hr = g_dwrite.dwrite_factory->GetSystemFontCollection(
        g_dwrite.system_fonts.ReleaseAndGetAddressOf(),
        FALSE  // checkForUpdates
    );
    
    if (FAILED(hr)) {
        // Non-fatal - continue without system fonts cached
        log_error(hr, "qalam_dwrite_init", "Failed to get system font collection (non-fatal)");
    }
    
    g_dwrite.initialized = true;
    g_dwrite.ref_count = 1;
    
    return QALAM_OK;
}

extern "C" void qalam_dwrite_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_dwrite.init_mutex);
    
    if (g_dwrite.ref_count <= 0) {
        return;
    }
    
    g_dwrite.ref_count--;
    
    if (g_dwrite.ref_count == 0 && g_dwrite.initialized) {
        // Release all resources
        g_dwrite.system_fonts.Reset();
        g_dwrite.dwrite_factory.Reset();
        g_dwrite.d2d_factory.Reset();
        
        // Uninitialize COM
        CoUninitialize();
        
        g_dwrite.initialized = false;
    }
}

extern "C" bool qalam_dwrite_is_initialized(void) {
    return g_dwrite.initialized;
}

/* ============================================================================
 * Text Format Management
 * ============================================================================ */

extern "C" QalamResult qalam_dwrite_text_format_create(
    const QalamDWriteFontParams* params,
    QalamDWriteTextFormat** out_format)
{
    if (!params || !out_format || !params->family) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!g_dwrite.initialized) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    *out_format = nullptr;
    
    // Map font weight
    DWRITE_FONT_WEIGHT weight = static_cast<DWRITE_FONT_WEIGHT>(params->weight);
    
    // Map font style
    DWRITE_FONT_STYLE style;
    switch (params->style) {
        case QALAM_DWRITE_FONT_STYLE_ITALIC:
            style = DWRITE_FONT_STYLE_ITALIC;
            break;
        case QALAM_DWRITE_FONT_STYLE_OBLIQUE:
            style = DWRITE_FONT_STYLE_OBLIQUE;
            break;
        default:
            style = DWRITE_FONT_STYLE_NORMAL;
            break;
    }
    
    // Choose locale based on RTL setting
    const wchar_t* locale = params->is_rtl ? L"ar" : L"en-US";
    
    // Create text format
    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = g_dwrite.dwrite_factory->CreateTextFormat(
        params->family,
        nullptr,  // Use system font collection
        weight,
        style,
        DWRITE_FONT_STRETCH_NORMAL,
        params->size,
        locale,
        format.GetAddressOf()
    );
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_text_format_create", "Failed to create text format");
        return hr_to_result(hr);
    }
    
    // Configure RTL if requested
    if (params->is_rtl) {
        hr = format->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
        if (FAILED(hr)) {
            log_error(hr, "qalam_dwrite_text_format_create", "Failed to set RTL direction");
            return hr_to_result(hr);
        }
        
        // Set text alignment to trailing (right side for RTL)
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }
    
    // Create wrapper structure
    auto* result = new (std::nothrow) QalamDWriteTextFormat();
    if (!result) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    result->format = std::move(format);
    result->is_rtl = params->is_rtl;
    
    *out_format = result;
    return QALAM_OK;
}

extern "C" QalamResult qalam_dwrite_text_format_create_arabic(
    const wchar_t* font_family,
    float font_size,
    QalamDWriteTextFormat** out_format)
{
    QalamDWriteFontParams params = {};
    params.family = font_family;
    params.size = font_size;
    params.weight = QALAM_DWRITE_FONT_WEIGHT_NORMAL;
    params.style = QALAM_DWRITE_FONT_STYLE_NORMAL;
    params.is_rtl = true;
    
    return qalam_dwrite_text_format_create(&params, out_format);
}

extern "C" void qalam_dwrite_text_format_destroy(QalamDWriteTextFormat* format) {
    delete format;
}

/* ============================================================================
 * Text Layout Management
 * ============================================================================ */

extern "C" QalamResult qalam_dwrite_text_layout_create(
    const wchar_t* text,
    uint32_t text_length,
    QalamDWriteTextFormat* format,
    float max_width,
    float max_height,
    QalamDWriteTextLayout** out_layout)
{
    if (!text || !format || !out_layout) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!g_dwrite.initialized) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    *out_layout = nullptr;
    
    // Create text layout
    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = g_dwrite.dwrite_factory->CreateTextLayout(
        text,
        text_length,
        format->format.Get(),
        max_width,
        max_height,
        layout.GetAddressOf()
    );
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_text_layout_create", "Failed to create text layout");
        return hr_to_result(hr);
    }
    
    // Configure RTL on layout if format is RTL
    if (format->is_rtl) {
        layout->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
        layout->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
    }
    
    // Create wrapper structure
    auto* result = new (std::nothrow) QalamDWriteTextLayout();
    if (!result) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    result->layout = std::move(layout);
    result->is_rtl = format->is_rtl;
    
    *out_layout = result;
    return QALAM_OK;
}

extern "C" void qalam_dwrite_text_layout_destroy(QalamDWriteTextLayout* layout) {
    delete layout;
}

extern "C" QalamResult qalam_dwrite_text_layout_get_metrics(
    QalamDWriteTextLayout* layout,
    QalamDWriteTextMetrics* out_metrics)
{
    if (!layout || !out_metrics) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    DWRITE_TEXT_METRICS metrics;
    HRESULT hr = layout->layout->GetMetrics(&metrics);
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_text_layout_get_metrics", "Failed to get metrics");
        return hr_to_result(hr);
    }
    
    out_metrics->left = metrics.left;
    out_metrics->top = metrics.top;
    out_metrics->width = metrics.width;
    out_metrics->height = metrics.height;
    out_metrics->layout_width = metrics.layoutWidth;
    out_metrics->layout_height = metrics.layoutHeight;
    out_metrics->line_count = metrics.lineCount;
    
    return QALAM_OK;
}

extern "C" QalamResult qalam_dwrite_text_layout_hit_test_point(
    QalamDWriteTextLayout* layout,
    float x,
    float y,
    QalamDWriteHitTestResult* out_result)
{
    if (!layout || !out_result) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    BOOL is_trailing = FALSE;
    BOOL is_inside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics;
    
    HRESULT hr = layout->layout->HitTestPoint(x, y, &is_trailing, &is_inside, &metrics);
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_text_layout_hit_test_point", "Failed to hit test point");
        return hr_to_result(hr);
    }
    
    out_result->text_position = metrics.textPosition;
    out_result->length = metrics.length;
    out_result->left = metrics.left;
    out_result->top = metrics.top;
    out_result->width = metrics.width;
    out_result->height = metrics.height;
    out_result->is_trailing_hit = (is_trailing != FALSE);
    out_result->is_inside = (is_inside != FALSE);
    
    return QALAM_OK;
}

extern "C" QalamResult qalam_dwrite_text_layout_hit_test_position(
    QalamDWriteTextLayout* layout,
    uint32_t text_position,
    bool is_trailing,
    float* out_x,
    float* out_y,
    QalamDWriteHitTestResult* out_result)
{
    if (!layout || !out_x || !out_y) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    float x, y;
    DWRITE_HIT_TEST_METRICS metrics;
    
    HRESULT hr = layout->layout->HitTestTextPosition(
        text_position,
        is_trailing ? TRUE : FALSE,
        &x,
        &y,
        &metrics
    );
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_text_layout_hit_test_position", "Failed to hit test position");
        return hr_to_result(hr);
    }
    
    *out_x = x;
    *out_y = y;
    
    if (out_result) {
        out_result->text_position = metrics.textPosition;
        out_result->length = metrics.length;
        out_result->left = metrics.left;
        out_result->top = metrics.top;
        out_result->width = metrics.width;
        out_result->height = metrics.height;
        out_result->is_trailing_hit = is_trailing;
        out_result->is_inside = true;
    }
    
    return QALAM_OK;
}

/* ============================================================================
 * Render Target Management
 * ============================================================================ */

extern "C" QalamResult qalam_dwrite_render_target_create(
    void* hwnd,
    QalamDWriteRenderTarget** out_target)
{
    if (!hwnd || !out_target) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    if (!g_dwrite.initialized) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    *out_target = nullptr;
    
    HWND window = static_cast<HWND>(hwnd);
    
    // Get client area dimensions
    RECT rc;
    if (!GetClientRect(window, &rc)) {
        return QALAM_ERROR_WINDOW_CREATE;
    }
    
    D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT32>(rc.right - rc.left),
        static_cast<UINT32>(rc.bottom - rc.top)
    );
    
    // Ensure minimum size
    if (size.width == 0) size.width = 1;
    if (size.height == 0) size.height = 1;
    
    // Create render target properties
    D2D1_RENDER_TARGET_PROPERTIES rt_props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0.0f, 0.0f,  // Default DPI
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
    );
    
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_props = D2D1::HwndRenderTargetProperties(
        window,
        size,
        D2D1_PRESENT_OPTIONS_NONE
    );
    
    // Create HWND render target
    ComPtr<ID2D1HwndRenderTarget> target;
    HRESULT hr = g_dwrite.d2d_factory->CreateHwndRenderTarget(
        rt_props,
        hwnd_rt_props,
        target.GetAddressOf()
    );
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_render_target_create", "Failed to create render target");
        return QALAM_ERROR_RENDER_TARGET;
    }
    
    // Create wrapper structure
    auto* result = new (std::nothrow) QalamDWriteRenderTarget();
    if (!result) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    result->target = std::move(target);
    result->hwnd = window;
    
    *out_target = result;
    return QALAM_OK;
}

extern "C" QalamResult qalam_dwrite_render_target_resize(
    QalamDWriteRenderTarget* target,
    uint32_t width,
    uint32_t height)
{
    if (!target) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    // Ensure minimum size
    if (width == 0) width = 1;
    if (height == 0) height = 1;
    
    D2D1_SIZE_U size = D2D1::SizeU(width, height);
    
    HRESULT hr = target->target->Resize(size);
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_render_target_resize", "Failed to resize render target");
        
        if (hr == D2DERR_RECREATE_TARGET) {
            return QALAM_ERROR_RENDER_TARGET;
        }
        
        return hr_to_result(hr);
    }
    
    return QALAM_OK;
}

extern "C" void qalam_dwrite_render_target_destroy(QalamDWriteRenderTarget* target) {
    delete target;
}

extern "C" void qalam_dwrite_render_target_get_dpi(
    QalamDWriteRenderTarget* target,
    float* out_dpi_x,
    float* out_dpi_y)
{
    if (!target || !out_dpi_x || !out_dpi_y) {
        return;
    }
    
    target->target->GetDpi(out_dpi_x, out_dpi_y);
}

/* ============================================================================
 * Brush Management
 * ============================================================================ */

extern "C" QalamResult qalam_dwrite_brush_create_solid(
    QalamDWriteRenderTarget* target,
    QalamDWriteColor color,
    QalamDWriteBrush** out_brush)
{
    if (!target || !out_brush) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    *out_brush = nullptr;
    
    ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = target->target->CreateSolidColorBrush(
        to_d2d_color(color),
        brush.GetAddressOf()
    );
    
    if (FAILED(hr)) {
        log_error(hr, "qalam_dwrite_brush_create_solid", "Failed to create brush");
        return hr_to_result(hr);
    }
    
    // Create wrapper structure
    auto* result = new (std::nothrow) QalamDWriteBrush();
    if (!result) {
        return QALAM_ERROR_OUT_OF_MEMORY;
    }
    
    result->brush = std::move(brush);
    
    *out_brush = result;
    return QALAM_OK;
}

extern "C" void qalam_dwrite_brush_destroy(QalamDWriteBrush* brush) {
    delete brush;
}

extern "C" void qalam_dwrite_brush_set_color(QalamDWriteBrush* brush, QalamDWriteColor color) {
    if (!brush) {
        return;
    }
    
    brush->brush->SetColor(to_d2d_color(color));
}

/* ============================================================================
 * Rendering Operations
 * ============================================================================ */

extern "C" void qalam_dwrite_render_begin(QalamDWriteRenderTarget* target) {
    if (!target) {
        return;
    }
    
    target->target->BeginDraw();
}

extern "C" QalamResult qalam_dwrite_render_end(QalamDWriteRenderTarget* target) {
    if (!target) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    HRESULT hr = target->target->EndDraw();
    
    if (FAILED(hr)) {
        if (hr == D2DERR_RECREATE_TARGET) {
            log_error(hr, "qalam_dwrite_render_end", "Device lost - recreate target");
            return QALAM_ERROR_RENDER_TARGET;
        }
        
        log_error(hr, "qalam_dwrite_render_end", "EndDraw failed");
        return hr_to_result(hr);
    }
    
    return QALAM_OK;
}

extern "C" void qalam_dwrite_render_clear(QalamDWriteRenderTarget* target, QalamDWriteColor color) {
    if (!target) {
        return;
    }
    
    target->target->Clear(to_d2d_color(color));
}

extern "C" void qalam_dwrite_render_draw_text(
    QalamDWriteRenderTarget* target,
    QalamDWriteTextLayout* layout,
    float x,
    float y,
    QalamDWriteBrush* brush)
{
    if (!target || !layout || !brush) {
        return;
    }
    
    target->target->DrawTextLayout(
        D2D1::Point2F(x, y),
        layout->layout.Get(),
        brush->brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
    );
}

extern "C" void qalam_dwrite_render_draw_rect(
    QalamDWriteRenderTarget* target,
    float x,
    float y,
    float width,
    float height,
    QalamDWriteBrush* brush,
    bool filled)
{
    if (!target || !brush) {
        return;
    }
    
    D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
    
    if (filled) {
        target->target->FillRectangle(rect, brush->brush.Get());
    } else {
        target->target->DrawRectangle(rect, brush->brush.Get(), 1.0f);
    }
}

extern "C" void qalam_dwrite_render_draw_line(
    QalamDWriteRenderTarget* target,
    float x1,
    float y1,
    float x2,
    float y2,
    QalamDWriteBrush* brush,
    float stroke_width)
{
    if (!target || !brush) {
        return;
    }
    
    target->target->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2),
        brush->brush.Get(),
        stroke_width
    );
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

extern "C" QalamDWriteColor qalam_dwrite_color_from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    QalamDWriteColor color;
    color.r = static_cast<float>(r) / 255.0f;
    color.g = static_cast<float>(g) / 255.0f;
    color.b = static_cast<float>(b) / 255.0f;
    color.a = static_cast<float>(a) / 255.0f;
    return color;
}

extern "C" QalamDWriteColor qalam_dwrite_color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return qalam_dwrite_color_from_rgba(r, g, b, 255);
}

extern "C" QalamDWriteColor qalam_dwrite_color_from_hex(uint32_t hex) {
    return qalam_dwrite_color_from_rgb(
        static_cast<uint8_t>((hex >> 16) & 0xFF),
        static_cast<uint8_t>((hex >> 8) & 0xFF),
        static_cast<uint8_t>(hex & 0xFF)
    );
}