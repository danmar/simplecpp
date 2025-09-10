## test with python -m pytest integration_test.py

import os
import pathlib
import platform
import pytest
from testutils import (
    simplecpp,
    format_include_path_arg,
    format_isystem_path_arg,
    format_framework_path_arg,
    format_iframework_path_arg,
    format_include,
)

def __test_create_header(dir, hdr_relpath, with_pragma_once=True, already_included_error_msg=None):
    """
    Creates a C header file under `dir/hdr_relpath` with simple include guards.

    The file contains:
    - optional `#pragma once` (when `with_pragma_once=True`)
    - a header guard derived from `hdr_relpath` (e.g. "test.h" -> TEST_H_INCLUDED)
    - optional `#error <already_included_error_msg>` if the guard is already defined
    - a dummy non-preprocessor declaration to force line emission

    Return absolute path to the created header file.
    """
    inc_guard = hdr_relpath.upper().replace(".", "_") # "test.h" -> "TEST_H"
    header_file = os.path.join(dir, hdr_relpath)
    os.makedirs(os.path.dirname(header_file), exist_ok=True)
    with open(header_file, 'wt') as f:
        f.write(f"""
                {"#pragma once" if with_pragma_once else ""}
                #ifndef {inc_guard}_INCLUDED
                #define {inc_guard}_INCLUDED
                #else
                {f"#error {already_included_error_msg}" if already_included_error_msg else ""}
                #endif
                int __force_line_emission; /* anything non-preprocessor */
                """)
    return header_file

def __test_create_source(dir, include, is_include_sys=False):
    """
    Creates a minimal C source file that includes a single header.

    The generated `<dir>/test.c` contains one `#include` directive.
    If `is_include_sys` is True, the include is written as `<...>`;
    otherwise it is written as `"..."`.

    Returns absolute path to the created header file.
    """
    src_file = os.path.join(dir, 'test.c')
    with open(src_file, 'wt') as f:
        f.write(f"""
                #include {format_include(include, is_include_sys)}
                """)
    return src_file

def __test_create_framework(dir, fw_name, hdr_relpath, content="", private=False):
    """
    Creates a minimal Apple-style framework layout containing one header.

    The generated structure is:
      `<dir>/<fw_name>.framework/{Headers|PrivateHeaders}/<hdr_relpath>`

    The header file contains the given `content` followed by a dummy
    declaration to force line emission.

    Returns absolute path to the created header file.
    """
    fwdir = os.path.join(dir, f"{fw_name}.framework", "PrivateHeaders" if private else "Headers")
    header_file = os.path.join(fwdir, hdr_relpath)
    os.makedirs(os.path.dirname(header_file), exist_ok=True)
    with open(header_file, "wt", encoding="utf-8") as f:
        f.write(f"""
                {content}
                int __force_line_emission; /* anything non-preprocessor */
                """)
    return header_file

def __test_relative_header_create_header(dir, with_pragma_once=True):
    """
    Creates a local `test.h` header with both `#pragma once` (optional)
    and a macro guard.

    The header emits `#error header_was_already_included` if it is
    re-included past the guard.

    Returns tuple of:
    - absolute path to the created header file
    - expected compiler error substring for duplicate inclusion
    """
    already_included_error_msg="header_was_already_included"
    header_file = __test_create_header(
        dir, "test.h", with_pragma_once=with_pragma_once, already_included_error_msg=already_included_error_msg)
    return header_file, f"error: #error {already_included_error_msg}"

def __test_relative_header_create_source(dir, include1, include2, is_include1_sys=False, is_include2_sys=False, inv=False):
    """
    Creates a C source file that includes two headers in order.

    The generated `<dir>/test.c`:
    - `#undef TEST_H_INCLUDED` to reset the guard in `test.h`
    - includes `include1` then `include2`
    - if `inv=True`, the order is swapped (`include2` then `include1`)
    - each include can be written as `<...>` or `"..."`

    Returns absolute path to the created source file.
    """
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
        if inv or not source_relative:
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
        if source_relative and not inv:
            assert '#line 8 "test_subdir/test.h"' in stdout
        else:
            assert f'#line 8 "{pathlib.PurePath(test_subdir).as_posix()}/test.h"' in stdout

