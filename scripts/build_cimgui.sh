#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/third_party/cimgui"
SOURCE_DIR="${ROOT_DIR}/third_party/cimgui"

cmake -S "${SOURCE_DIR}" \
      -B "${BUILD_DIR}" \
      -DIMGUI_STATIC=ON \
      -DIMGUI_FREETYPE=OFF \
      -DCMAKE_BUILD_TYPE=Release

cmake --build "${BUILD_DIR}" --config Release
