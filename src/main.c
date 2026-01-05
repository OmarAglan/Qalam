/**
 * @file main.c
 * @brief Qalam IDE - Application Entry Point
 * 
 * Windows entry point for the Qalam IDE application.
 * Initializes subsystems, creates the main window, and runs the message loop.
 * 
 * @version 0.0.1
 * @copyright (c) 2026 Qalam Project
 */

/* Windows headers - must be included before other headers */
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
    #define WINVER 0x0A00
#endif
#ifndef UNICODE
    #define UNICODE
#endif
#ifndef _UNICODE
    #define _UNICODE
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

/* Qalam headers */
#include "qalam.h"
#include "editor.h"
#include "terminal.h"
#include "ui.h"

/*=============================================================================
 * Forward Declarations
 *============================================================================*/

static bool initialize_utf8_console(void);
static bool initialize_dpi_awareness(void);
static int run_application(HINSTANCE hInstance, int nCmdShow);
static void cleanup_application(void);

/*=============================================================================
 * Global State (will be refactored into proper structures)
 *============================================================================*/

static QalamWindow* g_main_window = NULL;
static QalamBuffer* g_active_buffer = NULL;
static QalamTerminal* g_terminal = NULL;
static bool g_is_running = false;

/*=============================================================================
 * Windows Entry Point
 *============================================================================*/

/**
 * @brief Windows GUI application entry point
 * 
 * @param hInstance Application instance handle
 * @param hPrevInstance Always NULL (legacy parameter)
 * @param lpCmdLine Command line arguments (ANSI)
 * @param nCmdShow Initial window show state
 * @return Exit code
 */
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    /* Suppress unused parameter warnings */
    (void)hPrevInstance;
    (void)lpCmdLine;
    
    int exit_code = 0;
    
    /*-------------------------------------------------------------------------
     * Step 1: Initialize UTF-8 Console Support
     *------------------------------------------------------------------------*/
    if (!initialize_utf8_console()) {
        /* Non-fatal: console output may not display Arabic correctly */
        OutputDebugStringW(L"[Qalam] Warning: Failed to initialize UTF-8 console\n");
    }
    
    /*-------------------------------------------------------------------------
     * Step 2: Initialize DPI Awareness
     *------------------------------------------------------------------------*/
    if (!initialize_dpi_awareness()) {
        /* Non-fatal: UI may appear blurry on high-DPI displays */
        OutputDebugStringW(L"[Qalam] Warning: Failed to set DPI awareness\n");
    }
    
    /*-------------------------------------------------------------------------
     * Step 3: Run Application
     *------------------------------------------------------------------------*/
    exit_code = run_application(hInstance, nCmdShow);
    
    /*-------------------------------------------------------------------------
     * Step 4: Cleanup
     *------------------------------------------------------------------------*/
    cleanup_application();
    
    return exit_code;
}

/*=============================================================================
 * Console Entry Point (Alternative for debugging)
 *============================================================================*/

#ifdef QALAM_CONSOLE_SUBSYSTEM
/**
 * @brief Console application entry point (for debugging)
 * 
 * This entry point is used when building with console subsystem
 * for easier debugging with printf output.
 */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    
    /* Initialize UTF-8 console */
    if (!initialize_utf8_console()) {
        fprintf(stderr, "[Qalam] Warning: Failed to initialize UTF-8 console\n");
    }
    
    /* Initialize DPI awareness */
    if (!initialize_dpi_awareness()) {
        fprintf(stderr, "[Qalam] Warning: Failed to set DPI awareness\n");
    }
    
    printf("Qalam IDE %s\n", QALAM_VERSION);
    printf("Arabic-first Integrated Development Environment\n\n");
    
    int exit_code = run_application(hInstance, SW_SHOWDEFAULT);
    
    cleanup_application();
    
    return exit_code;
}
#endif /* QALAM_CONSOLE_SUBSYSTEM */

/*=============================================================================
 * Initialization Functions
 *============================================================================*/

/**
 * @brief Initialize UTF-8 console code page
 * 
 * Sets the console to UTF-8 mode for proper Arabic text display.
 * 
 * @return true on success
 */
static bool initialize_utf8_console(void)
{
    /* Set console code page to UTF-8 */
    if (!SetConsoleOutputCP(CP_UTF8)) {
        return false;
    }
    if (!SetConsoleCP(CP_UTF8)) {
        return false;
    }
    
    /* Set C locale for UTF-8 */
    setlocale(LC_ALL, ".UTF-8");
    
    /* Enable ANSI/VT escape sequences for console */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }
    }
    
    return true;
}

/**
 * @brief Initialize per-monitor DPI awareness
 * 
 * Enables proper scaling on high-DPI displays.
 * 
 * @return true on success
 */
static bool initialize_dpi_awareness(void)
{
    /* Try SetProcessDpiAwarenessContext (Windows 10 1703+) */
    typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
    
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        SetProcessDpiAwarenessContextFunc setDpiContext = 
            (SetProcessDpiAwarenessContextFunc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        
        if (setDpiContext) {
            if (setDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                return true;
            }
        }
    }
    
    /* Fallback: SetProcessDpiAwareness (Windows 8.1+) */
    typedef HRESULT (WINAPI *SetProcessDpiAwarenessFunc)(int);
    
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
        SetProcessDpiAwarenessFunc setDpiAwareness = 
            (SetProcessDpiAwarenessFunc)GetProcAddress(shcore, "SetProcessDpiAwareness");
        
        if (setDpiAwareness) {
            /* PROCESS_PER_MONITOR_DPI_AWARE = 2 */
            if (SUCCEEDED(setDpiAwareness(2))) {
                FreeLibrary(shcore);
                return true;
            }
        }
        FreeLibrary(shcore);
    }
    
    return false;
}

