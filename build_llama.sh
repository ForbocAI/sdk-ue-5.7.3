#!/bin/bash
# =============================================================================
# build_llama.sh — Build libllama.a and copy to ThirdParty (Mac)
#
# Usage: ./build_llama.sh [tag]
#   tag: llama.cpp git tag (default: b8191, same as vendor_thirdparty.sh)
#
# Prerequisites: cmake, git
# Output: ThirdParty/llama.cpp/lib/Mac/libllama.a
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
THIRDPARTY_LIB_MAC="$SCRIPT_DIR/ThirdParty/llama.cpp/lib/Mac"
TAG="${1:-b8191}"
BUILD_DIR="$(mktemp -d)"
trap 'rm -rf "$BUILD_DIR"' EXIT

echo "[build_llama] Cloning llama.cpp ($TAG)..."
git clone --depth 1 --branch "$TAG" https://github.com/ggml-org/llama.cpp "$BUILD_DIR/llama.cpp"

echo "[build_llama] Configuring..."
cd "$BUILD_DIR/llama.cpp"
cmake -B build -DBUILD_SHARED_LIBS=OFF -DGGML_METAL=ON

echo "[build_llama] Building (this may take a few minutes)..."
cmake --build build --target llama -j

mkdir -p "$THIRDPARTY_LIB_MAC"
cp build/src/libllama.a "$THIRDPARTY_LIB_MAC/"

echo "[build_llama] Done. libllama.a copied to $THIRDPARTY_LIB_MAC/"
echo "[build_llama] Rebuild the UE plugin to pick up WITH_FORBOC_NATIVE=1"
