#!/bin/bash
# =============================================================================
# vendor_thirdparty.sh — Downloads and places ThirdParty dependencies
#
# Usage:
#   ./vendor_thirdparty.sh [--sqlite-only | --llama-only | --all]
#
# This script downloads:
#   1. sqlite3 amalgamation + sqlite-vec → ThirdParty/sqlite-vss/
#   2. llama.cpp headers → ThirdParty/llama.cpp/include/
#      (libs must be built separately — see below)
#
# After running, Build.cs will auto-detect and enable:
#   WITH_FORBOC_SQLITE_VEC=1  (when sqlite3.h + sqlite3.c + vec0.c present)
#   WITH_FORBOC_NATIVE=1      (when llama.h + platform lib present)
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
THIRDPARTY_DIR="$SCRIPT_DIR/ThirdParty"
TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

# Versions
SQLITE_YEAR="2024"
SQLITE_VERSION="3460100"
SQLITE_URL="https://www.sqlite.org/${SQLITE_YEAR}/sqlite-amalgamation-${SQLITE_VERSION}.zip"
SQLITE_VEC_VERSION="v0.1.6"
SQLITE_VEC_URL="https://github.com/asg017/sqlite-vec/archive/refs/tags/${SQLITE_VEC_VERSION}.tar.gz"
LLAMA_CPP_TAG="b8191"
LLAMA_CPP_URL="https://github.com/ggml-org/llama.cpp/archive/refs/tags/${LLAMA_CPP_TAG}.tar.gz"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()  { echo -e "${GREEN}[vendor]${NC} $*"; }
warn() { echo -e "${YELLOW}[vendor]${NC} $*"; }
err()  { echo -e "${RED}[vendor]${NC} $*" >&2; }

# -------------------------------------------------------------------------
# sqlite3 amalgamation + sqlite-vec
# -------------------------------------------------------------------------
vendor_sqlite() {
  local SQLITE_INCLUDE="$THIRDPARTY_DIR/sqlite-vss/include"
  local SQLITE_SRC="$THIRDPARTY_DIR/sqlite-vss/src"

  mkdir -p "$SQLITE_INCLUDE" "$SQLITE_SRC"

  # --- sqlite3 amalgamation ---
  if [ -f "$SQLITE_INCLUDE/sqlite3.h" ] && [ -f "$SQLITE_SRC/sqlite3.c" ]; then
    log "sqlite3 amalgamation already present, skipping"
  else
    log "Downloading sqlite3 amalgamation (${SQLITE_VERSION})..."
    curl -fsSL "$SQLITE_URL" -o "$TEMP_DIR/sqlite.zip"
    unzip -qo "$TEMP_DIR/sqlite.zip" -d "$TEMP_DIR/sqlite"

    local SQLITE_EXTRACT_DIR
    SQLITE_EXTRACT_DIR="$(find "$TEMP_DIR/sqlite" -maxdepth 1 -type d -name 'sqlite-amalgamation-*' | head -1)"
    if [ -z "$SQLITE_EXTRACT_DIR" ]; then
      err "Failed to find sqlite amalgamation in archive"
      return 1
    fi

    cp "$SQLITE_EXTRACT_DIR/sqlite3.h" "$SQLITE_INCLUDE/sqlite3.h"
    cp "$SQLITE_EXTRACT_DIR/sqlite3ext.h" "$SQLITE_INCLUDE/sqlite3ext.h"
    cp "$SQLITE_EXTRACT_DIR/sqlite3.c" "$SQLITE_SRC/sqlite3.c"
    log "sqlite3 amalgamation installed"
  fi

  # --- sqlite-vec ---
  if [ -f "$SQLITE_SRC/vec0.c" ]; then
    log "sqlite-vec already present, skipping"
  else
    log "Downloading sqlite-vec (${SQLITE_VEC_VERSION})..."
    curl -fsSL "$SQLITE_VEC_URL" -o "$TEMP_DIR/sqlite-vec.tar.gz"
    tar xzf "$TEMP_DIR/sqlite-vec.tar.gz" -C "$TEMP_DIR"

    local VEC_EXTRACT_DIR
    VEC_EXTRACT_DIR="$(find "$TEMP_DIR" -maxdepth 1 -type d -name 'sqlite-vec-*' | head -1)"
    if [ -z "$VEC_EXTRACT_DIR" ]; then
      err "Failed to find sqlite-vec in archive"
      return 1
    fi

    # sqlite-vec ships a single-file amalgamation
    if [ -f "$VEC_EXTRACT_DIR/sqlite-vec.c" ]; then
      cp "$VEC_EXTRACT_DIR/sqlite-vec.c" "$SQLITE_SRC/vec0.c"
    elif [ -f "$VEC_EXTRACT_DIR/vec0.c" ]; then
      cp "$VEC_EXTRACT_DIR/vec0.c" "$SQLITE_SRC/vec0.c"
    else
      # Build from source directory
      warn "sqlite-vec amalgamation not found, checking src/"
      if [ -f "$VEC_EXTRACT_DIR/src/sqlite-vec.c" ]; then
        cp "$VEC_EXTRACT_DIR/src/sqlite-vec.c" "$SQLITE_SRC/vec0.c"
      else
        err "Could not locate vec0.c or sqlite-vec.c in archive"
        return 1
      fi
    fi

    # Copy header if present
    if [ -f "$VEC_EXTRACT_DIR/sqlite-vec.h" ]; then
      cp "$VEC_EXTRACT_DIR/sqlite-vec.h" "$SQLITE_INCLUDE/sqlite-vec.h"
    fi

    log "sqlite-vec installed"
  fi

  log "sqlite-vss vendor complete"
  log "  Include: $SQLITE_INCLUDE"
  log "  Source:  $SQLITE_SRC"
  log "  Build.cs will set WITH_FORBOC_SQLITE_VEC=1"
}

