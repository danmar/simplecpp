cmake_minimum_required (VERSION 3.5)
project (simplecpp)

add_executable(simplecpp simplecpp.cpp main.cpp)
set_property(TARGET simplecpp PROPERTY CXX_STANDARD 11)

add_executable(testrunner simplecpp.cpp test.cpp)
set_property(TARGET testrunner PROPERTY CXX_STANDARD 11)
