cmake_minimum_required (VERSION 3.5)
project (simplecpp LANGUAGES CXX)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -pedantic -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wredundant-decls -Wshadow -Wundef)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Weverything)
    # no need for c++98 compatibility
    add_compile_options(-Wno-c++98-compat-pedantic)
    # these are not really fixable
    add_compile_options(-Wno-exit-time-destructors -Wno-global-constructors)
    # TODO: fix these?
    add_compile_options(-Wno-zero-as-null-pointer-constant -Wno-padded -Wno-sign-conversion -Wno-conversion -Wno-old-style-cast)
endif()

add_executable(simplecpp simplecpp.cpp main.cpp)
set_property(TARGET simplecpp PROPERTY CXX_STANDARD 11)

add_library(simplecpp-03-syntax STATIC simplecpp.cpp)
target_compile_options(simplecpp-03-syntax PRIVATE -std=c++03)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(simplecpp-03-syntax PRIVATE -Wno-long-long)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(simplecpp-03-syntax PRIVATE -Wno-c++11-long-long)
endif()
add_dependencies(simplecpp simplecpp-03-syntax)

add_executable(testrunner simplecpp.cpp test.cpp)
set_property(TARGET testrunner PROPERTY CXX_STANDARD 11)
