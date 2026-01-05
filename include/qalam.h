/**
 * @file qalam.h
 * @brief Qalam IDE - Main Public API Header
 * 
 * Qalam is an Arabic-first Integrated Development Environment designed
 * for native RTL text support using DirectWrite and ConPTY on Windows 10+.
 * 
 * @version 0.1.0-dev
 * @copyright (c) 2026 Qalam Project
 */

#ifndef QALAM_H
#define QALAM_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Platform Detection and Configuration
 *============================================================================*/

#ifndef _WIN32
    #error "Qalam IDE requires Windows 10 Build 18362+ (Version 1903)"
#endif

/* Ensure Windows version macros are set */
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
#endif

#ifndef WINVER
    #define WINVER 0x0A00
#endif

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*=============================================================================
 * Version Information
 *============================================================================*/

#ifndef QALAM_VERSION
    #define QALAM_VERSION "0.1.0-dev"
#endif

#define QALAM_VERSION_MAJOR 0
#define QALAM_VERSION_MINOR 1
#define QALAM_VERSION_PATCH 0

/**
 * @brief Get the Qalam version string
 * @return Pointer to static version string
 */
const char* qalam_get_version(void);

/**
 * @brief Get the Qalam version as numeric components
 * @param[out] major Major version number
 * @param[out] minor Minor version number  
 * @param[out] patch Patch version number
 */
void qalam_get_version_numbers(int* major, int* minor, int* patch);

/*=============================================================================
 * Error Handling
 *============================================================================*/

/**
 * @brief Result codes for Qalam API functions
 * 
 * All Qalam functions that can fail return a QalamResult.
 * QALAM_OK (0) indicates success, all other values indicate errors.
 */
typedef enum QalamResult {
    QALAM_OK = 0,                       /**< Operation completed successfully */
    
    /* General errors (1-99) */
    QALAM_ERROR_UNKNOWN = 1,            /**< Unknown or unspecified error */
    QALAM_ERROR_INVALID_ARGUMENT = 2,   /**< Invalid argument passed to function */
    QALAM_ERROR_NULL_POINTER = 3,       /**< Null pointer where valid pointer expected */
    QALAM_ERROR_OUT_OF_MEMORY = 4,      /**< Memory allocation failed */
    QALAM_ERROR_NOT_INITIALIZED = 5,    /**< Subsystem not initialized */
    QALAM_ERROR_ALREADY_INITIALIZED = 6,/**< Subsystem already initialized */
    
    /* Buffer errors (100-199) */
    QALAM_ERROR_BUFFER_EMPTY = 100,     /**< Buffer is empty */
    QALAM_ERROR_BUFFER_FULL = 101,      /**< Buffer capacity exceeded */
    QALAM_ERROR_INVALID_POSITION = 102, /**< Invalid cursor/position */
    QALAM_ERROR_INVALID_RANGE = 103,    /**< Invalid range specified */
    
    /* Window/UI errors (200-299) */
    QALAM_ERROR_WINDOW_CREATE = 200,    /**< Failed to create window */
    QALAM_ERROR_WINDOW_REGISTER = 201,  /**< Failed to register window class */
    QALAM_ERROR_DIRECTWRITE_INIT = 202, /**< DirectWrite initialization failed */
    QALAM_ERROR_D2D_INIT = 203,         /**< Direct2D initialization failed */
    QALAM_ERROR_RENDER_TARGET = 204,    /**< Failed to create render target */
    
    /* Terminal errors (300-399) */
    QALAM_ERROR_TERMINAL_CREATE = 300,  /**< Failed to create terminal */
    QALAM_ERROR_CONPTY_CREATE = 301,    /**< Failed to create ConPTY */
    QALAM_ERROR_PROCESS_SPAWN = 302,    /**< Failed to spawn process */
    QALAM_ERROR_PIPE_CREATE = 303,      /**< Failed to create pipe */
    QALAM_ERROR_IO_READ = 304,          /**< Read operation failed */
    QALAM_ERROR_IO_WRITE = 305,         /**< Write operation failed */
    
    /* File errors (400-499) */
    QALAM_ERROR_FILE_NOT_FOUND = 400,   /**< File not found */
    QALAM_ERROR_FILE_ACCESS = 401,      /**< File access denied */
    QALAM_ERROR_FILE_READ = 402,        /**< File read error */
    QALAM_ERROR_FILE_WRITE = 403,       /**< File write error */
    QALAM_ERROR_ENCODING = 404,         /**< Encoding conversion error */
    
} QalamResult;

