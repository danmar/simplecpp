Simple C/C++ preprocessor

This is a simple and easy to use preprocessor.

Written primarily for Cppcheck. But hopefully it will be reused in other projects also. There are no Cppcheck dependencies.

The intention is that this preprocessor will have good fidelity.
 * Preprocessor directives will be saved.
 * Comments will be saved.
 * Tracking which macro is expanded
This information is normally lost during preprocessing but it can be necessary for proper static analysis.
