#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${1:-debug}"
PRESET="user-${BUILD_TYPE}"
BUILD_DIR="${ROOT_DIR}/cmake-build-${BUILD_TYPE}"

SDK_PATH="$(xcrun --show-sdk-path)"
export SDKROOT="${SDK_PATH}"

# check if preset exists. if not, run generate_user_presets.sh
if [ ! -f "${ROOT_DIR}/CMakeUserPresets.json" ]; then
  "$(dirname "${BASH_SOURCE[0]}")/generate_user_presets.sh"
fi

cmake --preset "${PRESET}" \
  -DCMAKE_OSX_SYSROOT="${SDK_PATH}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "Configured ${BUILD_TYPE} Ninja build in: ${BUILD_DIR}"
echo "SDK: ${SDK_PATH}"
echo "Preset: ${PRESET}"