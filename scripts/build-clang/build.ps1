<#
.SYNOPSIS
    Build the std::embed-patched clang/LLVM toolchain natively on Windows with MSVC.

.DESCRIPTION
    Requires: Visual Studio 2022 (C++ workload), CMake, Ninja, git.
    Run from any PowerShell prompt; the script locates the VS developer
    environment via vswhere if cl.exe is not already on PATH.

    The LLVM source must already contain the std::embed implementation
    (apply patches/0002-clang-full-std-embed-and-depend.patch onto upstream
    commit ced9fa35844724063b20d4b0d67a7960247c8067 if starting fresh).

.EXAMPLE
    .\build.ps1 -LlvmSrc C:\src\llvm-toolchain
#>
param(
    [string]$LlvmSrc = "$env:USERPROFILE\llvm-toolchain",
    [string]$BuildRoot = "",
    [string]$DistDir = "",
    [int]$LinkJobs = 2,
    [ValidateSet("ON", "OFF")][string]$Assertions = "OFF",
    [switch]$ConfigureOnly
)

$ErrorActionPreference = "Stop"

$RepoDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
if (-not $BuildRoot) { $BuildRoot = Join-Path $LlvmSrc "build-dist" }
if (-not $DistDir)   { $DistDir = Join-Path $RepoDir "dist" }

if (-not (Test-Path (Join-Path $LlvmSrc "llvm\CMakeLists.txt"))) {
    throw "LLVM source not found at $LlvmSrc (pass -LlvmSrc)"
}

# Enter the VS developer environment if cl.exe is not already available.
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { throw "cl.exe not on PATH and vswhere.exe not found; install Visual Studio 2022 with the C++ workload" }
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw "no Visual Studio installation with C++ tools found" }
    Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64"
}

foreach ($tool in "cmake", "ninja") {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "$tool not found on PATH" }
}

$Target = "windows-x86_64-msvc"
$BuildDir = Join-Path $BuildRoot $Target
$LlvmRev = (& git -C $LlvmSrc rev-parse --short HEAD 2>$null)
if (-not $LlvmRev) { $LlvmRev = "local" }
$Name = "clang-std-embed-$LlvmRev-$Target"
$StageDir = Join-Path $DistDir $Name

Write-Host "== Configuring $Target (build dir: $BuildDir) =="
cmake -S (Join-Path $LlvmSrc "llvm") -B $BuildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER=cl `
    -DCMAKE_CXX_COMPILER=cl `
    -DLLVM_ENABLE_PROJECTS="clang;lld" `
    -DLLVM_TARGETS_TO_BUILD="X86;AArch64" `
    -DLLVM_ENABLE_ASSERTIONS=$Assertions `
    -DLLVM_INCLUDE_TESTS=OFF `
    -DLLVM_INCLUDE_EXAMPLES=OFF `
    -DLLVM_INCLUDE_BENCHMARKS=OFF `
    -DLLVM_INCLUDE_DOCS=OFF `
    -DCLANG_INCLUDE_TESTS=OFF `
    -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON `
    -DLLVM_ENABLE_ZSTD=OFF `
    -DLLVM_ENABLE_LIBXML2=OFF `
    -DLLVM_ENABLE_ZLIB=OFF `
    -DLLVM_PARALLEL_LINK_JOBS=$LinkJobs
if ($LASTEXITCODE -ne 0) { throw "configure failed" }

if ($ConfigureOnly) {
    Write-Host "== Configure-only requested; stopping. Build with: ninja -C $BuildDir =="
    exit 0
}

Write-Host "== Building =="
ninja -C $BuildDir
if ($LASTEXITCODE -ne 0) { throw "build failed" }

Write-Host "== Installing to $StageDir =="
if (Test-Path $StageDir) { Remove-Item -Recurse -Force $StageDir }
cmake --install $BuildDir --prefix $StageDir
if ($LASTEXITCODE -ne 0) { throw "install failed" }

Write-Host "== Packaging =="
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
$ZipPath = Join-Path $DistDir "$Name.zip"
if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }
Compress-Archive -Path $StageDir -DestinationPath $ZipPath
Write-Host "Done: $ZipPath"
