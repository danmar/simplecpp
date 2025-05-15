## test with python -m pytest integration_test.py

import os
import pathlib
import pytest
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
                const int dummy = 1;
                """)
    return header_file, "error: #error header_was_already_included"

def __test_relative_header_create_source(dir, include1, include2, is_include1_sys=False, is_include2_sys=False, inv=False):
    if inv:
        return __test_relative_header_create_source(dir, include1=include2, include2=include1, is_include1_sys=is_include2_sys, is_include2_sys=is_include1_sys)
    ## otherwise

    src_file = os.path.join(dir, 'test.c')
    with open(src_file, 'wt') as f:
        f.write(f"""
                #undef TEST_H_INCLUDED
                #include {format_include(include1, is_include1_sys)}
                #include {format_include(include2, is_include2_sys)}
                """)
    return src_file

@pytest.mark.parametrize("with_pragma_once", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
def test_relative_header_1(tmpdir, with_pragma_once, is_sys):
    _, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=with_pragma_once)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", "test.h", is_include1_sys=is_sys, is_include2_sys=is_sys)

    args = ([format_include_path_arg(tmpdir)] if is_sys else []) + [test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)

    if with_pragma_once:
        assert stderr == ''
    else:
        assert double_include_error in stderr

@pytest.mark.parametrize("inv", (False, True))
@pytest.mark.parametrize("source_relative", (False, True))
def test_relative_header_2(tmpdir, inv, source_relative):
    header_file, _ = __test_relative_header_create_header(tmpdir)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", header_file, inv=inv)

    args = ["test.c" if source_relative else test_file]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''
    if source_relative and not inv:
        assert '#line 8 "test.h"' in stdout
    else:
        assert f'#line 8 "{pathlib.PurePosixPath(tmpdir)}/test.h"' in stdout

@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
@pytest.mark.parametrize("source_relative", (False, True))
def test_relative_header_3(tmpdir, is_sys, inv, source_relative):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)

    test_file = __test_relative_header_create_source(tmpdir, "test_subdir/test.h", header_file, is_include1_sys=is_sys, inv=inv)

    args = ["test.c" if source_relative else test_file]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)

    if is_sys:
        assert "missing header: Header not found" in stderr
    else:
        assert stderr == ''
        if source_relative and not inv:
            assert '#line 8 "test_subdir/test.h"' in stdout
        else:
            assert f'#line 8 "{pathlib.PurePosixPath(test_subdir)}/test.h"' in stdout

@pytest.mark.parametrize("use_short_path", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
def test_relative_header_4(tmpdir, use_short_path, is_sys, inv):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)
    if use_short_path:
        header_file = "test_subdir/test.h"

    test_file = __test_relative_header_create_source(tmpdir, header_file, "test.h", is_include2_sys=is_sys, inv=inv)

    args = [format_include_path_arg(test_subdir), test_file]

    _, _, stderr = simplecpp(args, cwd=tmpdir)
    assert stderr == ''
