
import glob
import os
import subprocess

def cleanup(out):
  ret = ''
  for s in out.split('\n'):
    if len(s) > 1 and s[0] == '#':
      continue
    s = "".join(s.split())
    ret = ret + s
  return ret

commands = []
for f in sorted(glob.glob(os.path.expanduser('testsuite/*/*.c*'))):

  for line in open(f, 'rt'):
    if line.startswith('// RUN: %clang_cc1 '):
      cmd = ''
      for arg in line[19:].split():
        if arg == '-E' or (len(arg) >= 3 and arg[:2] == '-D'):
          cmd = cmd + ' ' + arg
      if len(cmd) > 1:
        commands.append(cmd[1:] + ' ' + f)

# skipping tests..
skip = ['assembler-with-cpp.c',
        'builtin_line.c',
        'comment_save.c', # _Pragma
        'has_attribute.c',
        'header_lookup1.c', # missing include <stddef.h>
        'line-directive-output.c',
        'microsoft-ext.c',
        '_Pragma-location.c',
        '_Pragma-dependency.c',
        '_Pragma-dependency2.c',
        '_Pragma-physloc.c',
        'pragma-pushpop-macro.c', # pragma push/pop
        'x86_target_features.c',
        'warn-disabled-macro-expansion.c']

todo = [
         # todo, low priority: wrong number of macro arguments, pragma, etc
         'macro_backslash.c',
         'macro_fn_comma_swallow.c',
         'macro_fn_comma_swallow2.c',
         'macro_fn_lparen_scan.c',
         'macro_expand.c',
         'macro_fn_disable_expand.c',
         'macro_paste_commaext.c',
         'macro_paste_hard.c',
         'macro_paste_hashhash.c',
         'macro_rescan_varargs.c',

         # todo, high priority
         'c99-6_10_3_3_p4.c',
         'c99-6_10_3_4_p5.c',
         'c99-6_10_3_4_p6.c',
         'cxx_compl.cpp',  # if A compl B
         'cxx_not.cpp',
         'cxx_not_eq.cpp',  # if A not_eq B
         'cxx_oper_keyword_ms_compat.cpp',
         'expr_usual_conversions.c', # condition is true: 4U - 30 >= 0
         'stdint.c',
         'stringize_misc.c']

numberOfSkipped = 0
numberOfFailed = 0

usedTodos = []

for cmd in set(commands):
  if cmd[cmd.rfind('/')+1:] in skip:
    numberOfSkipped = numberOfSkipped + 1
    continue

  clang_cmd = ['clang']
  clang_cmd.extend(cmd.split(' '))
  p = subprocess.Popen(clang_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  clang_output = cleanup(comm[0])

  gcc_cmd = ['gcc']
  gcc_cmd.extend(cmd.split(' '))
  p = subprocess.Popen(gcc_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  gcc_output = cleanup(comm[0])

  simplecpp_cmd = ['./simplecpp']
  simplecpp_cmd.extend(cmd.split(' '))
  p = subprocess.Popen(simplecpp_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  simplecpp_output = cleanup(comm[0])

  if simplecpp_output != clang_output and simplecpp_output != gcc_output:
    filename = cmd[cmd.rfind('/')+1:]
    if filename in todo:
      print('TODO ' + cmd)
      usedTodos.append(filename)
    else:
      print('FAILED ' + cmd)
      numberOfFailed = numberOfFailed + 1

for filename in todo:
    if not filename in usedTodos:
        print('FIXED ' + filename)

print('Number of tests: ' + str(len(commands)))
print('Number of skipped: ' + str(numberOfSkipped))
print('Number of todos: ' + str(len(usedTodos)))
print('Number of failed: ' + str(numberOfFailed))