@pytest.mark.parametrize("use_short_path", (False, True))
@pytest.mark.parametrize("relative_include_dir", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
@pytest.mark.parametrize("source_relative", (False, True))
def test_relative_header_4(record_property, tmpdir, use_short_path, relative_include_dir, is_sys, inv, source_relative):
    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    header_file, _ = __test_relative_header_create_header(test_subdir)
    if use_short_path:
        header_file = "test_subdir/test.h"

    test_file = __test_relative_header_create_source(tmpdir, header_file, "test.h", is_include2_sys=is_sys, inv=inv)

    args = [format_include_path_arg("test_subdir" if relative_include_dir else test_subdir), "test.c" if source_relative else test_file]

    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)

    assert stderr == ''
    if (source_relative and use_short_path and not inv) or (relative_include_dir and inv):
        assert '#line 8 "test_subdir/test.h"' in stdout
    else:
        assert f'#line 8 "{pathlib.PurePath(test_subdir).as_posix()}/test.h"' in stdout

@pytest.mark.parametrize("with_pragma_once", (False, True))
@pytest.mark.parametrize("relative_include_dir", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
@pytest.mark.parametrize("source_relative", (False, True))
def test_relative_header_5(record_property, tmpdir, with_pragma_once, relative_include_dir, is_sys, inv, source_relative): # test relative paths with ..
    ## in this test, the subdir role is the opposite then the previous - it contains the test.c file, while the parent tmpdir contains the header file
    header_file, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=with_pragma_once)
    if is_sys:
        header_file_second_path = "test.h"
    else:
        header_file_second_path = "../test.h"

    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    test_file = __test_relative_header_create_source(test_subdir, header_file, header_file_second_path, is_include2_sys=is_sys, inv=inv)

    args = ([format_include_path_arg(".." if relative_include_dir else tmpdir)] if is_sys else []) + ["test.c" if source_relative else test_file]

    _, stdout, stderr = simplecpp(args, cwd=test_subdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)
    if with_pragma_once:
        assert stderr == ''
        if (relative_include_dir if is_sys else source_relative) and inv:
            assert '#line 8 "../test.h"' in stdout
        else:
            assert f'#line 8 "{pathlib.PurePath(tmpdir).as_posix()}/test.h"' in stdout
    else:
        assert double_include_error in stderr

@pytest.mark.parametrize("with_pragma_once", (False, True))
@pytest.mark.parametrize("relative_include_dir", (False, True))
@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("inv", (False, True))
@pytest.mark.parametrize("source_relative", (False, True))
def test_relative_header_6(record_property, tmpdir, with_pragma_once, relative_include_dir, is_sys, inv, source_relative): # test relative paths with .. that is resolved only by an include dir
    ## in this test, both the header and the source file are at the same dir, but there is a dummy inclusion dir as a subdir
    header_file, double_include_error = __test_relative_header_create_header(tmpdir, with_pragma_once=with_pragma_once)

    test_subdir = os.path.join(tmpdir, "test_subdir")
    os.mkdir(test_subdir)
    test_file = __test_relative_header_create_source(tmpdir, header_file, "../test.h", is_include2_sys=is_sys, inv=inv)

    args = [format_include_path_arg("test_subdir" if relative_include_dir else test_subdir), "test.c" if source_relative else test_file]

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

@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize("is_iframework", (False, True))
@pytest.mark.parametrize("is_private", (False, True))
def test_framework_lookup(record_property, tmpdir, is_sys, is_iframework, is_private):
    # Arrange framework: <tmp>/FwRoot/MyKit.framework/(Headers|PrivateHeaders)/Component.h
    fw_root = os.path.join(tmpdir, "FwRoot")
    __test_create_framework(fw_root, "MyKit", "Component.h", private=is_private)

    test_file = __test_create_source(tmpdir, "MyKit/Component.h", is_include_sys=is_sys)

    args = [format_iframework_path_arg(fw_root) if is_iframework else format_framework_path_arg(fw_root), test_file]
    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)

    assert stderr == ""
    relative = "PrivateHeaders" if is_private else "Headers"
    assert f'#line 3 "{pathlib.PurePath(tmpdir).as_posix()}/FwRoot/MyKit.framework/{relative}/Component.h"' in stdout

