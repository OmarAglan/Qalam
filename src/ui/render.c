/**
 * @file render.c
 * @brief Qalam IDE - Direct2D Rendering Implementation
 * 
 * Implements Direct2D rendering integration for the Qalam IDE.
 * Provides render target management, text rendering with DirectWrite,
 * and basic drawing operations.
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 */

#include "dwrite_internal.h"
#include <stdio.h>

/*=============================================================================
 * Render Target Management
 *============================================================================*/

QalamResult qalam_render_create_target(
    HWND hwnd,
    ID2D1HwndRenderTarget** out_target)
{
    HRESULT hr;
    ID2D1Factory* factory;
    RECT rc;
    D2D1_RENDER_TARGET_PROPERTIES rt_props;
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_props;
    D2D1_SIZE_U size;
    D2D1_PIXEL_FORMAT pixel_format;
    
    if (!hwnd || !out_target) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    *out_target = NULL;
    
    /* Get Direct2D factory */
    factory = qalam_dwrite_get_d2d_factory();
    if (!factory) {
        return QALAM_ERROR_NOT_INITIALIZED;
    }
    
    /* Get client area dimensions */
    if (!GetClientRect(hwnd, &rc)) {
        return QALAM_ERROR_WINDOW_CREATE;
    }
    
    size.width = (UINT32)(rc.right - rc.left);
    size.height = (UINT32)(rc.bottom - rc.top);
    
    /* Ensure minimum size */
    if (size.width == 0) size.width = 1;
    if (size.height == 0) size.height = 1;
    
    /* Set up pixel format */
    pixel_format.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    pixel_format.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    
    /* Set up render target properties */
    rt_props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_props.pixelFormat = pixel_format;
    rt_props.dpiX = 0.0f;  /* Use default DPI */
    rt_props.dpiY = 0.0f;
    rt_props.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    
    /* Set up HWND render target properties */
    hwnd_rt_props.hwnd = hwnd;
    hwnd_rt_props.pixelSize = size;
    hwnd_rt_props.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
    
    /* Create HWND render target */
    hr = factory->lpVtbl->CreateHwndRenderTarget(
        factory,
        &rt_props,
        &hwnd_rt_props,
        out_target
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_render_create_target", 
                         "Failed to create HWND render target");
        return QALAM_ERROR_RENDER_TARGET;
    }
    
    return QALAM_OK;
}

QalamResult qalam_render_resize_target(
    ID2D1HwndRenderTarget* target,
    UINT width,
    UINT height)
{
    HRESULT hr;
    D2D1_SIZE_U size;
    
    if (!target) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Ensure minimum size */
    if (width == 0) width = 1;
    if (height == 0) height = 1;
    
    size.width = width;
    size.height = height;
    
    /* Resize the render target */
    hr = target->lpVtbl->Resize(target, &size);
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_render_resize_target", 
                         "Failed to resize render target");
        
        /* Check for device loss */
        if (hr == D2DERR_RECREATE_TARGET) {
            return QALAM_ERROR_RENDER_TARGET;  /* Signal that target needs recreation */
        }
        
        return dwrite_hresult_to_result(hr);
    }
    
    return QALAM_OK;
}

void qalam_render_destroy_target(ID2D1HwndRenderTarget* target) {
    if (target) {
        target->lpVtbl->Release(target);
    }
}

/*=============================================================================
 * Brush Management
 *============================================================================*/

QalamResult qalam_render_create_brush(
    ID2D1RenderTarget* target,
    D2D1_COLOR_F color,
    ID2D1SolidColorBrush** out_brush)
{
    HRESULT hr;
    D2D1_BRUSH_PROPERTIES brush_props;
    
    if (!target || !out_brush) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    *out_brush = NULL;
    
    /* Set up brush properties */
    brush_props.opacity = 1.0f;
    brush_props.transform._11 = 1.0f;
    brush_props.transform._12 = 0.0f;
    brush_props.transform._21 = 0.0f;
    brush_props.transform._22 = 1.0f;
    brush_props.transform._31 = 0.0f;
    brush_props.transform._32 = 0.0f;
    
    /* Create solid color brush */
    hr = target->lpVtbl->CreateSolidColorBrush(
        target,
        &color,
        &brush_props,
        out_brush
    );
    
    if (FAILED(hr)) {
        dwrite_log_error(hr, "qalam_render_create_brush", 
                         "Failed to create solid color brush");
        return dwrite_hresult_to_result(hr);
    }
    
    return QALAM_OK;
}

/*=============================================================================
 * Text Rendering
 *============================================================================*/

