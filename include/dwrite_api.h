/**
 * @file dwrite_api.h
 * @brief Qalam IDE - Pure C DirectWrite API
 * 
 * This header provides a pure C interface to DirectWrite and Direct2D
 * functionality. The implementation is in C++ (dwrite_impl.cpp) but
 * this header exposes only C-compatible types and functions.
 * 
 * @version 0.0.2
 * @copyright (c) 2026 Qalam Project
 */

#ifndef QALAM_DWRITE_API_H
#define QALAM_DWRITE_API_H

#include "qalam.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Handle Types (hide C++ implementation details)
 * ============================================================================ */

/**
 * @brief Opaque handle to DirectWrite text format
 * 
 * Wraps IDWriteTextFormat and stores additional configuration.
 */
typedef struct QalamDWriteTextFormat QalamDWriteTextFormat;

/**
 * @brief Opaque handle to DirectWrite text layout
 * 
 * Wraps IDWriteTextLayout for text measurement and rendering.
 */
typedef struct QalamDWriteTextLayout QalamDWriteTextLayout;

/**
 * @brief Opaque handle to D2D render target
 * 
 * Wraps ID2D1HwndRenderTarget for window-based rendering.
 */
typedef struct QalamDWriteRenderTarget QalamDWriteRenderTarget;

/**
 * @brief Opaque handle to D2D brush
 * 
 * Wraps ID2D1SolidColorBrush for text and shape rendering.
 */
typedef struct QalamDWriteBrush QalamDWriteBrush;

/* ============================================================================
 * Text Metrics (C-compatible structure)
 * ============================================================================ */

/**
 * @brief Text layout metrics
 * 
 * Contains measurements for a laid-out text block.
 */
typedef struct QalamDWriteTextMetrics {
    float left;             /**< Left edge of layout box */
    float top;              /**< Top edge of layout box */
    float width;            /**< Width of text content */
    float height;           /**< Height of text content */
    float layout_width;     /**< Maximum layout width */
    float layout_height;    /**< Maximum layout height */
    uint32_t line_count;    /**< Number of lines */
} QalamDWriteTextMetrics;

/**
 * @brief Hit test result structure
 * 
 * Contains information about a hit test operation.
 */
typedef struct QalamDWriteHitTestResult {
    uint32_t text_position; /**< Character position in text */
    uint32_t length;        /**< Length of the hit character */
    float left;             /**< Left edge of character box */
    float top;              /**< Top edge of character box */
    float width;            /**< Width of character box */
    float height;           /**< Height of character box */
    bool is_trailing_hit;   /**< True if hit is on trailing edge */
    bool is_inside;         /**< True if point is inside text bounds */
} QalamDWriteHitTestResult;

/* ============================================================================
 * Font Configuration
 * ============================================================================ */

/**
 * @brief Font weight enumeration
 */
typedef enum QalamDWriteFontWeight {
    QALAM_DWRITE_FONT_WEIGHT_THIN = 100,
    QALAM_DWRITE_FONT_WEIGHT_EXTRA_LIGHT = 200,
    QALAM_DWRITE_FONT_WEIGHT_LIGHT = 300,
    QALAM_DWRITE_FONT_WEIGHT_SEMI_LIGHT = 350,
    QALAM_DWRITE_FONT_WEIGHT_NORMAL = 400,
    QALAM_DWRITE_FONT_WEIGHT_MEDIUM = 500,
    QALAM_DWRITE_FONT_WEIGHT_SEMI_BOLD = 600,
    QALAM_DWRITE_FONT_WEIGHT_BOLD = 700,
    QALAM_DWRITE_FONT_WEIGHT_EXTRA_BOLD = 800,
    QALAM_DWRITE_FONT_WEIGHT_BLACK = 900
} QalamDWriteFontWeight;

/**
 * @brief Font style enumeration
 */
typedef enum QalamDWriteFontStyle {
    QALAM_DWRITE_FONT_STYLE_NORMAL = 0,
    QALAM_DWRITE_FONT_STYLE_OBLIQUE = 1,
    QALAM_DWRITE_FONT_STYLE_ITALIC = 2
} QalamDWriteFontStyle;

/**
 * @brief Font parameters for text format creation
 */
typedef struct QalamDWriteFontParams {
    const wchar_t* family;          /**< Font family name (e.g., L"Cascadia Code") */
    float size;                     /**< Font size in DIPs (e.g., 14.0f) */
    QalamDWriteFontWeight weight;   /**< Font weight */
    QalamDWriteFontStyle style;     /**< Font style */
    bool is_rtl;                    /**< Enable RTL reading direction */
} QalamDWriteFontParams;

/* ============================================================================
 * Color (C-compatible)
 * ============================================================================ */

/**
 * @brief RGBA color structure with float components (0.0 - 1.0)
 */
typedef struct QalamDWriteColor {
    float r;    /**< Red component (0.0 - 1.0) */
    float g;    /**< Green component (0.0 - 1.0) */
    float b;    /**< Blue component (0.0 - 1.0) */
    float a;    /**< Alpha component (0.0 - 1.0) */
} QalamDWriteColor;

