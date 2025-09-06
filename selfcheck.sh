#!/bin/sh

set -x

output=$(./simplecpp simplecpp.cpp -e -f 2>&1)
ec=$?
errors=$(echo "$output" | grep -v 'Header not found: <')
if [ $ec -ne 0 ]; then
  # only fail if got errors which do not refer to missing system includes
  if [ ! -z "$errors" ]; then
    exit $ec
  fi
fi

if [ -n "${STRICT}" ] && [ "${STRICT}" -eq 1 ]; then
  # macos-13 does not support --tmpdir
  # macos-* does not support --suffix
  tmpfile=$(mktemp -u /tmp/selfcheck.XXXXXXXXXX.exp)
  ec=$?
  if [ $ec -ne 0 ]; then
    exit $ec
  fi
  ./simplecpp simplecpp.cpp > "$tmpfile" 2> /dev/null
  diff -u selfcheck.exp "$tmpfile"
  ec=$?
  rm -f "$tmpfile"
  if [ $ec -ne 0 ]; then
    exit $ec
  fi
fi