/*=============================================================================
 * Application Lifecycle
 *============================================================================*/

/**
 * @brief Main application initialization and run loop
 * 
 * @param hInstance Application instance
 * @param nCmdShow Initial window show state
 * @return Exit code
 */
static int run_application(HINSTANCE hInstance, int nCmdShow)
{
    QalamResult result;
    int exit_code = 0;
    
    (void)hInstance;  /* Will be used when creating window */
    (void)nCmdShow;   /* Will be used when showing window */
    
    /*-------------------------------------------------------------------------
     * Initialize Qalam Subsystems
     *------------------------------------------------------------------------*/
    
    /* TODO: Initialize Qalam core */
    /* 
    QalamInitOptions init_options;
    result = qalam_get_default_options(&init_options);
    if (result != QALAM_OK) {
        OutputDebugStringW(L"[Qalam] Failed to get default options\n");
        return 1;
    }
    
    init_options.enable_console_utf8 = true;
    init_options.enable_dpi_awareness = true;
    init_options.enable_dark_mode = true;
    init_options.app_name = L"Qalam IDE";
    
    result = qalam_init(&init_options);
    if (result != QALAM_OK) {
        OutputDebugStringW(L"[Qalam] Failed to initialize subsystems\n");
        return 1;
    }
    */
    
    /*-------------------------------------------------------------------------
     * Create Main Window
     *------------------------------------------------------------------------*/
    
    /* TODO: Create main window with RTL support */
    /*
    QalamWindowOptions window_options;
    result = qalam_window_get_default_options(&window_options);
    if (result != QALAM_OK) {
        qalam_shutdown();
        return 1;
    }
    
    window_options.title = L"Qalam IDE - محرر قلم";
    window_options.width = 1280;
    window_options.height = 720;
    window_options.rtl_layout = true;
    window_options.dark_mode = true;
    
    result = qalam_window_create(&g_main_window, &window_options);
    if (result != QALAM_OK) {
        OutputDebugStringW(L"[Qalam] Failed to create main window\n");
        qalam_shutdown();
        return 1;
    }
    */
    
    /*-------------------------------------------------------------------------
     * Create Initial Buffer
     *------------------------------------------------------------------------*/
    
    /* TODO: Create empty buffer for editing */
    /*
    result = qalam_buffer_create(&g_active_buffer);
    if (result != QALAM_OK) {
        OutputDebugStringW(L"[Qalam] Failed to create buffer\n");
        qalam_window_destroy(g_main_window);
        qalam_shutdown();
        return 1;
    }
    */
    
    /*-------------------------------------------------------------------------
     * Show Window and Run Message Loop
     *------------------------------------------------------------------------*/
    
    /* TODO: Show window and run event loop */
    /*
    result = qalam_window_show(g_main_window);
    if (result != QALAM_OK) {
        OutputDebugStringW(L"[Qalam] Failed to show window\n");
        exit_code = 1;
    } else {
        g_is_running = true;
        exit_code = qalam_window_run(g_main_window);
    }
    */
    
    /*-------------------------------------------------------------------------
     * Placeholder: Simple message box for testing
     *------------------------------------------------------------------------*/
    
    result = QALAM_OK;  /* Suppress unused variable warning */
    (void)result;
    
    MessageBoxW(
        NULL,
        L"Qalam IDE - محرر قلم\n\n"
        L"Arabic-first Integrated Development Environment\n"
        L"Version: " QALAM_VERSION L"\n\n"
        L"This is a placeholder. The full UI will be implemented in upcoming sprints.",
        L"Qalam IDE",
        MB_OK | MB_ICONINFORMATION | MB_RTLREADING | MB_RIGHT
    );
    
    return exit_code;
}

/**
 * @brief Cleanup application resources
 */
static void cleanup_application(void)
{
    g_is_running = false;
    
    /* TODO: Cleanup in reverse order of initialization */
    /*
    if (g_terminal) {
        qalam_terminal_destroy(g_terminal);
        g_terminal = NULL;
    }
    
    if (g_active_buffer) {
        qalam_buffer_destroy(g_active_buffer);
        g_active_buffer = NULL;
    }
    
    if (g_main_window) {
        qalam_window_destroy(g_main_window);
        g_main_window = NULL;
    }
    
    qalam_shutdown();
    */
    
    OutputDebugStringW(L"[Qalam] Application cleanup complete\n");
}

/*=============================================================================
 * Event Handlers (Stubs for future implementation)
 *============================================================================*/

/**
 * @brief Handle window events
 * 
 * @param window Source window
 * @param event Event data
 * @param user_data User context (unused)
 * @return true if event was handled
 */
#if 0  /* Will be enabled when UI is implemented */
static bool on_window_event(QalamWindow* window, const QalamEvent* event, void* user_data)
{
    (void)user_data;
    
    switch (event->type) {
        case QALAM_EVENT_CLOSE:
            qalam_window_quit(window, 0);
            return true;
            
        case QALAM_EVENT_RESIZE:
            /* Handle window resize */
            qalam_window_invalidate(window);
            return true;
            
        case QALAM_EVENT_PAINT:
            /* Handle paint request */
            /* render_editor(window, g_active_buffer); */
            return true;
            
        case QALAM_EVENT_KEY_DOWN:
            /* Handle keyboard input */
            /* handle_key_input(window, g_active_buffer, event); */
            return false;  /* Allow default processing */
            
        case QALAM_EVENT_CHAR:
            /* Handle character input */
            /* handle_char_input(g_active_buffer, event->data.character.codepoint); */
            return true;
            
        default:
            return false;  /* Not handled */
    }
}
#endif