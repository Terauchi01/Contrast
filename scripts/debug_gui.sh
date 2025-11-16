#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="$ROOT_DIR/build"

echo "[debug_gui] root=$ROOT_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f CMakeCache.txt ]; then
	echo "[debug_gui] configuring project (cmake)"
	cmake .. -DBUILD_TESTS=ON
fi

echo "[debug_gui] building gui_debug (may take a while on first run)"
cmake --build . --target gui_debug -- -j$(sysctl -n hw.ncpu)

APP="$BUILD_DIR/apps/gui_debug/gui_debug"
if [ ! -x "$APP" ]; then
	echo "[debug_gui] GUI binary not found or not executable: $APP" >&2
	exit 1
fi

echo "[debug_gui] launching: $APP"
"$APP"

