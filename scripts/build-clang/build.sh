#!/usr/bin/env bash
#
# Build the std::embed-patched clang/LLVM toolchain for common platforms.
#
# The LLVM source is expected to be a llvm-project checkout that already
# contains the std::embed implementation (default: ~/llvm-toolchain). To
# recreate that checkout from scratch, apply
# patches/0002-clang-full-std-embed-and-depend.patch onto upstream commit
# ced9fa35844724063b20d4b0d67a7960247c8067 (see scripts/build-clang/README.md).
#
# Native builds (run on the machine you are targeting):
#   ./build.sh native            autodetect host: linux-x86_64, linux-aarch64,
#                                macos-arm64 or macos-x86_64
#   ./build.sh macos-universal   macOS only: one arm64+x86_64 fat toolchain
#
# Cross builds (run on Linux/WSL x86_64):
#   ./build.sh linux-aarch64     needs: sudo apt install g++-aarch64-linux-gnu
#   ./build.sh windows-x86_64    needs: sudo apt install mingw-w64
#
# Native Windows/MSVC builds: use build.ps1 instead.
#
# Environment overrides:
#   LLVM_SRC        llvm-project checkout            (default: ~/llvm-toolchain)
#   BUILD_ROOT      where build trees go             (default: $LLVM_SRC/build-dist)
#   DIST_DIR        install trees + archives go here (default: <embed repo>/dist)
#   JOBS            compile parallelism              (default: all cores)
#   LINK_JOBS       link parallelism; links are RAM-hungry, ~4 GB each (default: 2)
#   ASSERTIONS      ON/OFF LLVM assertions           (default: OFF)
#   CONFIGURE_ONLY  1 = stop after the CMake configure step
#   CC / CXX        host compiler for native builds  (default: CMake's pick)

set -euo pipefail

usage() {
    sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

die() {
    echo "error: $*" >&2
    exit 1
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

LLVM_SRC="${LLVM_SRC:-$HOME/llvm-toolchain}"
BUILD_ROOT="${BUILD_ROOT:-$LLVM_SRC/build-dist}"
DIST_DIR="${DIST_DIR:-$REPO_DIR/dist}"
JOBS="${JOBS:-$( (nproc || sysctl -n hw.ncpu) 2>/dev/null )}"
LINK_JOBS="${LINK_JOBS:-2}"
ASSERTIONS="${ASSERTIONS:-OFF}"
CONFIGURE_ONLY="${CONFIGURE_ONLY:-0}"

TARGET="${1:-}"
case "$TARGET" in
    -h|--help|"") usage; exit 0 ;;
esac

[ -f "$LLVM_SRC/llvm/CMakeLists.txt" ] || die "LLVM source not found at $LLVM_SRC (set LLVM_SRC)"
command -v cmake >/dev/null || die "cmake not found"
command -v ninja >/dev/null || die "ninja not found"

HOST_OS="$(uname -s)"
HOST_ARCH="$(uname -m)"
[ "$HOST_ARCH" = "arm64" ] && HOST_ARCH=aarch64

host_target() {
    case "$HOST_OS-$HOST_ARCH" in
        Linux-x86_64)   echo linux-x86_64 ;;
        Linux-aarch64)  echo linux-aarch64 ;;
        Darwin-aarch64) echo macos-arm64 ;;
        Darwin-x86_64)  echo macos-x86_64 ;;
        *) die "unsupported host $HOST_OS/$HOST_ARCH (use build.ps1 on Windows)" ;;
    esac
}

if [ "$TARGET" = "native" ]; then
    TARGET="$(host_target)"
fi

CROSS=0
if [ "$TARGET" != "$(host_target 2>/dev/null || true)" ] && [ "$TARGET" != "macos-universal" ]; then
    CROSS=1
fi

CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE=Release
    -DLLVM_ENABLE_PROJECTS="clang;lld"
    -DLLVM_TARGETS_TO_BUILD="X86;AArch64"
    -DLLVM_ENABLE_ASSERTIONS="$ASSERTIONS"
    -DLLVM_INCLUDE_TESTS=OFF
    -DLLVM_INCLUDE_EXAMPLES=OFF
    -DLLVM_INCLUDE_BENCHMARKS=OFF
    -DLLVM_INCLUDE_DOCS=OFF
    -DCLANG_INCLUDE_TESTS=OFF
    -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON
    -DLLVM_ENABLE_ZSTD=OFF
    -DLLVM_ENABLE_CURL=ON   # __builtin_std_fetch: compile-time HTTP via libcurl
    -DLLVM_ENABLE_LIBXML2=OFF
    -DLLVM_PARALLEL_LINK_JOBS="$LINK_JOBS"
)

if command -v ccache >/dev/null; then
    CMAKE_ARGS+=(
        -DCMAKE_C_COMPILER_LAUNCHER=ccache
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    )
fi

# Cross builds run TableGen on the host, so a small native build of the
# generators has to exist first. LLVM_NATIVE_TOOL_DIR points the cross
# configure at them.
ensure_native_tools() {
    local ntdir="$BUILD_ROOT/native-tools"
    if [ ! -x "$ntdir/bin/llvm-tblgen" ] || [ ! -x "$ntdir/bin/clang-tblgen" ]; then
        echo "== Building native TableGen tools (one-time) =="
        cmake -S "$LLVM_SRC/llvm" -B "$ntdir" -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLVM_ENABLE_PROJECTS=clang \
            -DLLVM_TARGETS_TO_BUILD=Native \
            -DLLVM_INCLUDE_TESTS=OFF \
            -DLLVM_INCLUDE_EXAMPLES=OFF \
            -DLLVM_INCLUDE_BENCHMARKS=OFF
        ninja -C "$ntdir" llvm-tblgen clang-tblgen
        # Present in recent LLVM; harmless to skip if the target is gone.
        ninja -C "$ntdir" llvm-min-tblgen || true
    fi
    CMAKE_ARGS+=(-DLLVM_NATIVE_TOOL_DIR="$ntdir/bin")
}

