/**
 * @file ui.h
 * @brief Qalam IDE - UI and Window Interface
 * 
 * Defines the window management and DirectWrite rendering APIs
 * for the Qalam IDE with full RTL layout support.
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 */

#ifndef QALAM_UI_H
#define QALAM_UI_H

#include "qalam.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * UI Types and Structures
 *============================================================================*/

/**
 * @brief Window creation options
 */
typedef struct QalamWindowOptions {
    const wchar_t* title;           /**< Window title */
    int x;                          /**< Initial X position (CW_USEDEFAULT for default) */
    int y;                          /**< Initial Y position (CW_USEDEFAULT for default) */
    int width;                      /**< Initial width */
    int height;                     /**< Initial height */
    bool rtl_layout;                /**< Enable RTL window layout */
    bool dark_mode;                 /**< Request dark mode title bar */
    bool maximized;                 /**< Start maximized */
    bool resizable;                 /**< Allow resizing */
    HWND parent;                    /**< Parent window (NULL for top-level) */
} QalamWindowOptions;

/**
 * @brief Window state enumeration
 */
typedef enum QalamWindowState {
    QALAM_WINDOW_NORMAL = 0,        /**< Normal windowed state */
    QALAM_WINDOW_MINIMIZED,         /**< Minimized to taskbar */
    QALAM_WINDOW_MAXIMIZED,         /**< Maximized */
    QALAM_WINDOW_FULLSCREEN,        /**< Fullscreen mode */
} QalamWindowState;

/**
 * @brief Rectangle structure (client coordinates)
 */
typedef struct QalamRect {
    float left;
    float top;
    float right;
    float bottom;
} QalamRect;

/**
 * @brief Point structure
 */
typedef struct QalamPoint {
    float x;
    float y;
} QalamPoint;

/**
 * @brief Color structure (RGBA, 0.0-1.0)
 */
typedef struct QalamColor {
    float r;
    float g;
    float b;
    float a;
} QalamColor;

/**
 * @brief Font weight enumeration
 */
typedef enum QalamFontWeight {
    QALAM_FONT_WEIGHT_THIN = 100,
    QALAM_FONT_WEIGHT_LIGHT = 300,
    QALAM_FONT_WEIGHT_NORMAL = 400,
    QALAM_FONT_WEIGHT_MEDIUM = 500,
    QALAM_FONT_WEIGHT_BOLD = 700,
    QALAM_FONT_WEIGHT_BLACK = 900,
} QalamFontWeight;

/**
 * @brief Font style enumeration
 */
typedef enum QalamFontStyle {
    QALAM_FONT_STYLE_NORMAL = 0,
    QALAM_FONT_STYLE_ITALIC,
    QALAM_FONT_STYLE_OBLIQUE,
} QalamFontStyle;

/**
 * @brief Text format/style configuration
 */
typedef struct QalamTextFormat {
    const wchar_t* font_family;     /**< Font family name */
    float font_size;                /**< Font size in DIPs */
    QalamFontWeight weight;         /**< Font weight */
    QalamFontStyle style;           /**< Font style */
    bool rtl;                       /**< Right-to-left reading direction */
} QalamTextFormat;

/**
 * @brief Text metrics after layout
 */
typedef struct QalamTextMetrics {
    float width;                    /**< Total width */
    float height;                   /**< Total height */
    float baseline;                 /**< Distance from top to baseline */
    float line_height;              /**< Line height */
    size_t line_count;              /**< Number of lines */
} QalamTextMetrics;

/**
 * @brief Hit test result for text
 */
typedef struct QalamTextHitTest {
    size_t text_position;           /**< Character position in text */
    bool is_trailing;               /**< Hit on trailing edge of character */
    bool is_inside;                 /**< Point is inside text bounds */
    QalamRect char_bounds;          /**< Bounds of hit character */
} QalamTextHitTest;

/**
 * @brief Opaque handle to DirectWrite text layout
 */
typedef struct QalamTextLayout QalamTextLayout;

/**
 * @brief Opaque handle to a brush (solid color, gradient, etc.)
 */
typedef struct QalamBrush QalamBrush;

/*=============================================================================
 * Window Event Types
 *============================================================================*/

/**
 * @brief Window event type enumeration
 */
