cmake_minimum_required (VERSION 3.5)
project (simplecpp LANGUAGES CXX)

option(DISABLE_CPP03_SYNTAX_CHECK "Disable the C++03 syntax check." OFF)

include(CheckCXXCompilerFlag)

function(add_compile_options_safe FLAG)
    string(MAKE_C_IDENTIFIER "HAS_CXX_FLAG${FLAG}" mangled_flag)
    check_cxx_compiler_flag(${FLAG} ${mangled_flag})
    if (${mangled_flag})
        add_compile_options(${FLAG})
    endif()
endfunction()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wextra -pedantic -Wcast-qual -Wfloat-equal -Wmissing-declarations -Wmissing-format-attribute -Wredundant-decls -Wshadow -Wundef -Wold-style-cast -Wno-multichar)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Weverything)
    # no need for c++98 compatibility
    add_compile_options(-Wno-c++98-compat-pedantic)
    # these are not really fixable
    add_compile_options(-Wno-exit-time-destructors -Wno-global-constructors -Wno-weak-vtables)
    add_compile_options_safe(-Wno-unsafe-buffer-usage)
    # we are not interested in these
    add_compile_options(-Wno-multichar -Wno-four-char-constants)
    # ignore C++11-specific warning
    add_compile_options(-Wno-suggest-override -Wno-suggest-destructor-override)
    # contradicts -Wcovered-switch-default
    add_compile_options(-Wno-switch-default)
    # TODO: fix these?
    add_compile_options(-Wno-padded -Wno-sign-conversion -Wno-implicit-int-conversion -Wno-shorten-64-to-32 -Wno-shadow-field-in-constructor)

    if (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 14 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 14)
        # TODO: verify this regression still exists in clang-15
        if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            # work around performance regression - see https://github.com/llvm/llvm-project/issues/53555
            add_compile_options(-mllvm -inline-deferral)
        endif()

        # use force DWARF 4 debug format since not all tools might be able to handle DWARF 5 yet - e.g. valgrind on ubuntu 20.04
        add_compile_options(-gdwarf-4)
    endif()
endif()

add_library(simplecpp_obj OBJECT simplecpp.cpp)

add_executable(simplecpp $<TARGET_OBJECTS:simplecpp_obj> main.cpp)
set_property(TARGET simplecpp PROPERTY CXX_STANDARD 11)

if (NOT DISABLE_CPP03_SYNTAX_CHECK)
    # it is not possible to set a standard older than C++14 with Visual Studio
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # we need to create a dummy library as -fsyntax-only will not produce any output files causing the build to fail
        add_library(simplecpp-03-syntax OBJECT simplecpp.cpp)
        target_compile_options(simplecpp-03-syntax PRIVATE -std=c++03)
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_compile_options(simplecpp-03-syntax PRIVATE -Wno-long-long)
        elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(simplecpp-03-syntax PRIVATE -Wno-c++11-long-long -Wno-c++11-compat)
        endif()
        add_dependencies(simplecpp simplecpp-03-syntax)
    endif()
endif()

add_executable(testrunner $<TARGET_OBJECTS:simplecpp_obj> test.cpp)
set_property(TARGET testrunner PROPERTY CXX_STANDARD 11)

enable_testing()
add_test(NAME testrunner COMMAND testrunner)
