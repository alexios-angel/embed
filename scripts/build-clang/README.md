# Building the std::embed clang toolchain

These scripts build the patched clang (with `__builtin_std_embed` / full
`std::embed` and `#depend` support) for the common platforms:

| Platform          | How                                              | Script |
|-------------------|--------------------------------------------------|--------|
| Linux x86_64      | native (works in WSL2)                           | `build.sh native` |
| Linux aarch64     | native on an arm64 box, or cross from x86_64     | `build.sh native` / `build.sh linux-aarch64` |
| Windows x86_64    | native MSVC, or MinGW cross from Linux           | `build.ps1` / `build.sh windows-x86_64` |
| macOS arm64/x86_64| native on a Mac (Apple SDK required)             | `build.sh native` or `build.sh macos-universal` |

All of the above are also built natively in CI by
[`.github/workflows/build-clang.yml`](../../.github/workflows/build-clang.yml)
(manual trigger: *Actions → build-clang-std-embed → Run workflow*), which
uploads a toolchain archive per platform. That is the easiest way to cover
every platform at once — native builds avoid all cross-compilation caveats.

## Source layout

The scripts build from an llvm-project checkout that already contains the
std::embed implementation. Locally that is `~/llvm-toolchain` (override with
`LLVM_SRC=...`). To recreate it from scratch:

```sh
git clone https://github.com/llvm/llvm-project.git llvm-toolchain
cd llvm-toolchain
git checkout ced9fa35844724063b20d4b0d67a7960247c8067
git apply /path/to/embed/patches/0002-clang-full-std-embed-and-depend.patch
```

`patches/0002-clang-full-std-embed-and-depend.patch` is the exported
`[clang][Lex][Sema] Full std::embed and #depend implementation` commit;
regenerate it after changing the toolchain with:

```sh
git -C ~/llvm-toolchain format-patch -1 HEAD --stdout \
  > patches/0002-clang-full-std-embed-and-depend.patch
```

If the patch moves to a new base, update `LLVM_BASE` in the workflow and the
commit hash above.

## Local usage (Linux / WSL2 / macOS)

```sh
# Native toolchain for this machine:
./scripts/build-clang/build.sh native

# Cross builds from Linux/WSL x86_64:
sudo apt install g++-aarch64-linux-gnu   # once
./scripts/build-clang/build.sh linux-aarch64

sudo apt install mingw-w64               # once
./scripts/build-clang/build.sh windows-x86_64
```

Results land in `dist/clang-std-embed-<rev>-<target>/` plus a
`.tar.xz`/`.zip` of the same. Build trees live in `~/llvm-toolchain/build-dist/`
so nothing large is written under `/mnt/c`.

Knobs (environment variables): `LLVM_SRC`, `BUILD_ROOT`, `DIST_DIR`, `JOBS`,
`LINK_JOBS` (default 2 — clang links need ~4 GB RAM each; lower it on small
machines), `ASSERTIONS=ON` for a checked compiler, `CONFIGURE_ONLY=1` to stop
after CMake configure.

Cross builds automatically bootstrap a small **native TableGen build** first
(`build-dist/native-tools/`) — TableGen must run on the build machine while
everything else compiles for the target.

## Windows (MSVC)

From any PowerShell prompt on a machine with Visual Studio 2022 (C++
workload), CMake and Ninja:

```powershell
.\scripts\build-clang\build.ps1 -LlvmSrc C:\src\llvm-toolchain
```

The script enters the VS developer environment by itself via vswhere.

## What gets built

Release, `clang` + `lld`, X86 + AArch64 backends, toolchain-only install
(no LLVM development headers/libs), zstd/libxml2 disabled for portable
binaries. The MinGW build links `-static` so the resulting `clang.exe` has no
runtime DLL dependencies.

## Using the result with this repo

Point a preset at the installed toolchain (see `CMakeUserPresets.json`, the
`embed-std-clang` preset) — e.g.:

```json
"CMAKE_CXX_COMPILER": "<repo>/dist/clang-std-embed-<rev>-linux-x86_64/bin/clang++"
```

then `cmake --preset embed-std-clang && cmake --build --preset embed-std-clang && ctest --preset embed-std-clang`.
