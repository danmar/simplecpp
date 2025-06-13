#!/bin/sh

output=$(./simplecpp simplecpp.cpp -e -f 2>&1)
ec=$?
errors=$(echo "$output" | grep -v 'Header not found: <')
if [ $ec -ne 0 ]; then
  # only fail if got errors which do not refer to missing system includes
  if [ ! -z "$errors" ]; then
    exit $ec
  fi
fi