typedef enum QalamEventType {
    QALAM_EVENT_NONE = 0,
    
    /* Window events */
    QALAM_EVENT_CLOSE,              /**< Window close requested */
    QALAM_EVENT_RESIZE,             /**< Window resized */
    QALAM_EVENT_FOCUS,              /**< Window gained focus */
    QALAM_EVENT_BLUR,               /**< Window lost focus */
    QALAM_EVENT_DPI_CHANGED,        /**< DPI changed (moved to different monitor) */
    
    /* Mouse events */
    QALAM_EVENT_MOUSE_MOVE,         /**< Mouse moved */
    QALAM_EVENT_MOUSE_DOWN,         /**< Mouse button pressed */
    QALAM_EVENT_MOUSE_UP,           /**< Mouse button released */
    QALAM_EVENT_MOUSE_WHEEL,        /**< Mouse wheel scrolled */
    QALAM_EVENT_MOUSE_ENTER,        /**< Mouse entered window */
    QALAM_EVENT_MOUSE_LEAVE,        /**< Mouse left window */
    
    /* Keyboard events */
    QALAM_EVENT_KEY_DOWN,           /**< Key pressed */
    QALAM_EVENT_KEY_UP,             /**< Key released */
    QALAM_EVENT_CHAR,               /**< Character input (after translation) */
    
    /* Custom/Paint events */
    QALAM_EVENT_PAINT,              /**< Paint requested */
} QalamEventType;

/**
 * @brief Mouse button enumeration
 */
typedef enum QalamMouseButton {
    QALAM_MOUSE_LEFT = 0,
    QALAM_MOUSE_RIGHT,
    QALAM_MOUSE_MIDDLE,
    QALAM_MOUSE_X1,
    QALAM_MOUSE_X2,
} QalamMouseButton;

/**
 * @brief Modifier key flags
 */
typedef enum QalamModifiers {
    QALAM_MOD_NONE = 0,
    QALAM_MOD_SHIFT = 1 << 0,
    QALAM_MOD_CTRL = 1 << 1,
    QALAM_MOD_ALT = 1 << 2,
    QALAM_MOD_META = 1 << 3,        /**< Windows/Super key */
} QalamModifiers;

/**
 * @brief Window event structure
 */
typedef struct QalamEvent {
    QalamEventType type;            /**< Event type */
    QalamModifiers modifiers;       /**< Active modifiers */
    
    union {
        /* Resize event data */
        struct {
            int width;
            int height;
        } resize;
        
        /* Mouse event data */
        struct {
            float x;
            float y;
            QalamMouseButton button;
            int click_count;        /**< 1=single, 2=double, etc. */
            float wheel_delta;      /**< Wheel scroll amount */
        } mouse;
        
        /* Key event data */
        struct {
            UINT virtual_key;       /**< Virtual key code */
            UINT scan_code;         /**< Hardware scan code */
            bool is_repeat;         /**< Key repeat */
        } key;
        
        /* Character event data */
        struct {
            UINT32 codepoint;       /**< Unicode codepoint */
            wchar_t chars[4];       /**< UTF-16 characters (may be surrogate pair) */
        } character;
        
        /* DPI change event data */
        struct {
            UINT dpi;
            RECT suggested_rect;
        } dpi;
    } data;
} QalamEvent;

/**
 * @brief Window event callback
 * 
 * @param window The window that received the event
 * @param event Event data
 * @param user_data User-provided context
 * @return true if event was handled, false to continue default processing
 */
typedef bool (*QalamEventCallback)(
    QalamWindow* window,
    const QalamEvent* event,
    void* user_data
);

/*=============================================================================
 * Window Creation and Management
 *============================================================================*/

/**
 * @brief Get default window options
 * 
 * @param[out] options Pointer to options structure to fill
 * @return QALAM_OK on success
 */
QalamResult qalam_window_get_default_options(QalamWindowOptions* options);

/**
 * @brief Create a new window
 * 
 * Creates a Win32 window with DirectWrite/Direct2D rendering context.
 * 
 * @param[out] window Pointer to receive window handle
 * @param options Window options (NULL for defaults)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_window_create(QalamWindow** window, const QalamWindowOptions* options);

/**
 * @brief Destroy a window
 * 
 * @param window Window to destroy (may be NULL)
 */
void qalam_window_destroy(QalamWindow* window);

/**
 * @brief Show the window
 * 
 * @param window Target window
 * @return QALAM_OK on success
 */
QalamResult qalam_window_show(QalamWindow* window);

/**
 * @brief Hide the window
 * 
 * @param window Target window
 * @return QALAM_OK on success
 */
