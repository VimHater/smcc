#!/bin/bash
COMPILER=build/compiler
TEST_DIR=test/

mkdir -p "$TEST_DIR/build"

ANSI_RED="\033[31m"
ANSI_DIM="\033[0m"
printf "${ANSI_RED}%-20s %-20s %s\n"  "Name"   "Output"   "Ret"
printf "${ANSI_DIM}"
for src in "$TEST_DIR"/*.c; do
    name=$(basename "$src" .c)
    "$COMPILER" "$src" -o "$TEST_DIR/build/$name" 2>/dev/null
    output=$(qemu-mips-static "$TEST_DIR/build/$name" 2>/dev/null)
    rc=$?
    printf "%-20s %-20s %d\n" "$name" "$output" "$rc"
done
