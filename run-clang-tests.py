
import glob
import os
import subprocess

def cleanup(out):
  ret = ''
  for s in out.split('\n'):
    s = "".join(s.split())
    if len(s) == 0 or s[0] == '#':
      continue
    ret = ret + s + '\n'
  return ret

commands = []
for f in sorted(glob.glob(os.path.expanduser('~/llvm/tools/clang/test/Preprocessor/*.c*'))):
  filename = f[f.rfind('/')+1:]

  # skipping tests..
  skip = ['assembler-with-cpp.c',
          'builtin_line.c',
          'has_attribute.c',
          'line-directive-output.c',
          'microsoft-ext.c',
          '_Pragma-location.c',
          '_Pragma-dependency.c',
          '_Pragma-dependency2.c',
          '_Pragma-physloc.c',
          'pragma-pushpop-macro.c', # pragma push/pop
          'x86_target_features.c',
          'warn-disabled-macro-expansion.c']

  if filename in skip:
    continue

  todo = [
           # todo, low priority: wrong number of macro arguments, pragma, etc
           'header_lookup1.c',
           'macro_backslash.c',
           'macro_fn_comma_swallow.c',
           'macro_fn_comma_swallow2.c',
           'macro_fn_lparen_scan.c',
           'macro_fn_lparen_scan2.c',
           'macro_disable.c',
           'macro_expand.c',
           'macro_expand_empty.c',
           'macro_fn_disable_expand.c',
           'macro_paste_commaext.c',
           'macro_paste_empty.c',
           'macro_paste_hard.c',
           'macro_paste_hashhash.c',
           'macro_paste_identifier_error.c',
           'macro_paste_simple.c',
           'macro_paste_spacing.c',
           'macro_rescan.c',
           'macro_rescan_varargs.c',
           'macro_rescan2.c',
           'macro_space.c',

  # todo, high priority
           'c99-6_10_3_3_p4.c',
           'c99-6_10_3_4_p5.c',
           'c99-6_10_3_4_p6.c',
           'comment_save.c',
           'cxx_and.cpp', # if A and B
           'cxx_bitand.cpp',
           'cxx_bitor.cpp', # if A bitor B
           'cxx_compl.cpp',  # if A compl B
           'cxx_not.cpp',
           'cxx_not_eq.cpp',  # if A not_eq B
           'cxx_or.cpp',  # if A or B
           'cxx_xor.cpp',  # if A xor B
           'cxx_oper_keyword_ms_compat.cpp',
           'expr_usual_conversions.c', # condition is true: 4U - 30 >= 0
           'hash_line.c',
           'macro_fn_varargs_named.c',  # named vararg arguments
           'mi_opt2.c',  # stringify
           'print_line_include.c',  #stringify
           'stdint.c',
           'stringize_misc.c']

  if filename in todo:
    continue

  for line in open(f, 'rt'):
    if line.startswith('// RUN: %clang_cc1 '):
      cmd = ''
      for arg in line[19:].split():
        if arg == '-E' or (len(arg) >= 3 and arg[:2] == '-D'):
          cmd = cmd + ' ' + arg
      if len(cmd) > 1:
        commands.append(cmd[1:] + ' ' + f)

for cmd in set(commands):
  clang_cmd = ['clang']
  clang_cmd.extend(cmd.split(' '))
  p = subprocess.Popen(clang_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  clang_output = comm[0]

  cppcheck_cmd = [os.path.expanduser('~/cppcheck/cppcheck'), '-q']
  cppcheck_cmd.extend(cmd.split(' '))
  p = subprocess.Popen(cppcheck_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  cppcheck_output = comm[0]

  if cleanup(clang_output) != cleanup(cppcheck_output):
    print(cmd)
    print(cmd[cmd.rfind('/')+1:])
    #print('clang_output:\n' + cleanup(clang_output))
    #print('cppcheck_output:\n' + cleanup(cppcheck_output))
    #break