cross_common_args() {
    CMAKE_ARGS+=(
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
        -DLLVM_ENABLE_ZLIB=OFF
    )
}

ARCHIVE=tar
case "$TARGET" in
    linux-x86_64|linux-aarch64|macos-arm64|macos-x86_64|macos-universal)
        if [ "$CROSS" = 0 ]; then
            # Native build: CMake picks CC/CXX from the environment as usual.
            if [ "$TARGET" = "macos-universal" ]; then
                [ "$HOST_OS" = Darwin ] || die "macos-universal must be built on macOS"
                CMAKE_ARGS+=(-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64")
            fi
            case "$TARGET" in
                macos-*) CMAKE_ARGS+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0) ;;
            esac
        else
            case "$TARGET" in
                linux-aarch64)
                    [ "$HOST_OS" = Linux ] || die "cross build for $TARGET must run on Linux"
                    command -v aarch64-linux-gnu-g++ >/dev/null \
                        || die "aarch64-linux-gnu-g++ not found (sudo apt install g++-aarch64-linux-gnu)"
                    ensure_native_tools
                    cross_common_args
                    CMAKE_ARGS+=(
                        -DCMAKE_SYSTEM_NAME=Linux
                        -DCMAKE_SYSTEM_PROCESSOR=aarch64
                        -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc
                        -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
                        -DLLVM_HOST_TRIPLE=aarch64-linux-gnu
                    )
                    ;;
                macos-*)
                    die "macOS toolchains must be built on macOS (needs the Apple SDK); run this script there"
                    ;;
                *)
                    die "cannot cross-build $TARGET from this host"
                    ;;
            esac
        fi
        ;;

    windows-x86_64)
        [ "$HOST_OS" = Linux ] || die "windows-x86_64 (MinGW) cross build must run on Linux; for MSVC use build.ps1 on Windows"
        # Prefer the posix-threads flavour: LLVM needs working std::thread.
        MINGW_CC="$(command -v x86_64-w64-mingw32-gcc-posix || command -v x86_64-w64-mingw32-gcc || true)"
        MINGW_CXX="$(command -v x86_64-w64-mingw32-g++-posix || command -v x86_64-w64-mingw32-g++ || true)"
        [ -n "$MINGW_CXX" ] || die "MinGW toolchain not found (sudo apt install mingw-w64)"
        ensure_native_tools
        cross_common_args
        CMAKE_ARGS+=(
            -DCMAKE_SYSTEM_NAME=Windows
            -DCMAKE_C_COMPILER="$MINGW_CC"
            -DCMAKE_CXX_COMPILER="$MINGW_CXX"
            -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres
            -DLLVM_HOST_TRIPLE=x86_64-w64-windows-gnu
            -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++"
        )
        ARCHIVE=zip
        ;;

    *)
        usage
        die "unknown target '$TARGET'"
        ;;
esac

# Use lld for host links when the host compiler is clang and lld is around;
# it cuts link times drastically. GCC hosts keep their default linker.
if [ "$CROSS" = 0 ] && [ "$HOST_OS" = Linux ] && command -v ld.lld >/dev/null; then
    case "${CXX:-}${CC:-}" in
        *clang*) CMAKE_ARGS+=(-DLLVM_USE_LINKER=lld) ;;
    esac
fi

LLVM_REV="$(git -C "$LLVM_SRC" rev-parse --short HEAD 2>/dev/null || echo local)"
NAME="clang-std-embed-$LLVM_REV-$TARGET"
BUILD_DIR="$BUILD_ROOT/$TARGET"
STAGE_DIR="$DIST_DIR/$NAME"

echo "== Configuring $TARGET (build dir: $BUILD_DIR) =="
cmake -S "$LLVM_SRC/llvm" -B "$BUILD_DIR" -G Ninja "${CMAKE_ARGS[@]}"

if [ "$CONFIGURE_ONLY" = 1 ]; then
    echo "== Configure-only requested; stopping. Build with: ninja -C $BUILD_DIR =="
    exit 0
fi

echo "== Building (jobs: $JOBS, link jobs: $LINK_JOBS) =="
ninja -C "$BUILD_DIR" -j "$JOBS"

echo "== Installing to $STAGE_DIR =="
rm -rf "$STAGE_DIR"
cmake --install "$BUILD_DIR" --prefix "$STAGE_DIR" --strip

echo "== Packaging =="
mkdir -p "$DIST_DIR"
if [ "$ARCHIVE" = zip ] && command -v zip >/dev/null; then
    (cd "$DIST_DIR" && rm -f "$NAME.zip" && zip -qry "$NAME.zip" "$NAME")
    echo "Done: $DIST_DIR/$NAME.zip"
else
    tar -C "$DIST_DIR" -cJf "$DIST_DIR/$NAME.tar.xz" "$NAME"
    echo "Done: $DIST_DIR/$NAME.tar.xz"
fi

if [ "$CROSS" = 0 ]; then
    echo
    echo "Point the embed repo at it, e.g. in CMakeUserPresets.json:"
    echo "  \"CMAKE_CXX_COMPILER\": \"$STAGE_DIR/bin/clang++\""
fi
