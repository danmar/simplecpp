cmake_minimum_required (VERSION 3.5)
project (simplecpp LANGUAGES CXX)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wextra -pedantic -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wredundant-decls -Wshadow -Wundef -Wold-style-cast -Wno-multichar)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Weverything)
    # no need for c++98 compatibility
    add_compile_options(-Wno-c++98-compat-pedantic)
    # these are not really fixable
    add_compile_options(-Wno-exit-time-destructors -Wno-global-constructors)
    # we are not interested in these
    add_compile_options(-Wno-multichar)
    # TODO: fix these?
    add_compile_options(-Wno-padded -Wno-sign-conversion -Wno-implicit-int-conversion -Wno-shorten-64-to-32)
endif()

add_executable(simplecpp simplecpp.cpp main.cpp)
set_property(TARGET simplecpp PROPERTY CXX_STANDARD 11)

# it is not possible to set a standard older than C++14 with Visual Studio
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # we need to create a dummy library as -fsyntax-only will not produce any output files causing the build to fail
    add_library(simplecpp-03-syntax STATIC simplecpp.cpp)
    target_compile_options(simplecpp-03-syntax PRIVATE -std=c++03)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(simplecpp-03-syntax PRIVATE -Wno-long-long)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(simplecpp-03-syntax PRIVATE -Wno-c++11-long-long -Wno-c++11-compat)
    endif()
    add_dependencies(simplecpp simplecpp-03-syntax)
endif()

add_executable(testrunner simplecpp.cpp test.cpp)
set_property(TARGET testrunner PROPERTY CXX_STANDARD 11)

enable_testing()
add_test(NAME testrunner COMMAND testrunner)
