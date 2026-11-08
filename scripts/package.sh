#!/usr/bin/env bash
# ==============================================================================
# scripts/package.sh - stage a built CarveSpace VST3 bundle into dist/
#
# The VST3 bundle is produced by CMake at
#   <build-dir>/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3
# and is copied here to
#   dist/<platform>/CarveSpace.vst3
# (plus a portable .zip with --zip).
#
# Examples:
#   ./scripts/package.sh                       # copy current platform from ./build
#   ./scripts/package.sh --build               # configure+build, then copy
#   ./scripts/package.sh --build --build-dir build-ci
#   ./scripts/package.sh --windows --build     # cross-build (needs LLVM-MinGW)
#   ./scripts/package.sh --windows            # copy an existing build-win/ bundle
#   ./scripts/package.sh --zip                # also write dist/CarveSpace-*.zip
#
# Supported platforms: linux, windows (x86_64), macos (arm64).
# ==============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_JOBS=4

PLATFORM=""
BUILD=0
ZIP=0
JOBS="${JOBS:-$DEFAULT_JOBS}"
BUILD_DIR=""

usage() {
    # Print the header comment block (everything between the shebang and the
    # first non-comment line), with the leading "# " stripped.
    awk 'NR == 1 { next } /^set -euo pipefail/ { exit } { sub(/^# ?/, ""); print }' \
        "${BASH_SOURCE[0]}"
}

die() { echo "error: $*" >&2; exit 1; }
info() { echo "==> $*"; }

detect_host_platform() {
    case "$(uname -s 2>/dev/null)" in
        Linux*)                 echo linux ;;
        Darwin*)                echo macos ;;
        MINGW*|MSYS*|CYGWIN*)   echo windows ;;
        *)                      echo unknown ;;
    esac
}

platform_arch() {
    case "$1" in
        linux)   echo x86_64 ;;
        windows) echo x64 ;;
        macos)   echo arm64 ;;
        *)       echo unknown ;;
    esac
}

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)      usage; exit 0 ;;
        --linux)        PLATFORM=linux ;;
        --windows)      PLATFORM=windows ;;
        --macos)        PLATFORM=macos ;;
        --build)        BUILD=1 ;;
        --zip)          ZIP=1 ;;
        --no-zip)       ZIP=0 ;;
        --build-dir)    shift; [ $# -gt 0 ] || die "--build-dir needs a value"; BUILD_DIR="$1" ;;
        --build-dir=*)  BUILD_DIR="${1#*=}" ;;
        --jobs|-j)      shift; [ $# -gt 0 ] || die "$1 needs a value"; JOBS="$1" ;;
        --jobs=*)       JOBS="${1#*=}" ;;
        *)              die "unknown argument: $1 (try --help)" ;;
    esac
    shift
done

HOST_PLATFORM="$(detect_host_platform)"
[ -n "$PLATFORM" ] || PLATFORM="$HOST_PLATFORM"
[ "$PLATFORM" != "unknown" ] || die "cannot detect this platform; use --linux, --windows or --macos"
[ "$PLATFORM" = "linux" ] || [ "$PLATFORM" = "windows" ] || [ "$PLATFORM" = "macos" ] || \
    die "unsupported platform '$PLATFORM'"

# Sensible build-dir defaults: cross-builds use build-win, everything else build.
if [ -z "$BUILD_DIR" ]; then
    if [ "$PLATFORM" = "windows" ] && [ "$HOST_PLATFORM" != "windows" ]; then
        BUILD_DIR="$ROOT/build-win"
    else
        BUILD_DIR="$ROOT/build"
    fi
fi
case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT/$BUILD_DIR" ;;
esac

VST3_REL="CarveSpace_artefacts/Release/VST3/CarveSpace.vst3"

# --- Optional configure + build ---------------------------------------------
if [ "$BUILD" -eq 1 ]; then
    info "Configuring ($PLATFORM) in $BUILD_DIR"
    cmake_args=(-S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release)
    case "$PLATFORM" in
        linux)
            command -v ninja >/dev/null 2>&1 && cmake_args+=(-G Ninja)
            ;;
        macos)
            command -v ninja >/dev/null 2>&1 && cmake_args+=(-G Ninja)
            [ "$HOST_PLATFORM" = "macos" ] || die "macOS builds must run on macOS"
            cmake_args+=(
                -DCMAKE_OSX_ARCHITECTURES="${CARVESPACE_OSX_ARCH:-arm64}"
                -DCMAKE_OSX_DEPLOYMENT_TARGET="${CARVESPACE_OSX_DEPLOYMENT_TARGET:-11.0}"
            )
            ;;
        windows)
            if [ "$HOST_PLATFORM" = "windows" ]; then
                # Native Windows: Ninja if available, else the VS 2022 generator.
                if command -v ninja >/dev/null 2>&1; then
                    cmake_args+=(-G Ninja)
                else
                    cmake_args+=(-G "Visual Studio 17 2022" -A x64)
                fi
            else
                # Cross-compile with portable LLVM-MinGW.
                if [ ! -f "$ROOT/cmake/toolchains/mingw-w64.cmake" ]; then
                    die "missing cmake/toolchains/mingw-w64.cmake"
                fi
                export LLVM_MINGW_ROOT="${LLVM_MINGW_ROOT:-/tmp/llvm-mingw}"
                [ -x "$LLVM_MINGW_ROOT/bin/x86_64-w64-mingw32-clang" ] || \
                    die "LLVM-MinGW not found at LLVM_MINGW_ROOT=$LLVM_MINGW_ROOT (see docs/BUILD-CROSS.md)"
                cmake_args+=(
                    -G Ninja
                    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/toolchains/mingw-w64.cmake"
                )
            fi
            ;;
    esac

    cmake "${cmake_args[@]}"

    info "Building CarveSpace_VST3 (-j$JOBS)"
    cmake --build "$BUILD_DIR" --target CarveSpace_VST3 --parallel "$JOBS"

    # Standalone is a nice extra when available; never fail the package over it.
    if [ "${CARVESPACE_WITH_STANDALONE:-1}" = "1" ]; then
        info "Building CarveSpace_Standalone (optional)"
        cmake --build "$BUILD_DIR" --target CarveSpace_Standalone --parallel "$JOBS" || \
            echo "warning: Standalone build failed; continuing with the VST3 only" >&2
    fi