/** Predefined colors */
#define QALAM_DWRITE_COLOR_BLACK   ((QalamDWriteColor){0.0f, 0.0f, 0.0f, 1.0f})
#define QALAM_DWRITE_COLOR_WHITE   ((QalamDWriteColor){1.0f, 1.0f, 1.0f, 1.0f})
#define QALAM_DWRITE_COLOR_RED     ((QalamDWriteColor){1.0f, 0.0f, 0.0f, 1.0f})
#define QALAM_DWRITE_COLOR_GREEN   ((QalamDWriteColor){0.0f, 1.0f, 0.0f, 1.0f})
#define QALAM_DWRITE_COLOR_BLUE    ((QalamDWriteColor){0.0f, 0.0f, 1.0f, 1.0f})

/* ============================================================================
 * DirectWrite Context (Singleton-style)
 * ============================================================================ */

/**
 * @brief Initialize DirectWrite subsystem
 * 
 * Initializes COM, creates DirectWrite and Direct2D factories.
 * Must be called once at application startup before any other
 * DirectWrite functions.
 * 
 * Thread-safe: Uses reference counting for multiple init calls.
 * 
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_init(void);

/**
 * @brief Shutdown DirectWrite and release all resources
 * 
 * Releases DirectWrite and Direct2D factories, uninitializes COM.
 * Should be called once at application shutdown.
 * 
 * Thread-safe: Uses reference counting to ensure proper cleanup.
 */
void qalam_dwrite_shutdown(void);

/**
 * @brief Check if DirectWrite is initialized
 * 
 * @return true if DirectWrite subsystem is initialized
 */
bool qalam_dwrite_is_initialized(void);

/* ============================================================================
 * Text Format Management
 * ============================================================================ */

/**
 * @brief Create a text format with specified parameters
 * 
 * Creates a reusable text format that can be used for multiple
 * text layouts.
 * 
 * @param params Font parameters (family, size, weight, style, RTL)
 * @param out_format Pointer to receive the created format handle
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_text_format_create(
    const QalamDWriteFontParams* params,
    QalamDWriteTextFormat** out_format
);

/**
 * @brief Create Arabic-optimized text format (convenience function)
 * 
 * Creates a text format configured for Arabic text rendering with:
 * - RTL reading direction
 * - Arabic locale for proper shaping
 * - Appropriate text alignment
 * 
 * @param font_family Font family name
 * @param font_size Font size in DIPs
 * @param out_format Pointer to receive the created format handle
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_text_format_create_arabic(
    const wchar_t* font_family,
    float font_size,
    QalamDWriteTextFormat** out_format
);

/**
 * @brief Destroy text format and release resources
 * 
 * @param format Text format to destroy (may be NULL)
 */
void qalam_dwrite_text_format_destroy(QalamDWriteTextFormat* format);

/* ============================================================================
 * Text Layout Management
 * ============================================================================ */

/**
 * @brief Create text layout for measuring and rendering
 * 
 * Creates a text layout from the specified text and format.
 * The layout can be used for measuring text dimensions and rendering.
 * 
 * @param text Text to layout (UTF-16)
 * @param text_length Length of text in characters
 * @param format Text format to use
 * @param max_width Maximum layout width in DIPs
 * @param max_height Maximum layout height in DIPs
 * @param out_layout Pointer to receive the created layout handle
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_text_layout_create(
    const wchar_t* text,
    uint32_t text_length,
    QalamDWriteTextFormat* format,
    float max_width,
    float max_height,
    QalamDWriteTextLayout** out_layout
);

/**
 * @brief Destroy text layout and release resources
 * 
 * @param layout Text layout to destroy (may be NULL)
 */
void qalam_dwrite_text_layout_destroy(QalamDWriteTextLayout* layout);

/**
 * @brief Get text metrics for a layout
 * 
 * @param layout Source text layout
 * @param out_metrics Pointer to receive the metrics
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_text_layout_get_metrics(
    QalamDWriteTextLayout* layout,
    QalamDWriteTextMetrics* out_metrics
);

/**
 * @brief Hit test: point to text position
 * 
 * Determines which character position corresponds to the given
 * x,y coordinates. Used for cursor placement from mouse clicks.
 * 
 * @param layout Source text layout
 * @param x X coordinate relative to layout origin
 * @param y Y coordinate relative to layout origin
 * @param out_result Pointer to receive hit test result
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_text_layout_hit_test_point(
    QalamDWriteTextLayout* layout,
    float x,
    float y,
    QalamDWriteHitTestResult* out_result
);

/**
 * @brief Hit test: text position to point
 * 
 * Determines the x,y coordinates for a given character position.
 * Used for cursor rendering.
 * 
 * @param layout Source text layout
 * @param text_position Character position (0-based)
 * @param is_trailing True for trailing edge, false for leading edge
 * @param out_x Pointer to receive X coordinate
 * @param out_y Pointer to receive Y coordinate
 * @param out_result Optional pointer to receive additional hit test info
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_text_layout_hit_test_position(
    QalamDWriteTextLayout* layout,
    uint32_t text_position,
    bool is_trailing,
    float* out_x,
    float* out_y,
    QalamDWriteHitTestResult* out_result
);

/* ============================================================================
 * Render Target Management
 * ============================================================================ */

