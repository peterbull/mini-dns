#!/bin/bash
set -xe

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mkdir -p "$DIR/build"
mkdir -p "$DIR/../preprocessed"
clang -E -P "$DIR/src/main.c" -o "$DIR/../preprocessed/main.preprocessed.c"
clang -Wall -Wextra -o "$DIR/build/main" "$DIR/src/main.c"
