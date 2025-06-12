#!/bin/sh

output=$(./simplecpp simplecpp.cpp -e 2>&1)
ec=$?
echo "$output" | grep -v 'Header not found: <'
exit $ec

gcc_ver=$(gcc -dumpversion)
./simplecpp simplecpp.cpp -e -f -D__GNUC__ -D__STDC__ -D__STDC_HOSTED__ -D__CHAR_BIT__=8 -I"/usr/include" -I"/usr/include/linux" -I"/usr/include/c++/$gcc_ver" -I"/usr/include/c++/$gcc_ver/x86_64-pc-linux-gnu" -I"/usr/lib64/gcc/x86_64-pc-linux-gnu/$gcc_ver/include"

if [ -d "/usr/include/c++/v1" ]; then
  #clang_ver=$(clang -dumpversion)
  #clang_ver=${clang_ver%%.*}
  ./simplecpp simplecpp.cpp -e -f -D__BYTE_ORDER__ -I"/usr/include/c++/v1"
fi