# -------------------------------------------------------------------------
# llama.cpp headers (libs require separate build)
# -------------------------------------------------------------------------
vendor_llama_headers() {
  local LLAMA_INCLUDE="$THIRDPARTY_DIR/llama.cpp/include"

  mkdir -p "$LLAMA_INCLUDE"

  if [ -f "$LLAMA_INCLUDE/llama.h" ]; then
    log "llama.cpp headers already present, skipping"
    return 0
  fi

  log "Downloading llama.cpp headers (${LLAMA_CPP_TAG})..."
  curl -fsSL "$LLAMA_CPP_URL" -o "$TEMP_DIR/llama.tar.gz"
  tar xzf "$TEMP_DIR/llama.tar.gz" -C "$TEMP_DIR"

  local LLAMA_EXTRACT_DIR
  LLAMA_EXTRACT_DIR="$(find "$TEMP_DIR" -maxdepth 1 -type d -name 'llama.cpp-*' | head -1)"
  if [ -z "$LLAMA_EXTRACT_DIR" ]; then
    err "Failed to find llama.cpp in archive"
    return 1
  fi

  # Copy public headers
  cp "$LLAMA_EXTRACT_DIR/include/llama.h" "$LLAMA_INCLUDE/llama.h" 2>/dev/null || true
  cp "$LLAMA_EXTRACT_DIR/ggml/include/ggml.h" "$LLAMA_INCLUDE/ggml.h" 2>/dev/null || true
  cp "$LLAMA_EXTRACT_DIR/ggml/include/ggml-alloc.h" "$LLAMA_INCLUDE/ggml-alloc.h" 2>/dev/null || true
  cp "$LLAMA_EXTRACT_DIR/ggml/include/ggml-backend.h" "$LLAMA_INCLUDE/ggml-backend.h" 2>/dev/null || true

  if [ ! -f "$LLAMA_INCLUDE/llama.h" ]; then
    err "llama.h not found in expected location. Archive structure may have changed."
    warn "Try manually copying from: $LLAMA_EXTRACT_DIR"
    return 1
  fi

  # Create lib dirs so Build.cs can find libs once built
  mkdir -p "$THIRDPARTY_DIR/llama.cpp/lib/Mac" "$THIRDPARTY_DIR/llama.cpp/lib/Win64"

  log "llama.cpp headers installed"
  log "  Include: $LLAMA_INCLUDE"
  log "  Lib dirs: lib/Mac/, lib/Win64/ (place libllama.a / llama.lib)"
  echo ""
  warn "IMPORTANT: You still need to build libllama.a / llama.lib"
  warn "  Mac:   cmake -B build -DBUILD_SHARED_LIBS=OFF && cmake --build build --target llama"
  warn "         cp build/src/libllama.a $THIRDPARTY_DIR/llama.cpp/lib/Mac/"
  warn "  Win64: cmake -B build -DBUILD_SHARED_LIBS=OFF -G 'Visual Studio 17 2022' && cmake --build build --config Release --target llama"
  warn "         cp build\\src\\Release\\llama.lib $THIRDPARTY_DIR/llama.cpp/lib/Win64/"
  warn ""
  warn "After placing libs, Build.cs will set WITH_FORBOC_NATIVE=1"
}

# -------------------------------------------------------------------------
# Main
# -------------------------------------------------------------------------
MODE="${1:---all}"

case "$MODE" in
  --sqlite-only)
    vendor_sqlite
    ;;
  --llama-only)
    vendor_llama_headers
    ;;
  --all)
    vendor_sqlite
    echo ""
    vendor_llama_headers
    ;;
  *)
    echo "Usage: $0 [--sqlite-only | --llama-only | --all]"
    exit 1
    ;;
esac

echo ""
log "Done. Rebuild the plugin to pick up changes."
