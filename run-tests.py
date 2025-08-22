
import glob
import os
import subprocess
import sys

def cleanup(out):
  ret = ''
  for s in out.decode('utf-8').split('\n'):
    if len(s) > 1 and s[0] == '#':
      continue
    s = "".join(s.split())
    ret = ret + '\n' + s
  return ret.strip()

commands = []

for f in sorted(glob.glob(os.path.expanduser('testsuite/clang-preprocessor-tests/*.c*'))):
  for line in open(f, 'rt', encoding='utf-8'):
    if line.startswith('// RUN: %clang_cc1 '):
      cmd = ''
      for arg in line[19:].split():
        if arg == '-E' or (len(arg) >= 3 and arg[:2] == '-D'):
          cmd = cmd + ' ' + arg
      if len(cmd) > 1:
        newcmd = cmd[1:] + ' ' + f
        if not newcmd in commands:
          commands.append(cmd[1:] + ' ' + f)
for f in sorted(glob.glob(os.path.expanduser('testsuite/gcc-preprocessor-tests/*.c*'))):
  commands.append('-E ' + f)

# skipping tests..
skip = ['assembler-with-cpp.c',
        'builtin_line.c',
        'c99-6_10_3_3_p4.c',
        'clang_headers.c', # missing include <limits.h>
        'comment_save.c', # _Pragma
        'has_attribute.c',
        'has_attribute.cpp',
        'header_lookup1.c', # missing include <stddef.h>
        'line-directive-output.c',
        #  'macro_paste_hashhash.c',
        'microsoft-ext.c',
        'normalize-3.c', # gcc has different output \uAC00 vs \U0000AC00 on cygwin/linux
        'pr63831-1.c', # __has_attribute => works differently on cygwin/linux
        'pr63831-2.c', # __has_attribute => works differently on cygwin/linux
        'pr65238-1.c', # __has_attribute => works differently on cygwin/linux
        '_Pragma-location.c',
        '_Pragma-dependency.c',
        '_Pragma-dependency2.c',
        '_Pragma-physloc.c',
        'pragma-pushpop-macro.c', # pragma push/pop
        'x86_target_features.c',
        'warn-disabled-macro-expansion.c',
        'ucnid-2011-1.c' # \u00A8 generates different output on cygwin/linux
       ]

todo = [
         # todo, low priority: wrong number of macro arguments, pragma, etc
         'macro_backslash.c',
         'macro_fn_comma_swallow.c',
         'macro_fn_comma_swallow2.c',
         'macro_expand.c',
         'macro_fn_disable_expand.c',
         'macro_paste_commaext.c',
         'macro_paste_hard.c',
         'macro_rescan_varargs.c',

         # todo, high priority
         'c99-6_10_3_4_p5.c',
         'c99-6_10_3_4_p6.c',
         'expr_usual_conversions.c', # condition is true: 4U - 30 >= 0
         'stdint.c',

         # GCC..
         'diagnostic-pragma-1.c',
         'pr45457.c',
         'pr57580.c',
         ]

numberOfSkipped = 0
numberOfFailed = 0
numberOfFixed = 0

usedTodos = []

for cmd in commands:
  if cmd[cmd.rfind('/')+1:] in skip:
    numberOfSkipped = numberOfSkipped + 1
    continue

  clang_cmd = ['clang']
  clang_cmd.extend(cmd.split(' '))
  p = subprocess.Popen(clang_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  clang_ec = p.returncode
  clang_output = cleanup(comm[0])

  gcc_cmd = ['gcc']
  gcc_cmd.extend(cmd.split(' '))
  p = subprocess.Popen(gcc_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  gcc_ec = p.returncode
  gcc_output = cleanup(comm[0])

  simplecpp_cmd = ['./simplecpp']
  # -E is not supported and we bail out on unknown options
  simplecpp_cmd.extend(cmd.replace('-E ', '', 1).split(' '))
  p = subprocess.Popen(simplecpp_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  comm = p.communicate()
  simplecpp_ec = p.returncode
  simplecpp_output = cleanup(comm[0])
  simplecpp_err = comm[0].decode('utf-8').strip()

  clang_fail = simplecpp_output != clang_output
  gcc_fail = simplecpp_output != gcc_output
  if clang_fail and gcc_fail:
    filename = cmd[cmd.rfind('/')+1:]
    if filename in todo:
      print('TODO ' + cmd)
      usedTodos.append(filename)
    else:
      if clang_fail:
        print('FAILED (clang: {}) {}'.format(clang_ec, cmd))
        print('expected:')
        print(clang_output)
      if gcc_fail:
        print('FAILED (gcc: {}) {}'.format(gcc_ec, cmd))
        print('expected:')
        print(gcc_output)
      print('actual:')
      print(simplecpp_output)
      if simplecpp_ec:
          print('simplecpp failed - ' + simplecpp_err)
      numberOfFailed = numberOfFailed + 1

for filename in todo:
    if not filename in usedTodos:
        print('FIXED ' + filename)
        numberOfFixed = numberOfFixed + 1

print('Number of tests: ' + str(len(commands)))
print('Number of skipped: ' + str(numberOfSkipped))
print('Number of todos (fixed): ' + str(len(usedTodos)) + ' (' + str(numberOfFixed) + ')')
print('Number of failed: ' + str(numberOfFailed))

if numberOfFailed or numberOfFixed:
    sys.exit(1)

sys.exit(0)
