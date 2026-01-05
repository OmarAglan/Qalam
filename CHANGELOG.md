# Changelog

All notable changes to the Qalam IDE project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- DirectWrite text rendering with Arabic shaping
- Win32 window with RTL layout support
- ConPTY terminal integration
- Arabic-aware console output

---

## [0.0.2] - 2026-01-05

### Added

#### Gap Buffer Implementation (`src/core/buffer.c`)
Complete gap buffer text storage implementation for the Qalam editor core:

**Buffer Lifecycle**
- `qalam_buffer_create()` - Create empty buffer with 4096 wchar_t initial capacity
- `qalam_buffer_create_with_capacity()` - Create buffer with custom capacity
- `qalam_buffer_create_from_text()` - Create buffer from UTF-8 text
- `qalam_buffer_create_from_file()` - Create buffer from file
- `qalam_buffer_destroy()` - Free all buffer resources

**Core Operations**
- `qalam_buffer_insert()` - Insert UTF-8 text at cursor (O(1) at cursor)
- `qalam_buffer_insert_at()` - Insert text at specified position
- `qalam_buffer_delete()` - Delete characters forward/backward (O(1) at cursor)
- `qalam_buffer_delete_range()` - Delete text range
- `qalam_buffer_replace()` - Replace text in range

**Cursor Management**
- `qalam_buffer_get_cursor()` - Get current cursor position (line, column, offset)
- `qalam_buffer_set_cursor()` - Set cursor by line and column
- `qalam_buffer_set_cursor_offset()` - Set cursor by byte offset
- `qalam_buffer_move_cursor()` - Move cursor relative to current position
- `qalam_buffer_cursor_to_start()` - Move to buffer start
- `qalam_buffer_cursor_to_end()` - Move to buffer end
- `qalam_buffer_cursor_to_line_start()` - Move to line start
- `qalam_buffer_cursor_to_line_end()` - Move to line end

**Content Retrieval**
- `qalam_buffer_get_line()` - Get specific line content (UTF-8)
- `qalam_buffer_get_line_info()` - Get line metadata including RTL detection
- `qalam_buffer_get_content()` - Get full buffer content (UTF-8)
- `qalam_buffer_get_range()` - Get content range (UTF-8)

**Buffer Statistics**
- `qalam_buffer_get_stats()` - Get comprehensive buffer statistics
- `qalam_buffer_get_line_count()` - Get number of lines
- `qalam_buffer_get_size()` - Get byte size
- `qalam_buffer_is_modified()` - Check modification status
- `qalam_buffer_clear_modified()` - Clear modified flag

**File Operations**
- `qalam_buffer_save()` - Save buffer to file (UTF-8)
- `qalam_buffer_load()` - Load file into buffer

**Selection Operations**
- `qalam_buffer_get_selection()` - Get current selection
- `qalam_buffer_set_selection()` - Set selection range
- `qalam_buffer_clear_selection()` - Clear selection
- `qalam_buffer_get_selected_text()` - Get selected text

#### Unit Tests (`tests/test_buffer.c`)
Comprehensive test suite with 35 tests:

- **Buffer Creation/Destruction**: 5 tests
- **Insert Operations**: 6 tests (beginning, middle, end, with newlines)
- **Delete Operations**: 5 tests (forward, backward, range, newlines)
- **Cursor Movement**: 3 tests (absolute, relative, line navigation)
- **Line Counting**: 3 tests (multiple lines, insert/delete tracking)
- **Empty Buffer Edge Cases**: 2 tests
- **Unicode/UTF-16 Handling**: 4 tests (Arabic, mixed text, surrogate pairs)
- **Large Content**: 2 tests (1MB, 10MB performance validation)
- **Replace Operations**: 1 test
- **Selection Operations**: 1 test
- **Buffer Statistics**: 1 test
- **Line Information**: 1 test
- **Error Handling**: 1 test

#### Build System Updates (`CMakeLists.txt`)
- Added `src/core/buffer.c` to main executable sources
- Created `QALAM_CORE_SOURCES` variable for shared sources
- Added `test_buffer` executable target
- Enabled CTest integration with `BufferTests` test

### Technical Details

**Memory Strategy**
- Initial capacity: 4096 wchar_t (~8KB)
- Gap size: 2048 wchar_t initially
- Growth: Double capacity when gap exhausted, then add 2048 to gap
- Maximum buffer size: 100MB

**Performance Characteristics**
- O(1) insert/delete at cursor position
- O(n) cursor movement (gap relocation)
- Incremental line count updates

**UTF-16 Support**
- All text stored internally as wchar_t (UTF-16 on Windows)
- Proper surrogate pair handling for characters outside BMP
- UTF-8 API for external interface
- RTL character detection for Arabic/Hebrew text

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
| 0.0.2   | 2026-01-05 | Gap buffer implementation          |
| 0.0.1   | 2026-01-05 | Initial project structure and APIs |

---

## Links

- [Unreleased]: https://github.com/qalam-ide/qalam/compare/v0.0.2...HEAD
- [0.0.2]: https://github.com/qalam-ide/qalam/compare/v0.0.1...v0.0.2
- [0.0.1]: https://github.com/qalam-ide/qalam/releases/tag/v0.0.1