QalamResult qalam_window_hide(QalamWindow* window);

/**
 * @brief Get the native window handle
 * 
 * @param window Target window
 * @return HWND or NULL if invalid
 */
HWND qalam_window_get_hwnd(const QalamWindow* window);

/*=============================================================================
 * Message Loop and Events
 *============================================================================*/

/**
 * @brief Run the message loop
 * 
 * Blocks until the window is closed or qalam_window_quit() is called.
 * 
 * @param window Main window
 * @return Exit code
 */
int qalam_window_run(QalamWindow* window);

/**
 * @brief Process pending messages without blocking
 * 
 * @param window Target window
 * @return true if a message was processed
 */
bool qalam_window_poll(QalamWindow* window);

/**
 * @brief Request the message loop to exit
 * 
 * @param window Target window
 * @param exit_code Exit code to return from qalam_window_run()
 */
void qalam_window_quit(QalamWindow* window, int exit_code);

/**
 * @brief Set the event callback
 * 
 * @param window Target window
 * @param callback Callback function (NULL to clear)
 * @param user_data User context passed to callback
 * @return QALAM_OK on success
 */
QalamResult qalam_window_set_event_callback(QalamWindow* window,
                                             QalamEventCallback callback,
                                             void* user_data);

/*=============================================================================
 * Rendering Control
 *============================================================================*/

/**
 * @brief Invalidate the window (request repaint)
 * 
 * @param window Target window
 * @return QALAM_OK on success
 */
QalamResult qalam_window_invalidate(QalamWindow* window);

/**
 * @brief Invalidate a specific rectangle
 * 
 * @param window Target window
 * @param rect Rectangle to invalidate
 * @return QALAM_OK on success
 */
QalamResult qalam_window_invalidate_rect(QalamWindow* window, const QalamRect* rect);

/**
 * @brief Begin a paint operation
 * 
 * Must be called before any drawing operations.
 * 
 * @param window Target window
 * @return QALAM_OK on success
 */
QalamResult qalam_window_begin_paint(QalamWindow* window);

/**
 * @brief End paint and present to screen
 * 
 * @param window Target window
 * @return QALAM_OK on success
 */
QalamResult qalam_window_end_paint(QalamWindow* window);

/**
 * @brief Present the render target (alias for end_paint)
 * 
 * @param window Target window
 * @return QALAM_OK on success
 */
QalamResult qalam_window_present(QalamWindow* window);

/**
 * @brief Clear the window with a color
 * 
 * @param window Target window
 * @param color Clear color
 */
void qalam_window_clear(QalamWindow* window, QalamColor color);

/*=============================================================================
 * RTL Layout Support
 *============================================================================*/

/**
 * @brief Set RTL layout mode
 * 
 * When enabled, the window uses RTL layout with mirrored coordinates.
 * 
 * @param window Target window
 * @param rtl true for RTL, false for LTR
 * @return QALAM_OK on success
 */
QalamResult qalam_window_set_rtl(QalamWindow* window, bool rtl);

/**
 * @brief Get current RTL layout mode
 * 
 * @param window Target window
 * @return true if RTL layout is enabled
 */
bool qalam_window_get_rtl(const QalamWindow* window);

/**
 * @brief Get current DPI
 * 
 * @param window Target window
 * @return DPI value (96 = 100% scaling)
 */
UINT qalam_window_get_dpi(const QalamWindow* window);

/**
 * @brief Convert DIPs to pixels
 * 
 * @param window Target window
 * @param dips Device-independent pixels
 * @return Physical pixels
 */
int qalam_window_dip_to_px(const QalamWindow* window, float dips);

/**
 * @brief Convert pixels to DIPs
 * 
 * @param window Target window
 * @param pixels Physical pixels
 * @return Device-independent pixels
 */
float qalam_window_px_to_dip(const QalamWindow* window, int pixels);

/*=============================================================================
 * Text Layout and Rendering
 *============================================================================*/

/**
 * @brief Create a text format
 * 
 * @param window Window for DirectWrite factory
 * @param[out] format Pointer to receive format handle
 * @param options Format options
 * @return QALAM_OK on success
 */
QalamResult qalam_text_format_create(QalamWindow* window, QalamTextFormat** format,
                                      const QalamTextFormat* options);

/**
 * @brief Destroy a text format
 * 
 * @param format Format to destroy
 */
