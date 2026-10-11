cmake_minimum_required(VERSION 3.16)
project(CompilerCPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Release build for single-config generators (Makefiles, Ninja).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Core compiler/interpreter sources, shared by the CLI and the tests.
add_library(compiler_core STATIC
    CompilerC++/Scanner.cpp
    CompilerC++/Tree.cpp
    CompilerC++/Diagram.cpp
)
target_include_directories(compiler_core PUBLIC CompilerC++)

# Command-line front end.
add_executable(translator CompilerC++/CompilerC++.cpp)
target_link_libraries(translator PRIVATE compiler_core)

# Compiler warnings.
if(MSVC)
    target_compile_options(compiler_core PRIVATE /W4 /permissive-)
    target_compile_options(translator   PRIVATE /W4 /permissive-)
else()
    target_compile_options(compiler_core PRIVATE -Wall -Wextra)
    target_compile_options(translator   PRIVATE -Wall -Wextra)
endif()

# Unit and integration tests.
enable_testing()
add_executable(compiler_tests
    CompilerTests/CompilerTests.cpp
    CompilerTests/test_scanner.cpp
    CompilerTests/test_semantics.cpp
    CompilerTests/test_expressions.cpp
    CompilerTests/test_interpreter.cpp
)
target_link_libraries(compiler_tests PRIVATE compiler_core)
add_test(NAME compiler_tests COMMAND compiler_tests)
