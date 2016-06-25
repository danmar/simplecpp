# preprocessor

C/C++ preprocessor

It is written primarily for Cppcheck. But hopefully it will be reused in other projects also. This is a simple preprocessor that is easy to use. It has no Cppcheck-dependencies.

Preprocessing usually hides details so that static analysis can't be done properly. This preprocessor tries to achieve high fidelity:
 * Preprocessor directives are saved.
 * Comments are saved.
 * Source code

