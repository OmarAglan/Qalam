/**
 * @file terminal.h
 * @brief Qalam IDE - Terminal Interface (ConPTY Integration)
 * 
 * Defines the terminal subsystem API for embedded terminal functionality
 * using Windows ConPTY with Arabic console support.
 * 
 * @version 0.1.0-dev
 * @copyright (c) 2026 Qalam Project
 */

#ifndef QALAM_TERMINAL_H
#define QALAM_TERMINAL_H

#include "qalam.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Terminal Types and Structures
 *============================================================================*/

/**
 * @brief Terminal size in character cells
 */
typedef struct QalamTerminalSize {
    short cols;                     /**< Number of columns */
    short rows;                     /**< Number of rows */
} QalamTerminalSize;

/**
 * @brief Terminal creation options
 */
typedef struct QalamTerminalOptions {
    QalamTerminalSize size;         /**< Initial terminal size */
    const wchar_t* shell_path;      /**< Path to shell executable (NULL for default) */
    const wchar_t* working_dir;     /**< Initial working directory (NULL for current) */
    const wchar_t* environment;     /**< Environment block (NULL for inherit) */
    bool inherit_handles;           /**< Inherit parent handles */
    bool enable_vt_processing;      /**< Enable VT/ANSI processing */
    bool start_hidden;              /**< Start process hidden */
} QalamTerminalOptions;

/**
 * @brief Terminal state enumeration
 */
typedef enum QalamTerminalState {
    QALAM_TERMINAL_UNINITIALIZED = 0,   /**< Not initialized */
    QALAM_TERMINAL_READY,               /**< Ready, no process running */
    QALAM_TERMINAL_RUNNING,             /**< Process is running */
    QALAM_TERMINAL_EXITED,              /**< Process has exited */
    QALAM_TERMINAL_ERROR,               /**< Terminal in error state */
} QalamTerminalState;

/**
 * @brief Terminal information structure
 */
typedef struct QalamTerminalInfo {
    QalamTerminalState state;       /**< Current terminal state */
    QalamTerminalSize size;         /**< Current size */
    DWORD process_id;               /**< Running process ID (0 if none) */
    DWORD exit_code;                /**< Exit code if process exited */
    bool has_pending_output;        /**< Output available to read */
} QalamTerminalInfo;

/**
 * @brief Callback for terminal output
 * 
 * @param terminal The terminal that produced output
 * @param data Output data (UTF-8)
 * @param length Length of data in bytes
 * @param user_data User-provided context
 */
typedef void (*QalamTerminalOutputCallback)(
    QalamTerminal* terminal,
    const char* data,
    size_t length,
    void* user_data
);

/**
 * @brief Callback for terminal state changes
 * 
 * @param terminal The terminal that changed state
 * @param old_state Previous state
 * @param new_state New state
 * @param user_data User-provided context
 */
typedef void (*QalamTerminalStateCallback)(
    QalamTerminal* terminal,
    QalamTerminalState old_state,
    QalamTerminalState new_state,
    void* user_data
);

/*=============================================================================
 * Terminal Creation and Destruction
 *============================================================================*/

/**
 * @brief Get default terminal options
 * 
 * @param[out] options Pointer to options structure to fill
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_get_default_options(QalamTerminalOptions* options);

/**
 * @brief Create a new terminal instance
 * 
 * Creates a ConPTY pseudoconsole but does not spawn a process.
 * 
 * @param[out] terminal Pointer to receive terminal handle
 * @param options Terminal options (NULL for defaults)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_create(QalamTerminal** terminal, const QalamTerminalOptions* options);

/**
 * @brief Destroy a terminal instance
 * 
 * Kills any running process and releases all resources.
 * 
 * @param terminal Terminal to destroy (may be NULL)
 */
void qalam_terminal_destroy(QalamTerminal* terminal);

/*=============================================================================
 * Process Management
 *============================================================================*/

/**
 * @brief Spawn a process in the terminal
 * 
 * @param terminal Target terminal
 * @param command_line Command line to execute (NULL for default shell)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_spawn(QalamTerminal* terminal, const wchar_t* command_line);

/**
 * @brief Spawn the default shell
 * 
 * Uses cmd.exe or PowerShell based on system configuration.
 * 
 * @param terminal Target terminal
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_spawn_shell(QalamTerminal* terminal);

/**
 * @brief Kill the running process
 * 
 * @param terminal Target terminal
 * @param force If true, forcefully terminate; if false, attempt graceful shutdown
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_kill(QalamTerminal* terminal, bool force);

/**
 * @brief Wait for the process to exit
 * 
 * @param terminal Target terminal
 * @param timeout_ms Timeout in milliseconds (INFINITE for no timeout)
 * @param[out] exit_code Pointer to receive exit code (optional)
 * @return QALAM_OK if process exited, QALAM_ERROR_TIMEOUT if timed out
 */
