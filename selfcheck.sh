#!/bin/sh

output=$(./simplecpp simplecpp.cpp -e 2>&1)
ec=$?
echo "$output" | grep -v 'Header not found: <'
exit $ec