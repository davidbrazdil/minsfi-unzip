#!/bin/bash

source 'config.inc'

set -eu

if [ "$#" != 2 ]; then
  echo Usage: $0 input.o output_prog
  exit 1
fi

input="$1"
output="$2"

# We run -globaldce to remove some dead code (-std-link-opts doesn't
# seem to work for that).
# Of the sandboxing passes, "-sandbox-indirect-calls" must come last.
# "-strip-debug" is currently required by "-sandbox-indirect-calls".
pnacl-opt \
    -strip-tls \
    -loweratomic \
    -pnacl-abi-simplify-preopt \
    -globaldce \
    -pnacl-abi-simplify-postopt \
    -strip-debug \
    -expand-allocas -allocate-data-segment \
    -sandbox-memory-accesses -sandbox-indirect-calls \
    $input -o $output.bc
pnacl-llc -mtriple=x86_64-linux-gnu -relocation-model=pic -filetype=obj \
    $output.bc -o $output.o
objcopy --redefine-sym _start=sandbox_entry $output.o $output.o

gcc -g -m64 -Wall -Werror trusted/runtime.c trusted/system_io.c $output.o -o $output
