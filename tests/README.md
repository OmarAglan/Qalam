# Qalam IDE - Tests

This directory contains unit tests for the Qalam IDE project.

## Test Structure

```
tests/
├── test_buffer.c      # Gap buffer unit tests
├── test_cursor.c      # Cursor operations tests
├── test_unicode.c     # Unicode/UTF-8 handling tests
├── test_rtl.c         # RTL text detection tests
└── test_main.c        # Test runner entry point
```

## Running Tests

Tests will be integrated with CMake's CTest framework:

```bash
# Build and run tests
cmake --build build --target test

# Or using ctest directly
cd build
ctest --verbose
```

## Test Framework

The project will use a lightweight C testing approach:
- Simple assertion macros
- Test registration system
- Colored output for pass/fail

## Coverage

Future versions will include:
- Code coverage reporting
- Integration tests for window/terminal
- Performance benchmarks for buffer operations