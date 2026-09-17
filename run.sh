#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
TARGET="renderer"

cd "$(dirname "$0")"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_TESTING=OFF -DRENDERER_BUILD_APP=ON

cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --target "$TARGET" --parallel "${JOBS:-$(nproc 2>/dev/null || echo 4)}"

BIN="$BUILD_DIR/$TARGET"
if [[ ! -x "$BIN" ]]; then
  if [[ -x "$BUILD_DIR/$BUILD_TYPE/$TARGET" ]]; then
    BIN="$BUILD_DIR/$BUILD_TYPE/$TARGET"
  else
    echo "Could not find '$TARGET' in '$BUILD_DIR/'. Check the build output." >&2
    exit 1
  fi
fi

exec "$BIN" "$@"