QalamResult qalam_terminal_wait(QalamTerminal* terminal, DWORD timeout_ms, DWORD* exit_code);

/**
 * @brief Check if a process is running
 * 
 * @param terminal Target terminal
 * @return true if process is running
 */
bool qalam_terminal_is_running(const QalamTerminal* terminal);

/*=============================================================================
 * Input/Output Operations
 *============================================================================*/

/**
 * @brief Write data to the terminal
 * 
 * @param terminal Target terminal
 * @param data Data to write (UTF-8)
 * @param length Length of data in bytes
 * @param[out] bytes_written Actual bytes written (optional)
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_write(QalamTerminal* terminal, const char* data, 
                                  size_t length, size_t* bytes_written);

/**
 * @brief Write a string to the terminal
 * 
 * Convenience function for null-terminated strings.
 * 
 * @param terminal Target terminal
 * @param text Null-terminated UTF-8 string
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_write_string(QalamTerminal* terminal, const char* text);

/**
 * @brief Read available output from the terminal
 * 
 * Non-blocking read of available output data.
 * 
 * @param terminal Source terminal
 * @param[out] buffer Buffer to receive data
 * @param buffer_size Size of buffer
 * @param[out] bytes_read Actual bytes read
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_read(QalamTerminal* terminal, char* buffer, 
                                 size_t buffer_size, size_t* bytes_read);

/**
 * @brief Check if output is available
 * 
 * @param terminal Target terminal
 * @return true if output is available to read
 */
bool qalam_terminal_has_output(const QalamTerminal* terminal);

/**
 * @brief Send a signal/key to the terminal
 * 
 * @param terminal Target terminal
 * @param key Virtual key code
 * @param ctrl Control key modifier
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_send_key(QalamTerminal* terminal, WORD key, bool ctrl);

/**
 * @brief Send Ctrl+C to the terminal
 * 
 * @param terminal Target terminal
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_send_interrupt(QalamTerminal* terminal);

/*=============================================================================
 * Terminal Configuration
 *============================================================================*/

/**
 * @brief Resize the terminal
 * 
 * @param terminal Target terminal
 * @param cols New column count
 * @param rows New row count
 * @return QALAM_OK on success, error code on failure
 */
QalamResult qalam_terminal_resize(QalamTerminal* terminal, short cols, short rows);

/**
 * @brief Get current terminal size
 * 
 * @param terminal Target terminal
 * @param[out] size Pointer to receive size
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_get_size(const QalamTerminal* terminal, QalamTerminalSize* size);

/**
 * @brief Get terminal information
 * 
 * @param terminal Target terminal
 * @param[out] info Pointer to receive info
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_get_info(const QalamTerminal* terminal, QalamTerminalInfo* info);

/*=============================================================================
 * Callback Registration
 *============================================================================*/

/**
 * @brief Set output callback
 * 
 * The callback will be invoked when output is available.
 * 
 * @param terminal Target terminal
 * @param callback Callback function (NULL to clear)
 * @param user_data User context passed to callback
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_set_output_callback(QalamTerminal* terminal,
                                                QalamTerminalOutputCallback callback,
                                                void* user_data);

/**
 * @brief Set state change callback
 * 
 * @param terminal Target terminal
 * @param callback Callback function (NULL to clear)
 * @param user_data User context passed to callback
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_set_state_callback(QalamTerminal* terminal,
                                               QalamTerminalStateCallback callback,
                                               void* user_data);

/*=============================================================================
 * Arabic Console Support
 *============================================================================*/

/**
 * @brief Enable Arabic console mode
 * 
 * Configures the terminal for proper Arabic text display:
 * - Sets UTF-8 code page
 * - Enables VT processing for BiDi
 * - Configures appropriate font if available
 * 
 * @param terminal Target terminal
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_enable_arabic(QalamTerminal* terminal);

/**
 * @brief Set terminal code page
 * 
 * @param terminal Target terminal
 * @param codepage Code page identifier (e.g., CP_UTF8)
 * @return QALAM_OK on success
 */
QalamResult qalam_terminal_set_codepage(QalamTerminal* terminal, UINT codepage);

#ifdef __cplusplus
}
#endif

#endif /* QALAM_TERMINAL_H */