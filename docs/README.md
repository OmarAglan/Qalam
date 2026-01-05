# Qalam IDE - Documentation

Welcome to the Qalam IDE documentation. Qalam (قلم, meaning "pen" in Arabic) is an Arabic-first Integrated Development Environment designed for native RTL text support.

## Documentation Structure

```
docs/
├── README.md           # This file
├── architecture/       # System architecture documentation
│   ├── overview.md     # High-level architecture
│   ├── buffer.md       # Gap buffer design
│   ├── rendering.md    # DirectWrite rendering
│   └── terminal.md     # ConPTY integration
├── api/                # API reference documentation
│   ├── qalam.md        # Core API
│   ├── editor.md       # Editor/buffer API
│   ├── terminal.md     # Terminal API
│   └── ui.md           # UI/window API
├── guides/             # Developer guides
│   ├── building.md     # Build instructions
│   ├── contributing.md # Contribution guidelines
│   └── arabic-text.md  # Arabic text handling guide
└── user/               # End-user documentation
    ├── quickstart.md   # Getting started
    └── keyboard.md     # Keyboard shortcuts
```

## Quick Start

### Building from Source

```bash
# Requirements: CMake 3.16+, Visual Studio 2019+ or MSVC Build Tools

# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# Run
./build/bin/Release/qalam.exe
```

### System Requirements

- **OS**: Windows 10 Build 18362+ (Version 1903 or later)
- **Compiler**: MSVC with C11 support
- **Libraries**: DirectWrite, Direct2D (included with Windows)

## Key Features (Planned)

- 🔤 **Native Arabic Support**: Full RTL text editing with proper shaping
- 📝 **Efficient Editing**: Gap buffer for fast text manipulation
- 🖥️ **Embedded Terminal**: ConPTY-based terminal with Arabic support
- 🎨 **Modern UI**: Direct2D/DirectWrite rendering
- ⚡ **Performance**: Native C implementation

## API Overview

### Core Types

| Type | Description |
|------|-------------|
| `QalamEditor` | Main editor session manager |
| `QalamBuffer` | Text buffer with gap buffer implementation |
| `QalamTerminal` | ConPTY terminal wrapper |
| `QalamWindow` | Win32 window with DirectWrite context |

### Error Handling

All functions return `QalamResult`:
- `QALAM_OK` (0) - Success
- `QALAM_ERROR_*` - Various error codes

Use `qalam_get_last_error()` for detailed error information.

## License

[License information to be added]

## Contact

[Contact information to be added]