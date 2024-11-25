cmake_minimum_required (VERSION 3.10)
project (simplecpp LANGUAGES CXX)

option(DISABLE_CPP03_SYNTAX_CHECK "Disable the C++03 syntax check." OFF)

include(CheckCXXCompilerFlag)

if (WIN32)
    # prevent simplifyPath_cppcheck() from wasting time on looking for a hypothetical network host
    add_definitions(-DUNCHOST=$ENV{COMPUTERNAME})
endif()

function(add_compile_options_safe FLAG)
    string(MAKE_C_IDENTIFIER "HAS_CXX_FLAG${FLAG}" mangled_flag)
    check_cxx_compiler_flag(${FLAG} ${mangled_flag})
    if (${mangled_flag})
        add_compile_options(${FLAG})
    endif()
endfunction()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-pedantic)
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-Wcast-qual)                # Cast for removing type qualifiers
    add_compile_options(-Wfloat-equal)              # Floating values used in equality comparisons
    add_compile_options(-Wmissing-declarations)     # If a global function is defined without a previous declaration
    add_compile_options(-Wmissing-format-attribute) #
    add_compile_options(-Wno-long-long)
    add_compile_options(-Wpacked)                   #
    add_compile_options(-Wredundant-decls)          # if anything is declared more than once in the same scope
    add_compile_options(-Wundef)
    add_compile_options(-Wno-missing-braces)
    add_compile_options(-Wno-sign-compare)
    add_compile_options(-Wno-multichar)
    add_compile_options(-Woverloaded-virtual)       # when a function declaration hides virtual functions from a base class

    add_compile_options(-Wsuggest-attribute=noreturn)
    add_compile_options_safe(-Wuseless-cast)
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
