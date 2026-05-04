#!/usr/bin/env bash
# Auto-format all project source files using clang-format (WebKit style).
# Usage: ./scripts/format.sh [--check]
#   --check  Dry-run mode: exit 1 if any file would be reformatted (for CI).

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/src"

CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"

if ! command -v "$CLANG_FORMAT" &>/dev/null; then
    echo "error: clang-format not found. Install it or set CLANG_FORMAT env var." >&2
    exit 1
fi

MODE="format"
if [[ "${1:-}" == "--check" ]]; then
    MODE="check"
fi

FILES=$(find "$SRC_DIR" -type f \( -name '*.c' -o -name '*.h' \) \
    ! -path '*/resources/*')

if [[ -z "$FILES" ]]; then
    echo "No source files found."
    exit 0
fi

if [[ "$MODE" == "check" ]]; then
    echo "Checking format (dry-run)..."
    NEEDS_FORMAT=0
    for f in $FILES; do
        if ! "$CLANG_FORMAT" --dry-run --Werror "$f" 2>/dev/null; then
            echo "  needs formatting: $f"
            NEEDS_FORMAT=1
        fi
    done
    if [[ "$NEEDS_FORMAT" -eq 1 ]]; then
        echo "Some files need formatting. Run: ./scripts/format.sh"
        exit 1
    fi
    echo "All files are properly formatted."
else
    echo "Formatting source files..."
    for f in $FILES; do
        echo "  $f"
        "$CLANG_FORMAT" -i "$f"
    done
    echo "Done."
fi