void qalam_text_format_destroy(QalamTextFormat* format);

/**
 * @brief Create a text layout
 * 
 * @param window Window for DirectWrite factory
 * @param[out] layout Pointer to receive layout handle
 * @param text Text to layout (UTF-16)
 * @param text_length Length in characters
 * @param format Text format to use
 * @param max_width Maximum layout width
 * @param max_height Maximum layout height
 * @return QALAM_OK on success
 */
QalamResult qalam_text_layout_create(QalamWindow* window, QalamTextLayout** layout,
                                      const wchar_t* text, size_t text_length,
                                      const QalamTextFormat* format,
                                      float max_width, float max_height);

/**
 * @brief Destroy a text layout
 * 
 * @param layout Layout to destroy
 */
void qalam_text_layout_destroy(QalamTextLayout* layout);

/**
 * @brief Get text metrics
 * 
 * @param layout Source layout
 * @param[out] metrics Pointer to receive metrics
 * @return QALAM_OK on success
 */
QalamResult qalam_text_layout_get_metrics(const QalamTextLayout* layout, 
                                           QalamTextMetrics* metrics);

/**
 * @brief Hit test a point against text layout
 * 
 * @param layout Source layout
 * @param x X coordinate relative to layout origin
 * @param y Y coordinate relative to layout origin
 * @param[out] result Hit test result
 * @return QALAM_OK on success
 */
QalamResult qalam_text_layout_hit_test(const QalamTextLayout* layout,
                                        float x, float y,
                                        QalamTextHitTest* result);

/**
 * @brief Draw text layout
 * 
 * @param window Target window
 * @param layout Text layout to draw
 * @param x X position
 * @param y Y position
 * @param color Text color
 * @return QALAM_OK on success
 */
QalamResult qalam_window_draw_text_layout(QalamWindow* window, 
                                           QalamTextLayout* layout,
                                           float x, float y,
                                           QalamColor color);

/**
 * @brief Draw simple text (creates temporary layout)
 * 
 * @param window Target window
 * @param text Text to draw (UTF-16)
 * @param rect Bounding rectangle
 * @param format Text format
 * @param color Text color
 * @return QALAM_OK on success
 */
QalamResult qalam_window_draw_text(QalamWindow* window, const wchar_t* text,
                                    const QalamRect* rect,
                                    const QalamTextFormat* format,
                                    QalamColor color);

/*=============================================================================
 * Basic Drawing Operations
 *============================================================================*/

/**
 * @brief Draw a filled rectangle
 * 
 * @param window Target window
 * @param rect Rectangle to fill
 * @param color Fill color
 */
void qalam_window_fill_rect(QalamWindow* window, const QalamRect* rect, QalamColor color);

/**
 * @brief Draw a rectangle outline
 * 
 * @param window Target window
 * @param rect Rectangle to stroke
 * @param color Stroke color
 * @param stroke_width Line width
 */
void qalam_window_stroke_rect(QalamWindow* window, const QalamRect* rect, 
                               QalamColor color, float stroke_width);

/**
 * @brief Draw a line
 * 
 * @param window Target window
 * @param x1 Start X
 * @param y1 Start Y
 * @param x2 End X
 * @param y2 End Y
 * @param color Line color
 * @param stroke_width Line width
 */
void qalam_window_draw_line(QalamWindow* window, float x1, float y1, 
                             float x2, float y2, QalamColor color, float stroke_width);

/*=============================================================================
 * Clipboard Operations
 *============================================================================*/

/**
 * @brief Get text from clipboard
 * 
 * @param window Window for clipboard access
 * @param[out] text Buffer to receive text (UTF-8)
 * @param size Buffer size
 * @param[out] bytes_written Actual bytes written
 * @return QALAM_OK on success
 */
QalamResult qalam_clipboard_get_text(QalamWindow* window, char* text, 
                                      size_t size, size_t* bytes_written);

/**
 * @brief Set clipboard text
 * 
 * @param window Window for clipboard access
 * @param text Text to copy (UTF-8)
 * @return QALAM_OK on success
 */
QalamResult qalam_clipboard_set_text(QalamWindow* window, const char* text);

/**
 * @brief Check if clipboard has text
 * 
 * @param window Window for clipboard access
 * @return true if clipboard contains text
 */
bool qalam_clipboard_has_text(QalamWindow* window);

#ifdef __cplusplus
}
#endif

#endif /* QALAM_UI_H */