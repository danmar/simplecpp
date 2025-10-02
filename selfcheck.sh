#!/bin/bash

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
if [ "$cxx_type" = "Ubuntu" ] || [ "$cxx_type" = "Debian" ]; then
  cxx_type=$($CXX --version | head -1 | cut -d' ' -f2)
fi

# TODO: generate defines from compiler
if [ "$cxx_type" = "g++" ]; then
  defs=
  defs="$defs -D__GNUC__"
  defs="$defs -D__STDC__"
  defs="$defs -D__x86_64__"
  defs="$defs -D__STDC_HOSTED__"
  defs="$defs -D__CHAR_BIT__=8"
  defs="$defs -D__has_builtin(x)=(1)"
  defs="$defs -D__has_cpp_attribute(x)=(1)"
  defs="$defs -D__has_attribute(x)=(1)"

  inc=
  while read line
  do
    inc="$inc -I$line"
  done <<< "$($CXX -x c++ -v -c -S - 2>&1 < /dev/null | grep -e'^ [/A-Z]' | grep -v /cc1plus)"
elif [ "$cxx_type" = "clang" ]; then
  # libstdc++
  defs=
  defs="$defs -D__x86_64__"
  defs="$defs -D__STDC_HOSTED__"
  defs="$defs -D__CHAR_BIT__=8"
  defs="$defs -D__has_builtin(x)=(1)"
  defs="$defs -D__has_cpp_attribute(x)=(1)"
  defs="$defs -D__has_feature(x)=(1)"
  defs="$defs -D__has_include_next(x)=(0)"
  defs="$defs -D__has_attribute(x)=(0)"
  defs="$defs -D__building_module(x)=(0)"

  inc=
  while read line
  do
    inc="$inc -I$line"
  done <<< "$($CXX -x c++ -v -c -S - 2>&1 < /dev/null | grep -e'^ [/A-Z]')"

  # TODO: enable
  # libc++
  #defs=
  #defs="$defs -D__x86_64__"
  #defs="$defs -D__linux__"
  #defs="$defs -D__SIZEOF_SIZE_T__=8"
  #defs="$defs -D__has_include_next(x)=(0)"
  #defs="$defs -D__has_builtin(x)=(1)"
  #defs="$defs -D__has_feature(x)=(1)"

  #inc=
  #while read line
  #do
  #  inc="$inc -I$line"
  #done <<< "$($CXX -x c++ -stdlib=libc++ -v -c -S - 2>&1 < /dev/null | grep -e'^ [/A-Z]')"
elif [ "$cxx_type" = "Apple" ]; then
  defs=
  defs="$defs -D__BYTE_ORDER__"
  defs="$defs -D__APPLE__"
  defs="$defs -D__GNUC__=15"
  defs="$defs -D__x86_64__"
  defs="$defs -D__SIZEOF_SIZE_T__=8"
  defs="$defs -D__LITTLE_ENDIAN__"
  defs="$defs -D__has_feature(x)=(0)"
  defs="$defs -D__has_extension(x)=(1)"
  defs="$defs -D__has_attribute(x)=(0)"
  defs="$defs -D__has_cpp_attribute(x)=(0)"
  defs="$defs -D__has_include_next(x)=(0)"
  defs="$defs -D__has_builtin(x)=(1)"
  defs="$defs -D__is_target_os(x)=(0)"
  defs="$defs -D__is_target_arch(x)=(0)"
  defs="$defs -D__is_target_vendor(x)=(0)"
  defs="$defs -D__is_target_environment(x)=(0)"
  defs="$defs -D__is_target_variant_os(x)=(0)"
  defs="$defs -D__is_target_variant_environment(x)=(0)"

  inc=
  while read line
  do
    inc="$inc -I$line"
    # TODO: pass the framework path as such when possible
  done <<< "$($CXX -x c++ -v -c -S - 2>&1 < /dev/null | grep -e'^ [/A-Z]' | sed 's/ (framework directory)//g')"
  echo $inc
else
  echo "unknown compiler '$cxx_type'"
  exit 1
fi

# run with -std=gnuc++* so __has_include(...) is available
./simplecpp simplecpp.cpp -e -f -std=gnu++11 $defs $inc
ec=$?
if [ $ec -ne 0 ]; then
  exit $ec
fi
