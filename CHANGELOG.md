# Changelog

All notable changes to the Qalam IDE project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Gap buffer implementation for text editing
- DirectWrite text rendering with Arabic shaping
- Win32 window with RTL layout support
- ConPTY terminal integration

---

## [0.0.1] - 2026-01-05

### Added

#### Project Structure
- Created complete project directory structure:
  - `src/` - Source files with subdirectories for core, ui, console, terminal
  - `include/` - Public API headers
  - `tests/` - Unit test stubs (placeholder)
  - `docs/` - Documentation (placeholder)
  - `plans/` - Sprint planning documents

#### Build System
- **CMakeLists.txt**: Complete CMake configuration
  - Windows 10 Build 18362+ targeting (`_WIN32_WINNT=0x0A00`)
  - C11 standard required
  - Unicode support enabled (`UNICODE`, `_UNICODE`)
  - DirectWrite, Direct2D, and Windows libraries linked
  - UTF-8 source file support for MSVC
  - Warning level `/W4` enabled

#### Public API Headers
- **include/qalam.h**: Main public API
  - `QalamResult` error code enumeration with categorized error codes
  - `QalamError` extended error information structure
  - Forward declarations for core types (`QalamEditor`, `QalamBuffer`, `QalamTerminal`, `QalamWindow`)
  - Initialization and shutdown function declarations
  - Version information macros and functions
  - Helper macros (`QALAM_CHECK`, `QALAM_CHECK_NULL`)

- **include/editor.h**: Editor/buffer interface
  - `QalamCursor` position structure with line, column, and visual column
  - `QalamSelection` range structure
  - `QalamBufferStats` statistics structure
  - `QalamTextDirection` enumeration for RTL/LTR detection
  - `QalamLineInfo` structure for line metadata
  - Buffer CRUD operations (`qalam_buffer_create`, `qalam_buffer_destroy`)
  - Text manipulation (`qalam_buffer_insert`, `qalam_buffer_delete`, `qalam_buffer_replace`)
  - Cursor operations (`qalam_buffer_move_cursor`, `qalam_buffer_set_cursor`)
  - Content retrieval (`qalam_buffer_get_line`, `qalam_buffer_get_content`)
  - File operations (`qalam_buffer_save`, `qalam_buffer_load`)
  - Selection operations

- **include/terminal.h**: Terminal/ConPTY interface
  - `QalamTerminalSize` and `QalamTerminalOptions` structures
  - `QalamTerminalState` enumeration
  - `QalamTerminalInfo` status structure
  - Output and state change callbacks
  - Terminal lifecycle (`qalam_terminal_create`, `qalam_terminal_destroy`)
  - Process management (`qalam_terminal_spawn`, `qalam_terminal_kill`, `qalam_terminal_wait`)
  - I/O operations (`qalam_terminal_write`, `qalam_terminal_read`)
  - Arabic console support (`qalam_terminal_enable_arabic`)

- **include/ui.h**: UI/window interface
  - `QalamWindowOptions` for window creation
  - `QalamRect`, `QalamPoint`, `QalamColor` primitives
  - `QalamTextFormat` and `QalamTextMetrics` for DirectWrite
  - `QalamEvent` structure with comprehensive event types
  - Event callback system
  - Window lifecycle (`qalam_window_create`, `qalam_window_destroy`)
  - Message loop (`qalam_window_run`, `qalam_window_poll`)
  - Rendering control (`qalam_window_begin_paint`, `qalam_window_end_paint`)
  - RTL layout support (`qalam_window_set_rtl`, `qalam_window_get_rtl`)
  - DPI awareness utilities
  - Text layout and rendering functions
  - Basic drawing operations
  - Clipboard operations

#### Entry Point
- **src/main.c**: Windows application entry point
  - `WinMain` GUI entry point
  - Optional console entry point for debugging (`QALAM_CONSOLE_SUBSYSTEM`)
  - UTF-8 console initialization with VT processing
  - Per-monitor DPI awareness initialization
  - Placeholder message box with RTL Arabic text
  - Stub event handler structure

### Technical Decisions
- **Gap Buffer**: Chosen for text buffer implementation (efficient for local edits)
- **DirectWrite**: Selected for Arabic text shaping (superior to GDI for complex scripts)
- **ConPTY**: Required for modern terminal embedding on Windows 10+
- **C11 Standard**: Provides modern C features while maintaining broad compatibility
- **Opaque Pointers**: All major types use opaque handles for ABI stability

### Documentation
- Planning documents created:
  - `plans/QALAM_ANALYSIS.md` - Complete roadmap analysis
  - `plans/SPRINT_1_PLAN.md` - Detailed Sprint 1 implementation plan

---

## Version History

| Version | Date       | Milestone                          |
|---------|------------|------------------------------------|
| 0.0.1   | 2026-01-05 | Initial project structure and APIs |

---

## Links

- [Unreleased]: https://github.com/qalam-ide/qalam/compare/v0.0.1...HEAD
- [0.0.1]: https://github.com/qalam-ide/qalam/releases/tag/v0.0.1