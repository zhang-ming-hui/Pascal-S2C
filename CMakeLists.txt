cmake_minimum_required(VERSION 3.20)

project(pascal_s2c LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pascal_s2c
    src/main.cpp
    src/common/error.cpp
    src/ast/ast.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/semantic/analyzer.cpp
    src/lower/lower.cpp
    src/codegen/c_writer.cpp
    src/codegen/c_codegen.cpp
    src/driver/compiler.cpp
)

target_include_directories(pascal_s2c PRIVATE src)

if(MSVC)
    target_compile_options(pascal_s2c PRIVATE /W4 /permissive-)
    target_link_options(pascal_s2c PRIVATE /STACK:16777216)
else()
    target_compile_options(pascal_s2c PRIVATE -Wall -Wextra -Wpedantic)
endif()

