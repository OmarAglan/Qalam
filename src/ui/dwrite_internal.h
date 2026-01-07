/**
 * @file dwrite.h
 * @brief Qalam IDE - DirectWrite Internal Header
 * 
 * Internal header for DirectWrite implementation details.
 * This file contains the singleton context and internal helpers
 * for DirectWrite text rendering with Arabic RTL support.
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 */

#ifndef QALAM_DWRITE_INTERNAL_H
#define QALAM_DWRITE_INTERNAL_H

/* 
 * For C interface to COM objects, we need COBJMACROS to access lpVtbl
 * This must be defined before including any Windows/COM headers
 */
#ifndef COBJMACROS
#define COBJMACROS
#endif

/* Windows headers */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>

/* DirectWrite and Direct2D - these provide C interfaces when COBJMACROS is defined */
#include <dwrite.h>
#include <d2d1.h>

/* Our main header for result types */
#include "qalam.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * DirectWrite Singleton Context
 *============================================================================*/

/**
 * @brief DirectWrite singleton state
 * 
 * Manages the global DirectWrite factory and system font collection.
 * Initialized once at startup and released on shutdown.
 */
typedef struct DWriteContext {
    IDWriteFactory* factory;              /**< DirectWrite factory instance */
    IDWriteFontCollection* system_fonts;  /**< System font collection */
    ID2D1Factory* d2d_factory;            /**< Direct2D factory for rendering */
    int initialized;                      /**< Initialization flag (use int, not bool for C compat) */
    CRITICAL_SECTION init_lock;           /**< Lock for thread-safe initialization */
    LONG ref_count;                       /**< Reference count for factory */
} DWriteContext;

/**
 * @brief Get the singleton DirectWrite context
 * 
 * Returns a pointer to the global DirectWrite context.
 * The context must be initialized via qalam_dwrite_init() before use.
 * 
 * @return Pointer to the singleton context (never NULL)
 */
DWriteContext* dwrite_get_context(void);

/*=============================================================================
 * Factory Lifecycle (Singleton Pattern)
 *============================================================================*/

/**
 * @brief Initialize DirectWrite factory
 * 
 * Initializes COM, creates the DirectWrite factory, and loads
 * the system font collection. Must be called once at startup
 * before any other DirectWrite operations.
 * 
 * Thread-safe: Can be called from multiple threads; only first call initializes.
 * 
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_init(void);

/**
 * @brief Shutdown and release all DirectWrite resources
 * 
 * Releases the DirectWrite factory, font collection, and
 * uninitializes COM. Should be called once at application shutdown.
 * 
 * Thread-safe: Uses reference counting to ensure proper cleanup.
 */
void qalam_dwrite_shutdown(void);

/**
 * @brief Get the singleton DirectWrite factory
 * 
 * Returns the DirectWrite factory for creating text formats and layouts.
 * Returns NULL if not initialized.
 * 
 * @return IDWriteFactory pointer or NULL if not initialized
 */
IDWriteFactory* qalam_dwrite_get_factory(void);

/**
 * @brief Get the singleton Direct2D factory
 * 
 * Returns the Direct2D factory for creating render targets and resources.
 * Returns NULL if not initialized.
 * 
 * @return ID2D1Factory pointer or NULL if not initialized
 */
ID2D1Factory* qalam_dwrite_get_d2d_factory(void);

/*=============================================================================
 * Text Format Management
 *============================================================================*/

/**
 * @brief Create a text format for specified font parameters
 * 
 * Creates an IDWriteTextFormat with the specified font family, size,
 * weight, and style. The format can be used to create text layouts.
 * 
 * @param font_family   Font family name (e.g., L"Cascadia Code")
 * @param font_size     Font size in DIPs (device-independent pixels)
 * @param weight        Font weight (e.g., DWRITE_FONT_WEIGHT_NORMAL)
 * @param style         Font style (e.g., DWRITE_FONT_STYLE_NORMAL)
 * @param[out] out_format   Pointer to receive the created text format
 * 
 * @return QALAM_OK on success, error code on failure
 * 
 * @note Caller is responsible for releasing the returned format
 */
QalamResult qalam_dwrite_create_text_format(
    const WCHAR* font_family,
    float font_size,
    DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STYLE style,
    IDWriteTextFormat** out_format
);

