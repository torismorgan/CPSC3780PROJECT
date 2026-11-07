#!/bin/sh
set -eu

LAB_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "$LAB_DIR/.." && pwd)
TARGET="$LAB_DIR/shared/fielddrop"

mkdir -p "$TARGET/source"
cp "$PROJECT_DIR/Makefile" "$TARGET/Makefile"
cp "$PROJECT_DIR"/source/* "$TARGET/source/"
cp "$PROJECT_DIR/tests/kathara_8k.bin" "$TARGET/input.bin"

printf 'Copied current source, Makefile, and 8 KiB input to %s\n' "$TARGET"
