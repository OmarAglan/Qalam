#=============================================================================
# Qalam IDE - CMake Build Configuration
# Arabic-first Integrated Development Environment
#=============================================================================

cmake_minimum_required(VERSION 3.16)

project(Qalam
    VERSION 0.0.1
    DESCRIPTION "Arabic-first Integrated Development Environment"
    LANGUAGES C
)

#-----------------------------------------------------------------------------
# C Standard Configuration
#-----------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#-----------------------------------------------------------------------------
# Windows Platform Requirements
# Target: Windows 10 Build 18362+ (Version 1903)
#-----------------------------------------------------------------------------
if(WIN32)
    # Windows 10 version 1903 (Build 18362) minimum
    add_compile_definitions(
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
        NTDDI_VERSION=0x0A000007
    )
    
    # Enable Unicode support throughout
    add_compile_definitions(
        UNICODE
        _UNICODE
    )
    
    # Qalam version definition
    add_compile_definitions(
        QALAM_VERSION="0.0.1"
    )
endif()

#-----------------------------------------------------------------------------
# Compiler Warnings
#-----------------------------------------------------------------------------
if(MSVC)
    # High warning level for MSVC
    add_compile_options(/W4)
    # Treat specific warnings as errors (optional, can be enabled later)
    # add_compile_options(/WX)
    
    # UTF-8 source and execution character set
    add_compile_options(/utf-8)
else()
    # GCC/Clang warnings (for potential cross-compilation or MinGW)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#-----------------------------------------------------------------------------
# Include Directories
#-----------------------------------------------------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

#-----------------------------------------------------------------------------
# Source Files
#-----------------------------------------------------------------------------
set(QALAM_SOURCES
    src/main.c
    # Core subsystem sources
    src/core/buffer.c
    # src/core/cursor.c
    
    # UI subsystem sources (to be added)
    # src/ui/window.c
    # src/ui/render.c
    
    # Console subsystem sources (to be added)
    # src/console/arabic_console.c
    
    # Terminal subsystem sources (to be added)
    # src/terminal/conpty.c
)

#-----------------------------------------------------------------------------
# Core Library (for sharing between main executable and tests)
#-----------------------------------------------------------------------------
set(QALAM_CORE_SOURCES
    src/core/buffer.c
)

#-----------------------------------------------------------------------------
# Qalam Executable
#-----------------------------------------------------------------------------
add_executable(qalam WIN32 ${QALAM_SOURCES})

#-----------------------------------------------------------------------------
# Link Libraries (Windows-specific)
#-----------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(qalam PRIVATE
        # DirectWrite and Direct2D for text rendering
        dwrite
        d2d1
        
        # Desktop Window Manager
        dwmapi
        
        # Core Windows libraries
        user32
        gdi32
        shell32
        ole32
        
        # Additional libraries for ConPTY (Phase 5)
        # kernel32 is linked by default
    )
endif()

#-----------------------------------------------------------------------------
# Output Configuration
#-----------------------------------------------------------------------------
set_target_properties(qalam PROPERTIES
    OUTPUT_NAME "qalam"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

#-----------------------------------------------------------------------------
# Installation (optional, for future use)
#-----------------------------------------------------------------------------
install(TARGETS qalam
    RUNTIME DESTINATION bin
)

#-----------------------------------------------------------------------------
# Testing Configuration
#-----------------------------------------------------------------------------
enable_testing()

# Test executable for buffer tests
add_executable(test_buffer
    tests/test_buffer.c
    ${QALAM_CORE_SOURCES}
)

target_include_directories(test_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(test_buffer PRIVATE
        user32
        kernel32
    )
endif()

set_target_properties(test_buffer PROPERTIES
    OUTPUT_NAME "test_buffer"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Register test with CTest
add_test(NAME BufferTests COMMAND test_buffer)

message(STATUS "Qalam IDE Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests: Enabled")