/**
 * @brief Create Arabic-optimized text format with RTL reading direction
 * 
 * Creates a text format configured for Arabic text rendering:
 * - Right-to-left reading direction
 * - Arabic locale for proper shaping
 * - Appropriate paragraph alignment
 * 
 * @param font_family   Font family name (e.g., L"Cascadia Code")
 * @param font_size     Font size in DIPs
 * @param[out] out_format   Pointer to receive the created text format
 * 
 * @return QALAM_OK on success, error code on failure
 * 
 * @note Caller is responsible for releasing the returned format
 */
QalamResult qalam_dwrite_create_arabic_text_format(
    const WCHAR* font_family,
    float font_size,
    IDWriteTextFormat** out_format
);

/*=============================================================================
 * Text Layout Creation
 *============================================================================*/

/**
 * @brief Create text layout for measuring and rendering
 * 
 * Creates an IDWriteTextLayout for the specified text and format.
 * The layout can be used for measuring text dimensions and rendering.
 * 
 * @param text          Text to layout (UTF-16)
 * @param text_length   Length of text in characters
 * @param format        Text format to use
 * @param max_width     Maximum layout width in DIPs
 * @param max_height    Maximum layout height in DIPs
 * @param[out] out_layout   Pointer to receive the created layout
 * 
 * @return QALAM_OK on success, error code on failure
 * 
 * @note Caller is responsible for releasing the returned layout
 */
QalamResult qalam_dwrite_create_text_layout(
    const WCHAR* text,
    UINT32 text_length,
    IDWriteTextFormat* format,
    float max_width,
    float max_height,
    IDWriteTextLayout** out_layout
);

/**
 * @brief Create RTL text layout for Arabic text
 * 
 * Creates a text layout with right-to-left reading direction
 * and flow direction configured for Arabic text.
 * 
 * @param text          Text to layout (UTF-16)
 * @param text_length   Length of text in characters
 * @param format        Text format to use (should be Arabic-optimized)
 * @param max_width     Maximum layout width in DIPs
 * @param max_height    Maximum layout height in DIPs
 * @param[out] out_layout   Pointer to receive the created layout
 * 
 * @return QALAM_OK on success, error code on failure
 * 
 * @note Caller is responsible for releasing the returned layout
 */
QalamResult qalam_dwrite_create_rtl_text_layout(
    const WCHAR* text,
    UINT32 text_length,
    IDWriteTextFormat* format,
    float max_width,
    float max_height,
    IDWriteTextLayout** out_layout
);

/*=============================================================================
 * Text Measurement
 *============================================================================*/

/**
 * @brief Get metrics for a text layout
 * 
 * Retrieves the text metrics including width, height, line count,
 * and other measurements for the specified layout.
 * 
 * @param layout        Source text layout
 * @param[out] out_metrics  Pointer to receive the metrics
 * 
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_measure_text(
    IDWriteTextLayout* layout,
    DWRITE_TEXT_METRICS* out_metrics
);

/**
 * @brief Get character position from x,y coordinates (hit testing)
 * 
 * Determines which character position corresponds to the given
 * x,y coordinates within the text layout. Used for cursor placement
 * from mouse clicks.
 * 
 * @param layout        Source text layout
 * @param x             X coordinate relative to layout origin
 * @param y             Y coordinate relative to layout origin
 * @param[out] out_metrics  Pointer to receive hit test metrics
 * @param[out] is_trailing  TRUE if hit is on trailing edge of character
 * @param[out] is_inside    TRUE if point is inside text bounds
 * 
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_hit_test_point(
    IDWriteTextLayout* layout,
    float x,
    float y,
    DWRITE_HIT_TEST_METRICS* out_metrics,
    BOOL* is_trailing,
    BOOL* is_inside
);

/**
 * @brief Get x,y coordinates from character position
 * 
 * Determines the x,y coordinates for a given character position
 * within the text layout. Used for cursor rendering.
 * 
 * @param layout        Source text layout
 * @param position      Character position (0-based)
 * @param is_trailing   TRUE for trailing edge, FALSE for leading edge
 * @param[out] out_x    Pointer to receive X coordinate
 * @param[out] out_y    Pointer to receive Y coordinate
 * @param[out] out_metrics  Pointer to receive hit test metrics (optional, can be NULL)
 * 
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_hit_test_position(
    IDWriteTextLayout* layout,
    UINT32 position,
    BOOL is_trailing,
    float* out_x,
    float* out_y,
    DWRITE_HIT_TEST_METRICS* out_metrics
);

/*=============================================================================
 * Font Fallback
 *============================================================================*/

