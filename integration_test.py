## test with python -m pytest integration_test.py

import os
from testutils import simplecpp, format_include_path_arg, format_include

def __test_relative_header_create_header(dir, with_pragma_once=True):
    header_file = os.path.join(dir, 'test.h')
    with open(header_file, 'wt') as f:
        f.write(f"""
                {"#pragma once" if with_pragma_once else ""}
                #ifndef TEST_H_INCLUDED
                #define TEST_H_INCLUDED
                #else
                #error header_was_already_included
                #endif
                """)
    return header_file, "error: #error header_was_already_included"

def __test_relative_header_create_source(dir, include1, include2, is_include1_sys=False, is_include2_sys=False):
    src_file = os.path.join(dir, 'test.c')
    with open(src_file, 'wt') as f:
        f.write(f"""
                #undef TEST_H_INCLUDED
                #include {format_include(include1, is_include1_sys)}
                #include {format_include(include2, is_include2_sys)}
                """)
    return src_file


def test_relative_header_0_rel(tmpdir):
    _, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=False)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", "test.h")

    args = [test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert double_include_error in stderr

def test_relative_header_0_sys(tmpdir):
    _, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=False)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", "test.h", is_include1_sys=True, is_include2_sys=True)

    args = [format_include_path_arg(tmpdir), test_file]

    _, _, stderr = simplecpp(args)
    assert double_include_error in stderr

def test_relative_header_1_rel(tmpdir):
    __test_relative_header_create_header(tmpdir)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", "test.h")

    args = [test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''

def test_relative_header_1_sys(tmpdir):
    __test_relative_header_create_header(tmpdir)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", "test.h", is_include1_sys=True, is_include2_sys=True)

    args = [format_include_path_arg(tmpdir), test_file]

    _, _, stderr = simplecpp(args)
    assert stderr == ''

## TODO: the following tests should pass after applying simplecpp#362

def test_relative_header_2(tmpdir):
    header_file, _ = __test_relative_header_create_header(tmpdir)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", header_file)

    args = [test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''

def test_relative_header_3(tmpdir):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)

    test_file = __test_relative_header_create_source(tmpdir, "test_subdir/test.h", header_file)

    args = [test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''

def test_relative_header_3_inv(tmpdir):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)

    test_file = __test_relative_header_create_source(tmpdir, header_file, "test_subdir/test.h")

    args = [test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''


def test_relative_header_4(tmpdir):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)

    test_file = __test_relative_header_create_source(tmpdir, header_file, "test.h", is_include2_sys=True)

    args = [format_include_path_arg(test_subdir), test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''



def test_relative_header_5(tmpdir):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    __test_relative_header_create_header(test_subdir)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", "test_subdir/test.h", is_include1_sys=True)

    args = [format_include_path_arg(test_subdir), test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''
