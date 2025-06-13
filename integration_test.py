## test with python -m pytest integration_test.py

import os
import pathlib
import platform
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
def test_relative_header_1(record_property, tmpdir, with_pragma_once, is_sys):
    _, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=with_pragma_once)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", "test.h", is_include1_sys=is_sys, is_include2_sys=is_sys)

    args = ([format_include_path_arg(tmpdir)] if is_sys else []) + [test_file]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)

    if with_pragma_once:
        assert stderr == ''
    else:
        assert double_include_error in stderr

@pytest.mark.parametrize("with_pragma_once", (False, True))
@pytest.mark.parametrize("inv", (False, True))
@pytest.mark.parametrize("source_relative", (False, True))
def test_relative_header_2(record_property, tmpdir, with_pragma_once, inv, source_relative):
    header_file, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=with_pragma_once)

    test_file = __test_relative_header_create_source(tmpdir, "test.h", header_file, inv=inv)

    args = ["test.c" if source_relative else test_file]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)
    if with_pragma_once:
        assert stderr == ''
        if inv:
            assert f'#line 8 "{pathlib.PurePath(tmpdir).as_posix()}/test.h"' in stdout
        else:
            assert '#line 8 "test.h"' in stdout
    else:
        assert double_include_error in stderr

@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
@pytest.mark.parametrize("source_relative", (False, True))
def test_relative_header_3(record_property, tmpdir, is_sys, inv, source_relative):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)

    test_file = __test_relative_header_create_source(tmpdir, "test_subdir/test.h", header_file, is_include1_sys=is_sys, inv=inv)

    args = ["test.c" if source_relative else test_file]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)

    if is_sys:
        assert "missing header: Header not found" in stderr
    else:
        assert stderr == ''
        if inv:
            assert f'#line 8 "{pathlib.PurePath(test_subdir).as_posix()}/test.h"' in stdout
        else:
            assert '#line 8 "test_subdir/test.h"' in stdout

@pytest.mark.parametrize("use_short_path", (False, True))
@pytest.mark.parametrize("relative_include_dir", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
def test_relative_header_4(record_property, tmpdir, use_short_path, relative_include_dir, is_sys, inv):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)
    if use_short_path:
        header_file = "test_subdir/test.h"

    test_file = __test_relative_header_create_source(tmpdir, header_file, "test.h", is_include2_sys=is_sys, inv=inv)

    args = [format_include_path_arg("test_subdir" if relative_include_dir else test_subdir), test_file]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)
    assert stderr == ''
    if (use_short_path and not inv) or (relative_include_dir and inv):
        assert '#line 8 "test_subdir/test.h"' in stdout
    else:
        assert f'#line 8 "{pathlib.PurePath(test_subdir).as_posix()}/test.h"' in stdout

@pytest.mark.parametrize("with_pragma_once", (False, True))
@pytest.mark.parametrize("relative_include_dir", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
def test_relative_header_5(record_property, tmpdir, with_pragma_once, relative_include_dir, is_sys, inv): # test relative paths with ..
    ## in this test, the subdir role is the opposite then the previous - it contains the test.c file, while the parent tmpdir contains the header file
    header_file, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=with_pragma_once)
    if is_sys:
        header_file_second_path = "test.h"
    else:
        header_file_second_path = "../test.h"

    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    test_file = __test_relative_header_create_source(test_subdir, header_file, header_file_second_path, is_include2_sys=is_sys, inv=inv)

    args = ([format_include_path_arg(".." if relative_include_dir else tmpdir)] if is_sys else []) + ["test.c"]

    _, stdout, stderr = simplecpp(args, cwd=test_subdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)
    if with_pragma_once:
        assert stderr == ''
        if (relative_include_dir or not is_sys) and inv:
            assert '#line 8 "../test.h"' in stdout
        else:
            assert f'#line 8 "{pathlib.PurePath(tmpdir).as_posix()}/test.h"' in stdout
    else:
        assert double_include_error in stderr

@pytest.mark.parametrize("with_pragma_once", (False, True))
@pytest.mark.parametrize("relative_include_dir", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
def test_relative_header_6(record_property, tmpdir, with_pragma_once, relative_include_dir, is_sys, inv): # test relative paths with .. that is resolved only by an include dir
    ## in this test, both the header and the source file are at the same dir, but there is a dummy inclusion dir as a subdir
    header_file, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=with_pragma_once)

    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    test_file = __test_relative_header_create_source(tmpdir, header_file, "../test.h", is_include2_sys=is_sys, inv=inv)

    args = [format_include_path_arg("test_subdir" if relative_include_dir else test_subdir), "test.c"]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)
    if with_pragma_once:
        assert stderr == ''
        if relative_include_dir and inv:
            assert '#line 8 "test.h"' in stdout
        else:
            assert f'#line 8 "{pathlib.PurePath(tmpdir).as_posix()}/test.h"' in stdout
    else:
        assert double_include_error in stderr

def test_same_name_header(record_property, tmpdir):
    include_a = os.path.join(tmpdir, "include_a")
    include_b = os.path.join(tmpdir, "include_b")

    test_file = os.path.join(tmpdir, "test.c")
    header_a = os.path.join(include_a, "header_a.h")
    header_b = os.path.join(include_b, "header_b.h")
    same_name_a = os.path.join(include_a, "same_name.h")
    same_name_b = os.path.join(include_b, "same_name.h")

    os.mkdir(include_a)
    os.mkdir(include_b)

    with open(test_file, "wt") as f:
        f.write("""
                #include <header_a.h>
                #include <header_b.h>
                TEST
                """)

    with open(header_a, "wt") as f:
        f.write("""
                #include "same_name.h"
                """)

    with open(header_b, "wt") as f:
        f.write("""
                #include "same_name.h"
                """)

    with open(same_name_a, "wt") as f:
        f.write("""
                #define TEST E
                """)

    with open(same_name_b, "wt") as f:
        f.write("""
                #define TEST OK
                """)

    args = [
        format_include_path_arg(include_a),
        format_include_path_arg(include_b),
        test_file
    ]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)

    assert "OK" in stdout
    assert stderr == ""

def test_pragma_once_matching(record_property, tmpdir):
    if platform.system() == "win32":
        names_to_test = [
            '"once.h"',
            '"Once.h"',
            '<once.h>',
            '<Once.h>',
            '"../test_dir/once.h"',
            '"../test_dir/Once.h"',
            '"../Test_Dir/once.h"',
            '"../Test_Dir/Once.h"',
            '"test_subdir/../once.h"',
            '"test_subdir/../Once.h"',
            '"Test_Subdir/../once.h"',
            '"Test_Subdir/../Once.h"',
        ]
    else:
        names_to_test = [
            '"once.h"',
            '<once.h>',
            '"../test_dir/once.h"',
            '"test_subdir/../once.h"',
        ]

    test_dir = os.path.join(tmpdir, "test_dir")
    test_subdir = os.path.join(test_dir, "test_subdir")

    test_file = os.path.join(test_dir, "test.c")
    once_header = os.path.join(test_dir, "once.h")

    os.mkdir(test_dir)
    os.mkdir(test_subdir)

    with open(test_file, "wt") as f:
        for n in names_to_test:
            f.write(f"""
                    #include {n}
                    """);

    with open(once_header, "wt") as f:
        f.write(f"""
                #pragma once
                ONCE
                """);

    args = [
        format_include_path_arg(test_dir),
        test_file
    ]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)

    assert stdout.count("ONCE") == 1
    assert stderr == ""