/**
 * @brief Extended error information structure
 * 
 * Provides detailed error context beyond the QalamResult code.
 */
typedef struct QalamError {
    QalamResult code;                   /**< Error code */
    DWORD win32_error;                  /**< Windows error code (GetLastError) */
    HRESULT hresult;                    /**< COM/DirectX HRESULT if applicable */
    const char* message;                /**< Human-readable error message */
    const char* file;                   /**< Source file where error occurred */
    int line;                           /**< Line number where error occurred */
} QalamError;

/**
 * @brief Get the last error that occurred
 * @return Pointer to thread-local error structure
 */
const QalamError* qalam_get_last_error(void);

/**
 * @brief Get a human-readable message for a result code
 * @param result The result code
 * @return Static string describing the error
 */
const char* qalam_result_to_string(QalamResult result);

/**
 * @brief Clear the last error
 */
void qalam_clear_error(void);

/*=============================================================================
 * Forward Declarations - Core Types
 *============================================================================*/

/**
 * @brief Opaque handle to the main editor instance
 * 
 * The QalamEditor manages the editing session including buffers,
 * views, and undo/redo history.
 */
typedef struct QalamEditor QalamEditor;

/**
 * @brief Opaque handle to a text buffer
 * 
 * QalamBuffer implements a gap buffer for efficient text editing
 * with full Unicode and RTL text support.
 */
typedef struct QalamBuffer QalamBuffer;

/**
 * @brief Opaque handle to a terminal instance
 * 
 * QalamTerminal wraps ConPTY for embedded terminal functionality
 * with Arabic console support.
 */
typedef struct QalamTerminal QalamTerminal;

/**
 * @brief Opaque handle to the main window
 * 
 * QalamWindow manages the Win32 window, DirectWrite context,
 * and rendering for the IDE interface.
 */
typedef struct QalamWindow QalamWindow;

/*=============================================================================
 * Initialization and Shutdown
 *============================================================================*/

/**
 * @brief Qalam initialization options
 */
typedef struct QalamInitOptions {
    bool enable_console_utf8;           /**< Enable UTF-8 console codepage */
    bool enable_dpi_awareness;          /**< Enable per-monitor DPI awareness */
    bool enable_dark_mode;              /**< Request dark mode if available */
    const wchar_t* app_name;            /**< Application name for window class */
} QalamInitOptions;

/**
 * @brief Get default initialization options
 * @param[out] options Pointer to options structure to fill
 * @return QALAM_OK on success
 */
QalamResult qalam_get_default_options(QalamInitOptions* options);

/**
 * @brief Initialize the Qalam subsystems
 * 
 * Must be called before any other Qalam functions.
 * Initializes COM, DirectWrite, and other Windows subsystems.
 * 
 * @param options Initialization options, or NULL for defaults
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_init(const QalamInitOptions* options);

/**
 * @brief Shutdown the Qalam subsystems
 * 
 * Releases all resources and uninitializes subsystems.
 * No other Qalam functions should be called after this.
 */
void qalam_shutdown(void);

/**
 * @brief Check if Qalam has been initialized
 * @return true if initialized, false otherwise
 */
bool qalam_is_initialized(void);

/*=============================================================================
 * Utility Macros
 *============================================================================*/

/**
 * @brief Helper macro to check result and return on error
 */
#define QALAM_CHECK(expr) \
    do { \
        QalamResult _result = (expr); \
        if (_result != QALAM_OK) { \
            return _result; \
        } \
    } while (0)

/**
 * @brief Helper macro to check pointer and return error if NULL
 */
#define QALAM_CHECK_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            return QALAM_ERROR_NULL_POINTER; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* QALAM_H */