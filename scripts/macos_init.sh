#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${1:-debug}"
BUNDLE_APP="${2:-OFF}"
PRESET="user-${BUILD_TYPE}"
BUILD_DIR="${ROOT_DIR}/cmake-build-${BUILD_TYPE}"

SDK_PATH="$(xcrun --show-sdk-path)"
export SDKROOT="${SDK_PATH}"

# check if preset exists. if not, run generate_user_presets.sh
if [ ! -f "${ROOT_DIR}/CMakeUserPresets.json" ]; then
  "$(dirname "${BASH_SOURCE[0]}")/generate_user_presets.sh"
fi
# if BUNDLE_APP is ON, override output directory to cmake-build-${BUILD_TYPE}-bundle
if [ "${BUNDLE_APP}" == "ON" ]; then
  BUILD_DIR="${ROOT_DIR}/cmake-build-${BUILD_TYPE}-bundle"
fi
cmake --preset "${PRESET}" \
  -B "${BUILD_DIR}" \
  -DCMAKE_OSX_SYSROOT="${SDK_PATH}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_MACOS_BUNDLE="${BUNDLE_APP}" \

echo "Configured ${BUILD_TYPE} Ninja build in: ${BUILD_DIR}"
echo "SDK: ${SDK_PATH}"
echo "Preset: ${PRESET}"