@pytest.mark.parametrize("is_sys", (False, True))
@pytest.mark.parametrize(
    "order,expected",
    [
        # Note:
        # - `I1` / `ISYS1` / `F1` / `IFW1` point to distinct directories and contain `Component_1.h` (a decoy).
        # - `I` / `ISYS` / `F` / `IFW` point to directories that contain `Component.h`, which the
        #   translation unit (TU) includes via `#include "MyKit/Component.h"`.
        #
        # This makes the winning flag (-I, -isystem, -F, or -iframework) uniquely identifiable
        # in the preprocessor `#line` output.

        # Sanity checks
        (("I",), "I"),
        (("ISYS",), "ISYS"),
        (("F",), "F"),
        (("IFW",), "IFW"),

        # Includes (-I)
        (("I1", "I"), "I"),
        (("I", "I1"), "I"),
        # Includes (-I) duplicates
        (("I1", "I", "I1"), "I"),
        (("I", "I1", "I"), "I"),

        # System includes (-isystem)
        (("ISYS1", "ISYS"), "ISYS"),
        (("ISYS", "ISYS1"), "ISYS"),
        # System includes (-isystem) duplicates
        (("ISYS1", "ISYS", "ISYS1"), "ISYS"),
        (("ISYS", "ISYS1", "ISYS"), "ISYS"),

        # Framework (-F)
        (("F1", "F"), "F"),
        (("F", "F1"), "F"),
        # Framework (-F) duplicates
        (("F1", "F", "F1"), "F"),
        (("F", "F1", "F"), "F"),

        # System framework (-iframework)
        (("IFW1", "IFW"), "IFW"),
        (("IFW", "IFW1"), "IFW"),
        # System framework (-iframework) duplicates
        (("IFW1", "IFW", "IFW1"), "IFW"),
        (("IFW", "IFW1", "IFW"), "IFW"),

        # -I and -F are processed as specified (left-to-right)
        (("I", "F"), "I"),
        (("I1", "I", "F"), "I"),
        (("F", "I"), "F"),
        (("F1", "F", "I"), "F"),

        # -I and -F takes precedence over -isystem
        (("I", "ISYS"), "I"),
        (("F", "ISYS"), "F"),
        (("ISYS", "F"), "F"),
        (("ISYS", "I", "F"), "I"),
        (("ISYS", "I1", "F1", "I", "F"), "I"),
        (("ISYS", "I"), "I"),
        (("ISYS", "F", "I"), "F"),
        (("ISYS", "F1", "I1", "F", "I"), "F"),

        # -I and -F beat system framework (-iframework)
        (("I", "IFW"), "I"),
        (("F", "IFW"), "F"),
        (("IFW", "F"), "F"),
        (("IFW", "I", "F"), "I"),
        (("IFW", "I1", "F1", "I", "F"), "I"),
        (("IFW", "I"), "I"),
        (("IFW", "F", "I"), "F"),
        (("IFW", "F1", "I1", "F", "I"), "F"),

        # system include (-isystem) beats system framework (-iframework)
        (("ISYS", "IFW"), "ISYS"),
        (("IFW", "ISYS"), "ISYS"),
        (("IFW1", "ISYS1", "IFW", "ISYS"), "ISYS"),
        (("I1", "F1", "IFW1", "ISYS1", "IFW", "ISYS"), "ISYS"),
    ],
)
def test_searchpath_order(record_property, tmpdir, is_sys, order, expected):
    """
    Validate include resolution order across -I (user include),
    -isystem (system include), -F (user framework), and
    -iframework (system framework) using a minimal file layout,
    asserting which physical header path appears in the preprocessor #line output.

    The test constructs four parallel trees (two entries per kind):
    - inc{,_1}/MyKit/Component{,_1}.h                     # for -I
    - isys{,_1}/MyKit/Component{,_1}.h                    # for -isystem
    - Fw{,_1}/MyKit.framework/Headers/Component{,_1}.h    # for -F
    - SysFw{,_1}/MyKit.framework/Headers/Component{,_1}.h # for -iframework

    It then preprocesses a TU that includes "MyKit/Component.h" (or <...> when
    is_sys=True), assembles compiler args in the exact `order`, and asserts that
    only the expected path appears in #line. Distinct names (Component.h vs
    Component_1.h) ensure a unique winner per bucket.

    References:
    - https://gcc.gnu.org/onlinedocs/cpp/Invocation.html#Invocation
    - https://gcc.gnu.org/onlinedocs/gcc/Darwin-Options.html
    """

    # Create two include dirs, two user framework dirs, and two system framework dirs
    inc_dirs, isys_dirs, fw_dirs, sysfw_dirs = [], [], [], []

    def _suffix(idx: int) -> str:
        return f"_{idx}" if idx > 0 else ""

    for idx in range(2):
        # -I paths
        inc_dir = os.path.join(tmpdir, f"inc{_suffix(idx)}")
        __test_create_header(inc_dir, hdr_relpath=f"MyKit/Component{_suffix(idx)}.h")
        inc_dirs.append(inc_dir)

        # -isystem paths (system includes)
        isys_dir = os.path.join(tmpdir, f"isys{_suffix(idx)}")
        __test_create_header(isys_dir, hdr_relpath=f"MyKit/Component{_suffix(idx)}.h")
        isys_dirs.append(isys_dir)

        # -F paths (user frameworks)
        fw_dir = os.path.join(tmpdir, f"Fw{_suffix(idx)}")
        __test_create_framework(fw_dir, "MyKit", f"Component{_suffix(idx)}.h")
        fw_dirs.append(fw_dir)

        # -iframework paths (system frameworks)
        sysfw_dir = os.path.join(tmpdir, f"SysFw{_suffix(idx)}")
        __test_create_framework(sysfw_dir, "MyKit", f"Component{_suffix(idx)}.h")
        sysfw_dirs.append(sysfw_dir)

    # Translation unit under test: include MyKit/Component.h (quote or system form)
    test_file = __test_create_source(tmpdir, "MyKit/Component.h", is_include_sys=is_sys)

    def idx_from_flag(prefix: str, flag: str) -> int:
        """Extract numeric suffix from tokens like 'I1', 'ISYS1', 'F1', 'IFW1'.
        Returns 0 when no suffix is present (e.g., 'I', 'ISYS', 'F', 'IFW')."""
        return int(flag[len(prefix):]) if len(flag) > len(prefix) else 0

    # Build argv in the exact order requested by `order`
    args = []
    for flag in order:
        if flag in ["I", "I1"]:
            args.append(format_include_path_arg(inc_dirs[idx_from_flag("I", flag)]))
        elif flag in ["ISYS", "ISYS1"]:
            args.append(format_isystem_path_arg(isys_dirs[idx_from_flag("ISYS", flag)]))
        elif flag in ["F", "F1"]:
            args.append(format_framework_path_arg(fw_dirs[idx_from_flag("F", flag)]))
        elif flag in ["IFW", "IFW1"]:
            args.append(format_iframework_path_arg(sysfw_dirs[idx_from_flag("IFW", flag)]))
        else:
            raise AssertionError(f"unknown flag in order: {flag}")
    args.append(test_file)

    # Run the preprocessor and capture outputs
    _, stdout, stderr = simplecpp(args, cwd=tmpdir)
    record_property("stdout", stdout)
    record_property("stderr", stderr)

    # Resolve the absolute expected/forbidden paths we want to see in #line output
    root = pathlib.PurePath(tmpdir).as_posix()

    inc_paths = [f"{root}/inc{_suffix(idx)}/MyKit/Component{_suffix(idx)}.h" for idx in range(2)]
    isys_paths = [f"{root}/isys{_suffix(idx)}/MyKit/Component{_suffix(idx)}.h" for idx in range(2)]
    fw_paths = [f"{root}/Fw{_suffix(idx)}/MyKit.framework/Headers/Component{_suffix(idx)}.h" for idx in range(2)]
    ifw_paths = [f"{root}/SysFw{_suffix(idx)}/MyKit.framework/Headers/Component{_suffix(idx)}.h" for idx in range(2)]
    all_candidate_paths = [*inc_paths, *isys_paths, *fw_paths, *ifw_paths]

    # Compute the single path we expect to appear
    expected_path = None
    if expected in ["I", "I1"]:
        expected_path = inc_paths[idx_from_flag("I", expected)]
    elif expected in ["ISYS", "ISYS1"]:
        expected_path = isys_paths[idx_from_flag("ISYS", expected)]
    elif expected in ["F", "F1"]:
        expected_path = fw_paths[idx_from_flag("F", expected)]
    elif expected in ["IFW", "IFW1"]:
        expected_path = ifw_paths[idx_from_flag("IFW", expected)]
    assert expected_path is not None, "test configuration error: expected token not recognized"

    # Assert ONLY the expected path appears in the preprocessor #line output
    assert expected_path in stdout
    for p in (p for p in all_candidate_paths if p != expected_path):
        assert p not in stdout

    # No diagnostics expected
    assert stderr == ""

def test_pragma_once_matching(record_property, tmpdir):
    test_dir = os.path.join(tmpdir, "test_dir")
    test_subdir = os.path.join(test_dir, "test_subdir")

    test_file = os.path.join(test_dir, "test.c")
    once_header = os.path.join(test_dir, "once.h")

    if platform.system() == "Windows":
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
            f"\"{test_dir}/once.h\"",
            f"\"{test_dir}/Once.h\"",
            f"<{test_dir}/once.h>",
            f"<{test_dir}/Once.h>",
        ]
    else:
        names_to_test = [
            '"once.h"',
            '<once.h>',
            '"../test_dir/once.h"',
            '"test_subdir/../once.h"',
            f"\"{test_dir}/once.h\"",
            f"<{test_dir}/once.h>",
        ]

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