fi

# --- Locate and stage the built bundle --------------------------------------
VST3_SRC="$BUILD_DIR/$VST3_REL"
if [ ! -d "$VST3_SRC" ]; then
    die "VST3 bundle not found: $VST3_SRC
       Build it first (./scripts/package.sh --build ...) or pass --build-dir <dir>."
fi

DEST_DIR="$ROOT/dist/$PLATFORM"
mkdir -p "$DEST_DIR"
rm -rf "$DEST_DIR/CarveSpace.vst3"
cp -R "$VST3_SRC" "$DEST_DIR/CarveSpace.vst3"

# Quick structural sanity check: the bundle must contain a real binary.
case "$PLATFORM" in
    linux)   INNER="Contents/x86_64-linux/CarveSpace.so" ;;
    windows) INNER="Contents/x86_64-win/CarveSpace.vst3" ;;
    macos)   INNER="Contents/MacOS/CarveSpace" ;;
esac
[ -s "$DEST_DIR/CarveSpace.vst3/$INNER" ] || \
    die "staged bundle is missing $INNER - the build did not finish correctly"

info "VST3 bundle: $DEST_DIR/CarveSpace.vst3"

# Optional standalone executable/app.
case "$PLATFORM" in
    linux)   STANDALONE="$BUILD_DIR/CarveSpace_artefacts/Release/Standalone/CarveSpace" ;;
    windows) STANDALONE="$BUILD_DIR/CarveSpace_artefacts/Release/Standalone/CarveSpace.exe" ;;
    macos)   STANDALONE="$BUILD_DIR/CarveSpace_artefacts/Release/Standalone/CarveSpace.app" ;;
esac
STAGED_STANDALONE=""
if [ -e "$STANDALONE" ]; then
    STANDALONE_NAME="$(basename "$STANDALONE")"
    rm -rf "${DEST_DIR:?}/$STANDALONE_NAME"
    cp -R "$STANDALONE" "$DEST_DIR/"
    STAGED_STANDALONE="$DEST_DIR/$STANDALONE_NAME"
    info "Standalone:  $STAGED_STANDALONE"
fi

# --- Optional portable archive ---------------------------------------------
if [ "$ZIP" -eq 1 ]; then
    ARCH="$(platform_arch "$PLATFORM")"
    ZIP_PATH="$ROOT/dist/CarveSpace-${PLATFORM}-${ARCH}.zip"
    rm -f "$ZIP_PATH"
    ITEMS=(CarveSpace.vst3)
    [ -n "$STAGED_STANDALONE" ] && ITEMS+=("$(basename "$STAGED_STANDALONE")")

    if command -v zip >/dev/null 2>&1; then
        ( cd "$DEST_DIR" && zip -r -9 -q "$ZIP_PATH" "${ITEMS[@]}" )
    elif command -v ditto >/dev/null 2>&1; then
        ( cd "$DEST_DIR" && ditto -c -k --keepParent "${ITEMS[0]}" "$ZIP_PATH" )
    elif command -v python3 >/dev/null 2>&1 || command -v python >/dev/null 2>&1; then
        PY="$(command -v python3 || command -v python)"
        ( cd "$DEST_DIR" && "$PY" - "$ZIP_PATH" "${ITEMS[@]}" <<'PYEOF'
import sys, os, zipfile
out, items = sys.argv[1], sys.argv[2:]
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for item in items:
        if os.path.isdir(item):
            for base, _, files in os.walk(item):
                for name in files:
                    p = os.path.join(base, name)
                    z.write(p, p)
        else:
            z.write(item, item)
PYEOF
        )
    else
        echo "warning: no zip/ditto/python3 found; skipping archive" >&2
    fi
    [ -f "$ZIP_PATH" ] && info "Archive:     $ZIP_PATH"
fi

echo
echo "Package ready:"
echo "  $DEST_DIR/CarveSpace.vst3"
[ -n "$STAGED_STANDALONE" ] && echo "  $STAGED_STANDALONE"
[ "$ZIP" -eq 1 ] && [ -f "$ZIP_PATH" ] && echo "  $ZIP_PATH"
exit 0
