# Qalam IDE - Development Kickoff Report

**Document Version:** 1.0  
**Date:** 2026-01-05  
**Status:** Ready for Implementation

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Phase Roadmap Summary](#2-phase-roadmap-summary)
3. [Sprint 1 Scope Definition](#3-sprint-1-scope-definition)
4. [Architecture Summary](#4-architecture-summary)
5. [Technical Decisions Register](#5-technical-decisions-register)
6. [Risk Register](#6-risk-register)
7. [Implementation Priority Queue](#7-implementation-priority-queue)
8. [Quality Gates](#8-quality-gates)
9. [Next Steps Checklist](#9-next-steps-checklist)
10. [Appendix: Quick Reference](#10-appendix-quick-reference)

---

## 1. Project Overview

### 1.1 Mission Statement

**Qalam (قلم)** is an Arabic-first Integrated Development Environment designed to solve the fundamental problems that existing editors and terminals have with Arabic text rendering, RTL layout, and console Arabic display on Windows.

### 1.2 Core Problem Statement

| Problem | Current State | Qalam Solution |
|---------|---------------|----------------|
| CMD/PowerShell Arabic rendering | Broken ligatures, incorrect display | Custom terminal with ConPTY + DirectWrite |
| IDE RTL layout | Retrofitted, inconsistent | Native RTL using `WS_EX_LAYOUTRTL` |
| Arabic text shaping | Missing or incomplete | DirectWrite with full OpenType support |
| Mixed LTR/RTL editing | Cursor confusion, selection bugs | Proper BiDi via Windows Uniscribe |

### 1.3 Target Platform

| Requirement | Specification |
|-------------|---------------|
| **Operating System** | Windows 10 Build 18362+ (Version 1903) |
| **API Requirement** | ConPTY (CreatePseudoConsole) |
| **Minimum RAM** | 256 MB (recommended 512 MB+) |
| **Display** | 1024×768 minimum (1920×1080 recommended) |

### 1.4 Technology Stack

| Component | Technology | Rationale |
|-----------|------------|-----------|
| Language | C11 | Native performance, minimal dependencies |
| Build System | CMake 3.16+ | Cross-platform build configuration |
| Text Rendering | DirectWrite | Hardware-accelerated Arabic shaping |
| BiDi Algorithm | Windows Uniscribe | Zero dependencies, proven Arabic support |
| Terminal | ConPTY | Modern pseudo-console API |
| UI Framework | Raw Win32 | Full RTL control, no framework overhead |
| Text Storage | Gap Buffer | O(1) insertion at cursor |

### 1.5 Final Deliverables

| Executable | Purpose | Phase |
|------------|---------|-------|
| `qalam.exe` | Main graphical IDE | Phases 1-9 |
| `qalam-cli.exe` | Command-line interface | Phase 1 |
| `qalam-term.exe` | Standalone Arabic terminal (optional) | Phase 5 |
| `baa-pkg.exe` | Package manager | Phase 10 |

---

## 2. Phase Roadmap Summary

### 2.1 All 12 Phases Overview

| Phase | Name | Criticality | Dependencies |
|-------|------|-------------|--------------|
| 1 | CLI Foundation | 🔴 Critical | None |
| 2 | Syntax Highlighter | 🟡 High | Phase 1 |
| 3 | GUI Core | 🔴 Critical | Phase 1 |
| 4 | Text Engine | 🔴 Critical | Phase 3 |
| 5 | Integrated Terminal | 🔴 Critical | Phase 3 |
| 6 | IDE Features | 🟡 High | Phases 4, 5 |
| 7 | Compiler Integration | 🔴 Critical | Phases 5, 6 |
| 8 | Debugger | 🟡 High | Phase 7 |
| 9 | Advanced Features | 🟡 High | Phases 6, 8 |
| 10 | Package Manager | 🟢 Medium | Phase 9 |
| 11 | Plugin System | 🟢 Medium | Phase 9 |
| 12 | Testing Framework | 🟢 Medium | Phase 9 |

### 2.2 Critical Path Visualization

```
Phase 1 (CLI) ──┬──> Phase 2 (Highlighter)
                │
                └──> Phase 3 (GUI) ──┬──> Phase 4 (Text Engine) ──┐
                                     │                            │
                                     └──> Phase 5 (Terminal) ─────┴──> Phase 6-12
```

### 2.3 Phase Dependencies (ASCII Diagram)

```
┌─────────────────────────────────────────────────────────────────────┐
│                        FOUNDATION LAYER                             │
│  ┌──────────────────┐                                               │
│  │  Phase 1: CLI    │ ─────────────────────────────────────────┐    │
│  │  - UTF-8 Console │                                          │    │
│  │  - Font Detection│                                          │    │
│  │  - VT100 Support │                                          │    │
│  └────────┬─────────┘                                          │    │
│           │                                                    │    │
│           ▼                                                    │    │
│  ┌──────────────────┐                                          │    │
│  │  Phase 3: GUI    │                                          │    │
│  │  - Win32 Window  │                                          │    │
│  │  - RTL Layout    │                                          │    │
│  │  - DirectWrite   │                                          │    │
│  └────────┬─────────┘                                          │    │
│           │                                                    │    │
├───────────┼────────────────────────────────────────────────────┤    │
│           │           CORE EDITOR LAYER                        │    │
│           ├──────────────────────┐                             │    │
│           ▼                      ▼                             │    │
│  ┌──────────────────┐   ┌──────────────────┐                   │    │
│  │  Phase 4: Text   │   │  Phase 5: Term   │                   │    │
│  │  - Gap Buffer    │   │  - ConPTY        │                   │    │
│  │  - Cursor/BiDi   │   │  - ANSI Parser   │                   │    │
│  │  - Undo/Redo     │   │  - RTL Render    │                   │    │
│  └────────┬─────────┘   └────────┬─────────┘                   │    │
│           │                      │                             │    │
│           └──────────┬───────────┘                             │    │
│                      ▼                                         │    │
├──────────────────────────────────────────────────────────────────────┤
│                      IDE LAYER                                 │    │
│  ┌──────────────────┐   ┌──────────────────┐                   │    │
│  │  Phase 6: IDE    │──▶│  Phase 7: Build  │                   │    │
│  │  Features        │   │  Integration     │                   │    │
│  └──────────────────┘   └────────┬─────────┘                   │    │
│                                  │                             │    │
│                                  ▼                             │    │
│  ┌──────────────────┐   ┌──────────────────┐                   │    │
│  │  Phase 8: Debug  │◀──│  Phase 9: Adv.   │                   │    │
│  └──────────────────┘   └──────────────────┘                   │    │
├──────────────────────────────────────────────────────────────────────┤
│                    EXTENSION LAYER (Parallel)                  │    │
│  ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐│    │
│  │ Phase 10: Pkg    │ │ Phase 11: Plugin │ │ Phase 12: Test   ││    │
│  └──────────────────┘ └──────────────────┘ └──────────────────┘│    │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.4 Parallel Development Opportunities

| Track A | Track B | Condition |
|---------|---------|-----------|
| Phase 2 (Syntax Highlighter) | Phase 3 (GUI Core) | After Phase 1 |
| Phase 4 (Text Engine) | Phase 5 (Terminal) | After Phase 3 |
| Phase 8 (Debugger) | Phase 9 (Advanced) | After Phase 7 |
| Phases 10, 11, 12 | All parallel | After Phase 9 |

---

## 3. Sprint 1 Scope Definition

### 3.1 Sprint 1 Components

| Component | Priority | Description | Source File |
|-----------|----------|-------------|-------------|
| Win32 Window with RTL Layout | 🔴 Critical | Foundation for all GUI components | [`src/ui/window.c`](src/ui/window.c) |
| Arabic-Aware Console | 🔴 Critical | Foundation for CLI tools and terminal | [`src/console/console.c`](src/console/console.c) |
| Gap Buffer Text Storage | 🔴 Critical | Core text storage for editor | [`src/core/buffer.c`](src/core/buffer.c) |

### 3.2 Component Boundaries

#### Component 1: Win32 Window with RTL Layout

**Scope:**
- Window class registration with Unicode support
- RTL extended styles (`WS_EX_LAYOUTRTL`, `WS_EX_RTLREADING`)
- DirectWrite factory and render target initialization
- Arabic text format creation with RTL reading direction
- Basic message loop with accelerator support
- Window resize handling with minimum size enforcement

**Out of Scope for Sprint 1:**
- Full menu system (placeholder only)
- Toolbar and status bar
- Editor pane rendering
- Terminal embedding

#### Component 2: Arabic-Aware Console

**Scope:**
- Console handle acquisition and mode configuration
- UTF-8 code page setup (`SetConsoleOutputCP(CP_UTF8)`)
- VT100 mode enabling (`ENABLE_VIRTUAL_TERMINAL_PROCESSING`)
- Arabic-capable font configuration
- Basic ANSI escape sequence support
- Error handling for unsupported configurations

**Out of Scope for Sprint 1:**
- Full ANSI parser state machine
- Terminal emulation grid
- ConPTY process spawning

#### Component 3: Gap Buffer Text Storage

**Scope:**
- Buffer creation and destruction
- Insert/delete operations at cursor position
- Gap movement optimization
- Line tracking array
- UTF-16 surrogate pair handling
- Basic cursor movement (prev/next character)

**Out of Scope for Sprint 1:**
- Undo/redo system
- Syntax highlighting integration
- File loading/saving
- BiDi layout integration

### 3.3 Acceptance Criteria Summary

#### Win32 Window with RTL Layout (10 criteria)

| # | Criterion | Verification |
|---|-----------|--------------|
| 1 | Window displays centered on screen | Visual |
| 2 | Title shows "قلم - Qalam IDE" correctly | Visual |
| 3 | Window uses `WS_EX_LAYOUTRTL` style | `GetWindowLongPtr` |
| 4 | Menu items in RTL order | Visual |
| 5 | Menu text renders Arabic correctly | Visual |
| 6 | Keyboard accelerators work | Manual test |
| 7 | Minimum size 800×600 enforced | Resize test |
| 8 | DirectWrite renders Arabic ligatures | Visual |
| 9 | `WM_SIZE` handled without crash | Resize test |
| 10 | Clean shutdown, no memory leaks | Debug build |

#### Arabic-Aware Console (10 criteria)

| # | Criterion | Verification |
|---|-----------|--------------|
| 1 | Displays "مرحباً بالعالم" | Print test |
| 2 | UTF-8 code page set (65001) | `GetConsoleOutputCP()` |
| 3 | VT100 mode enabled | Console mode flags |
| 4 | ANSI colors work | Color print test |
| 5 | Cursor movement works | ANSI escape test |
| 6 | Arabic font configured | `GetCurrentConsoleFontEx()` |
| 7 | Arabic input handling | Type test |
| 8 | Original settings restored | Exit and verify |
| 9 | Graceful VT100 fallback | Test on older Windows |
| 10 | Bilingual error messages | Error trigger test |

#### Gap Buffer Text Storage (10 criteria)

| # | Criterion | Verification |
|---|-----------|--------------|
| 1 | Insert single char: O(1) average | Benchmark |
| 2 | Insert string: O(n) for string length | Benchmark |
| 3 | Delete at cursor: O(1) | Benchmark |
| 4 | Surrogate pair handling in navigation | Emoji test |
| 5 | Line count accurate after edits | Unit test |
| 6 | Get line content correct | Unit test |
| 7 | Buffer grows when gap exhausted | Large insert test |
| 8 | No memory leaks in lifecycle | Debug heap |
| 9 | Empty buffer has 1 line | Unit test |
| 10 | UTF-16 surrogates in navigation | 𝄞 character test |

---

## 4. Architecture Summary

### 4.1 Module Structure

```
include/
├── qalam.h         # Main public API, error codes, init/shutdown
├── editor.h        # Buffer and cursor interfaces
├── terminal.h      # ConPTY terminal interface
└── ui.h            # Window and DirectWrite interfaces

src/
├── main.c          # Application entry point
├── core/
│   ├── buffer.c    # Gap buffer implementation
│   ├── cursor.c    # Cursor management
│   └── bidi.c      # BiDi text handling (Uniscribe)
├── ui/
│   ├── window.c    # Win32 window management
│   ├── render.c    # DirectWrite rendering
│   └── menu.c      # Menu and accelerators
├── console/
│   ├── console.c   # Console initialization
│   └── ansi.c      # ANSI escape handling
└── terminal/
    ├── conpty.c    # ConPTY wrapper
    └── parser.c    # VT100 parser
```

### 4.2 API Design Principles

| Principle | Implementation |
|-----------|----------------|
| **Opaque handles** | All major types (`QalamBuffer`, `QalamWindow`, etc.) are opaque pointers |
| **Result codes** | All fallible functions return [`QalamResult`](include/qalam.h:77-116) |
| **Create/Destroy pairs** | Every `_create()` has matching `_destroy()` |
| **Null-safe destroy** | All destroy functions accept NULL safely |
| **Thread-local errors** | Extended error info via [`qalam_get_last_error()`](include/qalam.h:136) |

### 4.3 Error Handling Patterns

**Pattern 1: Result code return**
```c
QalamResult qalam_buffer_insert(QalamBuffer* buffer, const char* text, size_t length);
// Returns QALAM_OK (0) on success, error code on failure
```

**Pattern 2: Check macro**
```c
#define QALAM_CHECK(expr) \
    do { \
        QalamResult _result = (expr); \
        if (_result != QALAM_OK) return _result; \
    } while (0)
```

**Error code ranges** (from [`qalam.h`](include/qalam.h:77-116)):
- `1-99`: General errors
- `100-199`: Buffer errors
- `200-299`: Window/UI errors
- `300-399`: Terminal errors
- `400-499`: File errors

### 4.4 Memory Management Patterns

| Pattern | Description |
|---------|-------------|
| **Create/Destroy pairs** | `qalam_buffer_create()` / `qalam_buffer_destroy()` |
| **Immediate allocation check** | Check malloc return immediately |
| **Zero initialization** | Use `calloc()` for clean initial state |
| **NULL-safe free** | Always check before free, set to NULL after |
| **Cleanup on error** | Use goto cleanup pattern for complex init |

### 4.5 Thread Safety Pattern

**Synchronization:** SRWLock (Slim Reader/Writer Lock)

| Resource | Lock Type | Readers | Writers |
|----------|-----------|---------|---------|
| Gap Buffer | SRWLock | Render thread | Edit thread |
| Terminal Output | SRWLock | UI thread | Read thread |
| Line Cache | SRWLock | Render | Edit operations |

---

## 5. Technical Decisions Register

### 5.1 Decisions Made

| ID | Decision | Rationale | Date |
|----|----------|-----------|------|
| D1 | **BiDi: Uniscribe API** | Zero dependencies, native Windows, 20+ years proven Arabic support | 2026-01-05 |
| D2 | **Text Rendering: DirectWrite** | Required for proper Arabic shaping, hardware acceleration, ligatures | 2026-01-05 |
| D3 | **Terminal: ConPTY with disable fallback** | Modern API required; disable terminal on older Windows | 2026-01-05 |
| D4 | **Thread Safety: SRWLock** | Reader/writer semantics, low overhead, Windows-native | 2026-01-05 |
| D5 | **Text Storage: Gap Buffer** | O(1) local edits, simpler than piece table, sufficient for 10MB files | 2026-01-05 |
| D6 | **Encoding: UTF-8 files, UTF-16 internal** | Windows APIs use UTF-16; UTF-8 for file I/O and console | 2026-01-05 |
| D7 | **File size limit: 10 MB** | Gap buffer performance degrades; warn at 5MB, refuse at 10MB | 2026-01-05 |
| D8 | **C Standard: C11** | Modern C with wide character support, no C++ complexity | 2026-01-05 |

### 5.2 Decisions Pending User Input

| ID | Question | Options | Impact |
|----|----------|---------|--------|
| P1 | **Terminal fallback for Windows < 1903** | A) Disable terminal, B) External terminal option, C) Exit with message | UX on older Windows |
| P2 | **Large file handling (>10MB)** | A) Refuse to open, B) Read-only mode, C) Suggest external editor | Large file editing |
| P3 | **Font fallback priority** | A) Cascadia → Consolas → Courier, B) User-configurable, C) System default | Font rendering |

### 5.3 Why NOT Alternative Technologies

| Rejected Option | Reason |
|-----------------|--------|
| **ICU for BiDi** | 25-30 MB DLL dependency, redundant with Uniscribe on Windows |
| **GDI+ for rendering** | No automatic Arabic shaping, manual ligature handling required |
| **Custom BiDi implementation** | UAX #9 is 150+ pages, high bug risk, 4-6 weeks effort |
| **Piece table** | More complex than needed for <10MB files |
| **Electron/CEF** | Memory overhead, no native RTL control |

---

## 6. Risk Register

### 6.1 Active Risks

| ID | Risk | Severity | Likelihood | Impact | Status | Mitigation |
|----|------|----------|------------|--------|--------|------------|
| R1 | ConPTY unavailable (Win < 1903) | 🔴 High | Low | Terminal disabled | ✅ Mitigated | Version detection + disable |
| R2 | DirectWrite Arabic edge cases | 🟡 Medium | Medium | Visual artifacts | 🔄 Monitoring | Comprehensive test suite |
| R3 | BiDi mixed content bugs | 🟡 Medium | Medium | Incorrect display | 🔄 Monitoring | Uniscribe + extensive testing |
| R4 | Gap buffer large file perf | 🟡 Medium | Low | Slow editing | ✅ Mitigated | File size limits + warnings |
| R5 | COM/DirectWrite memory leaks | 🟡 Medium | Medium | Memory growth | 🔄 Monitoring | Reference counting + debug tools |
| R6 | Thread contention on buffer | 🟡 Medium | Low | UI stuttering | ✅ Mitigated | SRWLock read/write semantics |
| R7 | UTF-8/UTF-16 conversion overhead | 🟢 Low | Low | Performance | ✅ Mitigated | Thread-local buffer reuse |
| R8 | Font fallback failures | 🟢 Low | Low | Missing glyphs | ⏳ Planned | Custom font fallback chain |
| R9 | DPI change handling | 🟢 Low | Medium | Blurry text | ⏳ Planned | Render target recreation |
| R10 | VT100 parsing errors | 🟢 Low | Medium | Terminal corruption | ⏳ Planned | Parser test suite |

### 6.2 Mitigation Implementation Status

| Risk | Mitigation Code Location |
|------|--------------------------|
| R1 (ConPTY) | Version check in [`src/main.c`](src/main.c:192-228) |
| R4 (Large files) | File size check before load |
| R6 (Threading) | SRWLock wrappers |
| R7 (Encoding) | Thread-local conversion buffers |

---

## 7. Implementation Priority Queue

### 7.1 Sprint 1 Task Order

| Priority | Task | Component | Dependencies | Estimated Complexity |
|----------|------|-----------|--------------|---------------------|
| 1 | **DPI awareness initialization** | Core | None | Low |
| 2 | **UTF-8 console setup** | Console | None | Low |
| 3 | **DirectWrite factory singleton** | UI | DPI awareness | Medium |
| 4 | **Window class registration** | UI | DirectWrite | Low |
| 5 | **Window creation with RTL styles** | UI | Window class | Medium |
| 6 | **Render target creation** | UI | Window | Medium |
| 7 | **Arabic text format creation** | UI | Render target | Low |
| 8 | **Basic message loop** | UI | Window | Low |
| 9 | **WM_PAINT handler (clear color)** | UI | Render target | Low |
| 10 | **WM_SIZE handler (resize target)** | UI | Render target | Low |
| 11 | **Gap buffer create/destroy** | Core | None | Low |
| 12 | **Gap movement implementation** | Core | Gap buffer | Medium |
| 13 | **Buffer insert operation** | Core | Gap movement | Medium |
| 14 | **Buffer delete operation** | Core | Gap movement | Medium |
| 15 | **Line tracking array** | Core | Insert/delete | Medium |
| 16 | **Console VT100 mode** | Console | UTF-8 setup | Low |
| 17 | **Console font configuration** | Console | VT100 mode | Low |
| 18 | **ANSI color output helpers** | Console | VT100 mode | Low |
| 19 | **Text rendering (single line)** | UI | Text format | Medium |
| 20 | **Menu placeholder** | UI | Window | Low |

### 7.2 Dependency Graph (Sprint 1 Tasks)

```
[DPI Init] ──┬──▶ [DirectWrite Factory] ──▶ [Window Class] ──▶ [Window Creation]
             │                                                        │
[UTF-8 Console] ──▶ [VT100 Mode] ──▶ [Font Config]                   │
                                          │                          │
                                          ▼                          ▼
                                   [ANSI Helpers]            [Render Target]
                                                                     │
                                                         ┌───────────┼───────────┐
                                                         ▼           ▼           ▼
                                                  [Text Format] [WM_PAINT] [WM_SIZE]
                                                         │
                                                         ▼
                                                  [Text Render]

[Gap Buffer Create] ──▶ [Gap Movement] ──┬──▶ [Insert]
                                         └──▶ [Delete]
                                                  │
                                                  ▼
                                           [Line Tracking]
```

### 7.3 Suggested Implementation Order (Solo Developer)

**Week 1: Foundation**
1. DPI awareness + UTF-8 console (Day 1)
2. DirectWrite factory singleton (Day 1-2)
3. Window class + creation with RTL (Day 2-3)
4. Render target + basic paint (Day 3-4)
5. Message loop + resize handling (Day 4-5)

**Week 2: Text Core**
1. Gap buffer structure + create/destroy (Day 1)
2. Gap movement + insert (Day 2-3)
3. Delete + line tracking (Day 3-4)
4. Arabic text format + rendering (Day 4-5)
5. Integration: render text in window (Day 5)

**Week 3: Console + Polish**
1. VT100 mode + font configuration (Day 1)
2. ANSI helpers + color output (Day 2)
3. Menu placeholder (Day 2)
4. Testing all acceptance criteria (Day 3-4)
5. Bug fixes + documentation (Day 5)

---

## 8. Quality Gates

### 8.1 Definition of Done (Sprint 1)

A component is complete when:

| # | Criterion |
|---|-----------|
| 1 | All acceptance criteria pass |
| 2 | No compiler warnings at `/W4` level |
| 3 | No memory leaks detected (debug build) |
| 4 | Code follows naming conventions |
| 5 | Public functions documented with Doxygen |
| 6 | Error paths tested |
| 7 | Works on Windows 10 1903+ |
| 8 | Graceful degradation on unsupported features |

### 8.2 Testing Requirements

**Unit Tests (Required for Sprint 1):**
- [ ] `test_buffer_create_empty` - Empty buffer has 1 line
- [ ] `test_buffer_insert_char` - Insert single character
- [ ] `test_buffer_insert_arabic` - Insert Arabic text
- [ ] `test_buffer_delete` - Delete at cursor
- [ ] `test_buffer_gap_movement` - Insert at different positions
- [ ] `test_console_utf8` - UTF-8 code page set
- [ ] `test_window_create` - Window created with RTL style

**Manual Tests (Checklist):**
- [ ] Arabic title bar displays correctly
- [ ] Window resize maintains minimum size
- [ ] Arabic text renders with proper ligatures
- [ ] Console displays Arabic output
- [ ] ANSI colors work in console
- [ ] No crashes during normal operation

### 8.3 Documentation Requirements

| Document | Content |
|----------|---------|
| Header comments | Doxygen-style for all public functions |
| README update | Build instructions, requirements |
| CHANGELOG | Sprint 1 changes |

---

## 9. Next Steps Checklist

### 9.1 Prerequisites Verification

- [ ] **Visual Studio 2019/2022** installed with C++ desktop development
- [ ] **Windows SDK 10.0.18362+** installed
- [ ] **CMake 3.16+** installed and in PATH
- [ ] **Git** configured for repository

### 9.2 Development Environment Setup

```batch
:: 1. Clone repository (if not already)
git clone <repository-url> qalam
cd qalam

:: 2. Create build directory
mkdir build
cd build

:: 3. Configure CMake
cmake .. -G "Visual Studio 17 2022" -A x64

:: 4. Build
cmake --build . --config Debug

:: 5. Run (placeholder message box appears)
.\bin\Debug\qalam.exe
```

### 9.3 Immediate Actions to Begin Implementation

| # | Action | Assignee | Status |
|---|--------|----------|--------|
| 1 | Verify build environment compiles current stub | Developer | ⬜ |
| 2 | Create `src/core/buffer.c` with gap buffer | Developer | ⬜ |
| 3 | Create `src/ui/window.c` with RTL window | Developer | ⬜ |
| 4 | Create `src/console/console.c` with UTF-8 init | Developer | ⬜ |
| 5 | Update `CMakeLists.txt` with new source files | Developer | ⬜ |
| 6 | Create `tests/test_buffer.c` with unit tests | Developer | ⬜ |
| 7 | Run all acceptance criteria tests | Developer | ⬜ |
| 8 | Update CHANGELOG.md with Sprint 1 | Developer | ⬜ |

### 9.4 Blockers Assessment

| Potential Blocker | Status | Resolution |
|-------------------|--------|------------|
| Windows SDK version | ✅ Clear | SDK 18362+ required, documented |
| ConPTY availability | ✅ Clear | Version detection implemented |
| DirectWrite availability | ✅ Clear | Windows 7+ with Platform Update |
| Pending user decisions | ⚠️ Open | P1, P2, P3 need answers before Phase 5 |

---

## 10. Appendix: Quick Reference

### 10.1 Key API Functions (from headers)

#### [`qalam.h`](include/qalam.h) - Core API

| Function | Purpose |
|----------|---------|
| [`qalam_init()`](include/qalam.h:216) | Initialize all subsystems |
| [`qalam_shutdown()`](include/qalam.h:224) | Cleanup and release resources |
| [`qalam_get_last_error()`](include/qalam.h:136) | Get thread-local error info |
| [`qalam_result_to_string()`](include/qalam.h:143) | Error code to message |

#### [`editor.h`](include/editor.h) - Buffer API

| Function | Purpose |
|----------|---------|
| [`qalam_buffer_create()`](include/editor.h:93) | Create empty buffer |
| [`qalam_buffer_destroy()`](include/editor.h:128) | Free buffer resources |
| [`qalam_buffer_insert()`](include/editor.h:142) | Insert text at cursor |
| [`qalam_buffer_delete()`](include/editor.h:162) | Delete characters |
| [`qalam_buffer_get_cursor()`](include/editor.h:198) | Get cursor position |
| [`qalam_buffer_set_cursor()`](include/editor.h:208) | Set cursor by line/column |

#### [`ui.h`](include/ui.h) - Window API

| Function | Purpose |
|----------|---------|
| [`qalam_window_create()`](include/ui.h:277) | Create Win32 window |
| [`qalam_window_destroy()`](include/ui.h:284) | Destroy window |
| [`qalam_window_show()`](include/ui.h:292) | Show window |
| [`qalam_window_run()`](include/ui.h:322) | Run message loop |
| [`qalam_window_set_rtl()`](include/ui.h:420) | Set RTL layout mode |
| [`qalam_window_begin_paint()`](include/ui.h:381) | Begin paint operation |
| [`qalam_window_end_paint()`](include/ui.h:389) | End paint and present |

#### [`terminal.h`](include/terminal.h) - Terminal API

| Function | Purpose |
|----------|---------|
| [`qalam_terminal_create()`](include/terminal.h:119) | Create ConPTY terminal |
| [`qalam_terminal_destroy()`](include/terminal.h:128) | Destroy terminal |
| [`qalam_terminal_spawn()`](include/terminal.h:141) | Spawn process |
| [`qalam_terminal_write()`](include/terminal.h:193) | Write to terminal |
| [`qalam_terminal_read()`](include/terminal.h:218) | Read from terminal |
| [`qalam_terminal_resize()`](include/terminal.h:259) | Resize terminal |

### 10.2 Windows API Calls Reference

#### Window Creation (RTL)

```c
DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_LAYOUTRTL | WS_EX_RTLREADING;
HWND hwnd = CreateWindowExW(dwExStyle, L"QalamMainWindow", L"قلم - Qalam IDE", ...);
```

#### DirectWrite Initialization

```c
DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&factory);
D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void**)&d2d);
```

#### UTF-8 Console

```c
SetConsoleOutputCP(CP_UTF8);
SetConsoleCP(CP_UTF8);
SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
```

#### ConPTY (Phase 5)

```c
CreatePseudoConsole(size, hInput, hOutput, 0, &hpc);
ResizePseudoConsole(hpc, size);
ClosePseudoConsole(hpc);
```

### 10.3 Build Commands

```batch
:: Configure (Debug)
cmake -B build -G "Visual Studio 17 2022" -A x64

:: Build Debug
cmake --build build --config Debug

:: Build Release
cmake --build build --config Release

:: Run
.\build\bin\Debug\qalam.exe
```

### 10.4 Error Code Quick Reference

| Code | Name | Meaning |
|------|------|---------|
| 0 | `QALAM_OK` | Success |
| 4 | `QALAM_ERROR_OUT_OF_MEMORY` | Allocation failed |
| 102 | `QALAM_ERROR_INVALID_POSITION` | Bad cursor position |
| 200 | `QALAM_ERROR_WINDOW_CREATE` | Window creation failed |
| 202 | `QALAM_ERROR_DIRECTWRITE_INIT` | DirectWrite init failed |
| 301 | `QALAM_ERROR_CONPTY_CREATE` | ConPTY creation failed |

---

## Sprint 1 Implementation Priority

**What to build first:**

1. **Gap Buffer (`src/core/buffer.c`)** - This is the foundation of all text editing. It has no dependencies on other Qalam components and can be fully unit tested in isolation.

2. **DirectWrite Singleton (`src/ui/dwrite.c`)** - Required before any window can render text. Initialize factories once, reuse everywhere.

3. **RTL Window (`src/ui/window.c`)** - Basic window with RTL styles, paint handling, and resize. Placeholder text rendering.

4. **Console UTF-8 (`src/console/console.c`)** - Enable Arabic output for debugging and future CLI tools.

---

## Blockers Before Coding

**None.** All prerequisites are documented and the codebase has:
- Working CMake build system
- Header files with complete API definitions
- Entry point stub that compiles and runs

**Pending decisions (P1, P2, P3) do not block Sprint 1** - they affect Phase 5 (Terminal) and can be resolved later.

---

*Document generated from analysis of project artifacts on 2026-01-05*
*Source documents: [`QALAM_ANALYSIS.md`](plans/QALAM_ANALYSIS.md), [`SPRINT_1_PLAN.md`](plans/SPRINT_1_PLAN.md), [`TECHNICAL_DECISIONS.md`](plans/TECHNICAL_DECISIONS.md)*