QalamResult qalam_render_draw_text_layout(
    ID2D1RenderTarget* target,
    IDWriteTextLayout* layout,
    float x,
    float y,
    ID2D1Brush* brush)
{
    D2D1_POINT_2F origin;
    
    if (!target || !layout || !brush) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    origin.x = x;
    origin.y = y;
    
    /* Draw the text layout */
    target->lpVtbl->DrawTextLayout(
        target,
        origin,
        layout,
        brush,
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT  /* Enable color fonts (emoji, etc.) */
    );
    
    return QALAM_OK;
}

/*=============================================================================
 * Frame Rendering
 *============================================================================*/

void qalam_render_begin_frame(ID2D1RenderTarget* target) {
    if (!target) {
        return;
    }
    
    target->lpVtbl->BeginDraw(target);
}

HRESULT qalam_render_end_frame(ID2D1RenderTarget* target) {
    HRESULT hr;
    D2D1_TAG tag1 = 0;
    D2D1_TAG tag2 = 0;
    
    if (!target) {
        return E_INVALIDARG;
    }
    
    hr = target->lpVtbl->EndDraw(target, &tag1, &tag2);
    
    if (FAILED(hr)) {
        if (hr == D2DERR_RECREATE_TARGET) {
            /* Device loss - caller needs to recreate resources */
            dwrite_log_error(hr, "qalam_render_end_frame", 
                             "Device lost - render target needs recreation");
        } else {
            dwrite_log_error(hr, "qalam_render_end_frame", 
                             "EndDraw failed");
        }
    }
    
    return hr;
}

void qalam_render_clear(ID2D1RenderTarget* target, D2D1_COLOR_F color) {
    if (!target) {
        return;
    }
    
    target->lpVtbl->Clear(target, &color);
}

/*=============================================================================
 * Additional Rendering Utilities
 *============================================================================*/

/**
 * @brief Draw a filled rectangle
 * 
 * @param target    Render target
 * @param rect      Rectangle to fill
 * @param brush     Brush to use for filling
 */
void qalam_render_fill_rect(
    ID2D1RenderTarget* target,
    const D2D1_RECT_F* rect,
    ID2D1Brush* brush)
{
    if (!target || !rect || !brush) {
        return;
    }
    
    target->lpVtbl->FillRectangle(target, rect, brush);
}

/**
 * @brief Draw a rectangle outline
 * 
 * @param target        Render target
 * @param rect          Rectangle to stroke
 * @param brush         Brush to use for stroke
 * @param stroke_width  Width of the stroke
 */
void qalam_render_draw_rect(
    ID2D1RenderTarget* target,
    const D2D1_RECT_F* rect,
    ID2D1Brush* brush,
    float stroke_width)
{
    if (!target || !rect || !brush) {
        return;
    }
    
    target->lpVtbl->DrawRectangle(target, rect, brush, stroke_width, NULL);
}

/**
 * @brief Draw a line
 * 
 * @param target        Render target
 * @param x1, y1        Start point
 * @param x2, y2        End point
 * @param brush         Brush to use
 * @param stroke_width  Width of the line
 */
void qalam_render_draw_line(
    ID2D1RenderTarget* target,
    float x1, float y1,
    float x2, float y2,
    ID2D1Brush* brush,
    float stroke_width)
{
    D2D1_POINT_2F p0, p1;
    
    if (!target || !brush) {
        return;
    }
    
    p0.x = x1;
    p0.y = y1;
    p1.x = x2;
    p1.y = y2;
    
    target->lpVtbl->DrawLine(target, p0, p1, brush, stroke_width, NULL);
}

/**
 * @brief Get the render target size
 * 
 * @param target    Render target
 * @param[out] size Pointer to receive size
 * @return QALAM_OK on success
 */
QalamResult qalam_render_get_size(
    ID2D1RenderTarget* target,
    D2D1_SIZE_F* size)
{
    if (!target || !size) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    *size = target->lpVtbl->GetSize(target);
    
    return QALAM_OK;
}

/**
 * @brief Get the render target DPI
 * 
 * @param target    Render target
 * @param[out] dpi_x    Pointer to receive X DPI
 * @param[out] dpi_y    Pointer to receive Y DPI
 * @return QALAM_OK on success
 */
QalamResult qalam_render_get_dpi(
    ID2D1RenderTarget* target,
    float* dpi_x,
    float* dpi_y)
{
    if (!target || !dpi_x || !dpi_y) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    target->lpVtbl->GetDpi(target, dpi_x, dpi_y);
    
    return QALAM_OK;
}

