# Simple C/C++ preprocessor

This is a simple C/C++ preprocessor.

The goal is to have good conformance with the C and C++ standards and to handle nonstandard preprocessor extensions in gcc / clang / visual studio preprocessors. Most of the preprocessor testcases in gcc and clang are handled OK by simplecpp.

To see how you can use simplecpp in your project, you can look at the file main.cpp.

Simplecpp has better fidelity than normal C/C++ preprocessors.
 * Preprocessor directives are available.
 * Comments are available.
 * Tracking macro usage.

This information is normally lost during preprocessing but it can be necessary for proper static analysis.

## Status
![CI-windows](https://github.com/danmar/simplecpp/workflows/CI-windows/badge.svg)
![CI Unixish](https://github.com/danmar/simplecpp/workflows/CI%20Unixish/badge.svg)


## Compiling

Compiling standalone simplecpp preprocessor:

Either:

    g++ -o simplecpp main.cpp simplecpp.cpp

Or:

    make


Compiling and running tests (you need python to run the gcc/clang test cases)

    make test