/**
 * @brief Font fallback chain for Arabic + Latin mixed text
 * 
 * Primary chain: Cascadia Code → Amiri → Noto Sans Arabic → Segoe UI
 */
#define QALAM_FONT_FALLBACK_PRIMARY     L"Cascadia Code"
#define QALAM_FONT_FALLBACK_ARABIC_1    L"Amiri"
#define QALAM_FONT_FALLBACK_ARABIC_2    L"Noto Sans Arabic"
#define QALAM_FONT_FALLBACK_SYSTEM      L"Segoe UI"

/**
 * @brief Set up font fallback chain for Arabic + Latin mixed text
 * 
 * Configures font fallback to handle text that mixes Arabic and
 * Latin scripts, ensuring proper rendering of both.
 * 
 * @param format    Text format to configure with fallback
 * 
 * @return QALAM_OK on success, error code on failure
 * 
 * @note This is a placeholder for future font fallback implementation.
 *       DirectWrite 1.2+ provides IDWriteFontFallbackBuilder for custom fallback.
 */
QalamResult qalam_dwrite_setup_font_fallback(
    IDWriteTextFormat* format
);

/*=============================================================================
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Convert HRESULT to QalamResult
 * 
 * Maps common HRESULT values to appropriate QalamResult error codes.
 * 
 * @param hr    HRESULT value to convert
 * @return Corresponding QalamResult
 */
QalamResult dwrite_hresult_to_result(HRESULT hr);

/**
 * @brief Log DirectWrite error with details
 * 
 * Logs detailed error information for debugging DirectWrite issues.
 * 
 * @param hr        HRESULT error code
 * @param function  Function name where error occurred
 * @param message   Additional error message
 */
void dwrite_log_error(HRESULT hr, const char* function, const char* message);

/*=============================================================================
 * Render Target Management (from render.c)
 *============================================================================*/

/**
 * @brief Create D2D render target for a window
 */
QalamResult qalam_render_create_target(
    HWND hwnd,
    ID2D1HwndRenderTarget** out_target
);

/**
 * @brief Resize render target when window resizes
 */
QalamResult qalam_render_resize_target(
    ID2D1HwndRenderTarget* target,
    UINT width,
    UINT height
);

/**
 * @brief Release render target
 */
void qalam_render_destroy_target(ID2D1HwndRenderTarget* target);

/**
 * @brief Create solid color brush
 */
QalamResult qalam_render_create_brush(
    ID2D1RenderTarget* target,
    D2D1_COLOR_F color,
    ID2D1SolidColorBrush** out_brush
);

/**
 * @brief Draw text layout at position
 */
QalamResult qalam_render_draw_text_layout(
    ID2D1RenderTarget* target,
    IDWriteTextLayout* layout,
    float x,
    float y,
    ID2D1Brush* brush
);

/**
 * @brief Begin frame rendering
 */
void qalam_render_begin_frame(ID2D1RenderTarget* target);

/**
 * @brief End frame rendering (presents to screen)
 */
HRESULT qalam_render_end_frame(ID2D1RenderTarget* target);

/**
 * @brief Clear with background color
 */
void qalam_render_clear(ID2D1RenderTarget* target, D2D1_COLOR_F color);

/**
 * @brief Color helper - create D2D1_COLOR_F from RGBA bytes
 */
D2D1_COLOR_F qalam_color_from_rgba(BYTE r, BYTE g, BYTE b, BYTE a);

/**
 * @brief Color helper - create D2D1_COLOR_F from RGB bytes
 */
D2D1_COLOR_F qalam_color_from_rgb(BYTE r, BYTE g, BYTE b);

/**
 * @brief Color helper - create D2D1_COLOR_F from COLORREF
 */
D2D1_COLOR_F qalam_color_from_colorref(COLORREF colorref);

/**
 * @brief Color helper - create D2D1_COLOR_F from hex
 */
D2D1_COLOR_F qalam_color_from_hex(DWORD hex);

#ifdef __cplusplus
}
#endif

#endif /* QALAM_DWRITE_INTERNAL_H */