/**
 * @brief Set the render target transform
 * 
 * @param target    Render target
 * @param transform Transform matrix
 */
void qalam_render_set_transform(
    ID2D1RenderTarget* target,
    const D2D1_MATRIX_3X2_F* transform)
{
    if (!target || !transform) {
        return;
    }
    
    target->lpVtbl->SetTransform(target, transform);
}

/**
 * @brief Reset the render target transform to identity
 * 
 * @param target    Render target
 */
void qalam_render_reset_transform(ID2D1RenderTarget* target) {
    D2D1_MATRIX_3X2_F identity = {
        .m[0][0] = 1.0f, .m[0][1] = 0.0f,
        .m[1][0] = 0.0f, .m[1][1] = 1.0f,
        .m[2][0] = 0.0f, .m[2][1] = 0.0f
    };
    
    if (!target) {
        return;
    }
    
    target->lpVtbl->SetTransform(target, &identity);
}

/*=============================================================================
 * Device Loss Handling
 *============================================================================*/

/**
 * @brief Check if render target is valid (no device loss)
 * 
 * @param target    Render target to check
 * @return true if valid, false if device lost and recreation needed
 */
bool qalam_render_check_target(ID2D1RenderTarget* target) {
    HRESULT hr;
    D2D1_TAG tag1 = 0;
    D2D1_TAG tag2 = 0;
    
    if (!target) {
        return false;
    }
    
    /* Try a dummy begin/end cycle to check device status */
    target->lpVtbl->BeginDraw(target);
    hr = target->lpVtbl->EndDraw(target, &tag1, &tag2);
    
    if (hr == D2DERR_RECREATE_TARGET) {
        return false;
    }
    
    return SUCCEEDED(hr);
}

/**
 * @brief Recreate render target after device loss
 * 
 * This function handles the device loss recovery by creating a new
 * render target. The caller is responsible for recreating any
 * device-dependent resources (brushes, etc.) after this call.
 * 
 * @param hwnd          Window handle
 * @param old_target    Old render target (will be released)
 * @param[out] new_target   Pointer to receive new render target
 * @return QALAM_OK on success
 */
QalamResult qalam_render_handle_device_loss(
    HWND hwnd,
    ID2D1HwndRenderTarget* old_target,
    ID2D1HwndRenderTarget** new_target)
{
    if (!hwnd || !new_target) {
        return QALAM_ERROR_NULL_POINTER;
    }
    
    /* Release old target if provided */
    if (old_target) {
        old_target->lpVtbl->Release(old_target);
    }
    
    /* Create new target */
    return qalam_render_create_target(hwnd, new_target);
}

/*=============================================================================
 * Color Utilities
 *============================================================================*/

/**
 * @brief Create a D2D1_COLOR_F from RGBA components (0-255)
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return D2D1_COLOR_F structure
 */
D2D1_COLOR_F qalam_color_from_rgba(BYTE r, BYTE g, BYTE b, BYTE a) {
    D2D1_COLOR_F color;
    color.r = (float)r / 255.0f;
    color.g = (float)g / 255.0f;
    color.b = (float)b / 255.0f;
    color.a = (float)a / 255.0f;
    return color;
}

/**
 * @brief Create a D2D1_COLOR_F from RGB components (0-255), fully opaque
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return D2D1_COLOR_F structure with alpha = 1.0
 */
D2D1_COLOR_F qalam_color_from_rgb(BYTE r, BYTE g, BYTE b) {
    return qalam_color_from_rgba(r, g, b, 255);
}

/**
 * @brief Create a D2D1_COLOR_F from a COLORREF (Windows color)
 * 
 * @param colorref COLORREF value (0x00BBGGRR format)
 * @return D2D1_COLOR_F structure with alpha = 1.0
 */
D2D1_COLOR_F qalam_color_from_colorref(COLORREF colorref) {
    return qalam_color_from_rgb(
        GetRValue(colorref),
        GetGValue(colorref),
        GetBValue(colorref)
    );
}

/**
 * @brief Create a D2D1_COLOR_F from a hex color code
 * 
 * @param hex Hex color value (0xRRGGBB format)
 * @return D2D1_COLOR_F structure with alpha = 1.0
 */
D2D1_COLOR_F qalam_color_from_hex(DWORD hex) {
    return qalam_color_from_rgb(
        (BYTE)((hex >> 16) & 0xFF),
        (BYTE)((hex >> 8) & 0xFF),
        (BYTE)(hex & 0xFF)
    );
}