#!/bin/sh

output=$(./simplecpp simplecpp.cpp -e -f 2>&1)
ec=$?
errors=$(echo "$output" | grep -v 'Header not found: <')
if [ $ec -ne 0 ]; then
  # only fail if we got errors which do not refer to missing system includes
  if [ ! -z "$errors" ]; then
    exit $ec
  fi
fi

if [ -z "$CXX" ]; then
  exit 0
fi

cxx_type=$($CXX --version | head -1 | cut -d' ' -f1)
if [ "$cxx_type" = "Ubuntu" ]; then
  cxx_type=$($CXX --version | head -1 | cut -d' ' -f2)
fi
if [ "$cxx_type" = "g++" ]; then
  gcc_ver=$($CXX -dumpversion)
  ./simplecpp simplecpp.cpp -e -f -std=gnu++11 -D__GNUC__ -D__STDC__ -D__STDC_HOSTED__ -D__CHAR_BIT__=8 -I"/usr/include" -I"/usr/include/linux" -I"/usr/include/c++/$gcc_ver" -I"/usr/include/x86_64-linux-gnu/c++/$gcc_ver"
  ec=$?
  if [ $ec -ne 0 ]; then
    exit $ec
  fi
elif [ "$cxx_type" = "clang" ]; then
  clang_ver=$($CXX -dumpversion)
  clang_ver=${clang_ver%%.*}
  find /usr/include -name stubs-32.h
  cxx_inc="/usr/include/c++/v1"
  if [ ! -d "$cxx_inc" ]; then
    cxx_inc="/usr/lib/llvm-$clang_ver/include/c++/v1"
  fi
  ./simplecpp simplecpp.cpp -e -f -std=gnu++11 -D__BYTE_ORDER__ -D__linux__ -I"$cxx_inc" -I"/usr/include" -I"/usr/include/x86_64-linux-gnu"
  ec=$?
  if [ $ec -ne 0 ]; then
    exit $ec
  fi
elif [ "$cxx_type" = "Apple" ]; then
  find /Applications -name endian.h
  xcode_path="/Applications/Xcode_16.4.app"
  if [ ! -d "$xcode_path" ]; then
    xcode_path="/Applications/Xcode_15.2.app"
  fi
  ./simplecpp simplecpp.cpp -e -f -std=gnu++11 -D__BYTE_ORDER__ -D__APPLE__ -I"$xcode_path/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include" -I"$xcode_path/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1"
  ec=$?
  if [ $ec -ne 0 ]; then
    exit $ec
  fi
else
  echo "unknown compiler '$cxx_type'"
  exit 1
fi
