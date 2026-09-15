#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
TARGET="renderer"

cd "$(dirname "$0")"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

cmake --build "$BUILD_DIR" --target "$TARGET" -j"$(nproc 2>/dev/null || echo 4)"

BIN="$BUILD_DIR/$TARGET"
if [[ ! -x "$BIN" ]]; then
  if [[ -x "$BUILD_DIR/$BUILD_TYPE/$TARGET" ]]; then
    BIN="$BUILD_DIR/$BUILD_TYPE/$TARGET"
  else
    echo "Не найден бинарник '$TARGET' в '$BUILD_DIR/'. Проверь CMakeLists.txt." >&2
    exit 1
  fi
fi

"$BIN" "$@"