/**
 * @brief Create render target for a window
 * 
 * Creates a Direct2D render target bound to the specified window.
 * 
 * @param hwnd Window handle (HWND passed as void* for C compatibility)
 * @param out_target Pointer to receive the created render target handle
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_render_target_create(
    void* hwnd,
    QalamDWriteRenderTarget** out_target
);

/**
 * @brief Resize render target when window resizes
 * 
 * @param target Render target to resize
 * @param width New width in pixels
 * @param height New height in pixels
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_render_target_resize(
    QalamDWriteRenderTarget* target,
    uint32_t width,
    uint32_t height
);

/**
 * @brief Destroy render target and release resources
 * 
 * @param target Render target to destroy (may be NULL)
 */
void qalam_dwrite_render_target_destroy(QalamDWriteRenderTarget* target);

/**
 * @brief Get DPI for render target
 * 
 * @param target Render target
 * @param out_dpi_x Pointer to receive X DPI
 * @param out_dpi_y Pointer to receive Y DPI
 */
void qalam_dwrite_render_target_get_dpi(
    QalamDWriteRenderTarget* target,
    float* out_dpi_x,
    float* out_dpi_y
);

/* ============================================================================
 * Brush Management
 * ============================================================================ */

/**
 * @brief Create solid color brush
 * 
 * @param target Render target to create brush for
 * @param color Brush color
 * @param out_brush Pointer to receive the created brush handle
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_dwrite_brush_create_solid(
    QalamDWriteRenderTarget* target,
    QalamDWriteColor color,
    QalamDWriteBrush** out_brush
);

/**
 * @brief Destroy brush and release resources
 * 
 * @param brush Brush to destroy (may be NULL)
 */
void qalam_dwrite_brush_destroy(QalamDWriteBrush* brush);

/**
 * @brief Update brush color
 * 
 * @param brush Brush to update
 * @param color New color
 */
void qalam_dwrite_brush_set_color(QalamDWriteBrush* brush, QalamDWriteColor color);

/* ============================================================================
 * Rendering Operations
 * ============================================================================ */

/**
 * @brief Begin frame rendering
 * 
 * Must be called before any drawing operations.
 * 
 * @param target Render target
 */
void qalam_dwrite_render_begin(QalamDWriteRenderTarget* target);

/**
 * @brief End frame rendering and present to screen
 * 
 * @param target Render target
 * @return QALAM_OK on success, error code on device lost, etc.
 */
QalamResult qalam_dwrite_render_end(QalamDWriteRenderTarget* target);

/**
 * @brief Clear render target with background color
 * 
 * @param target Render target
 * @param color Clear color
 */
void qalam_dwrite_render_clear(QalamDWriteRenderTarget* target, QalamDWriteColor color);

/**
 * @brief Draw text layout at position
 * 
 * @param target Render target
 * @param layout Text layout to draw
 * @param x X position
 * @param y Y position
 * @param brush Brush for text color
 */
void qalam_dwrite_render_draw_text(
    QalamDWriteRenderTarget* target,
    QalamDWriteTextLayout* layout,
    float x,
    float y,
    QalamDWriteBrush* brush
);

/**
 * @brief Draw rectangle (for cursor, selection, etc.)
 * 
 * @param target Render target
 * @param x Left edge
 * @param y Top edge
 * @param width Rectangle width
 * @param height Rectangle height
 * @param brush Brush for fill/stroke
 * @param filled True for filled rectangle, false for outline
 */
void qalam_dwrite_render_draw_rect(
    QalamDWriteRenderTarget* target,
    float x,
    float y,
    float width,
    float height,
    QalamDWriteBrush* brush,
    bool filled
);

/**
 * @brief Draw line
 * 
 * @param target Render target
 * @param x1 Start X
 * @param y1 Start Y
 * @param x2 End X
 * @param y2 End Y
 * @param brush Brush for line color
 * @param stroke_width Line width
 */
void qalam_dwrite_render_draw_line(
    QalamDWriteRenderTarget* target,
    float x1,
    float y1,
    float x2,
    float y2,
    QalamDWriteBrush* brush,
    float stroke_width
);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Create color from RGBA byte values (0-255)
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return QalamDWriteColor structure
 */
QalamDWriteColor qalam_dwrite_color_from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * @brief Create color from RGB byte values (0-255), fully opaque
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return QalamDWriteColor structure with alpha = 1.0
 */
QalamDWriteColor qalam_dwrite_color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Create color from hex value (0xRRGGBB)
 * 
 * @param hex Hex color value
 * @return QalamDWriteColor structure with alpha = 1.0
 */
QalamDWriteColor qalam_dwrite_color_from_hex(uint32_t hex);

#ifdef __cplusplus
}
#endif

#endif /* QALAM_DWRITE_API_H */