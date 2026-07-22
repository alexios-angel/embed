#!/usr/bin/env bash
# Unpack the newest fetched clang-std-embed toolchain into
# compile-time-browser/tools/clang-std-embed (binaries — never committed).
#
# Usage: ./sync-to-ctbrowser.sh [path-to-compile-time-browser]
set -euo pipefail
cd "$(dirname "$0")"

repo_root=$(git rev-parse --show-toplevel)
ctb="${1:-$repo_root/../compile-time-browser}"
[ -d "$ctb/include/ctbrowser" ] || { echo "compile-time-browser not found at $ctb (pass its path)" >&2; exit 1; }

archive=$(ls -t "$repo_root"/dist/clang-std-embed-*-linux-x86_64.tar.xz 2>/dev/null | head -1)
[ -n "$archive" ] || { echo "no linux-x86_64 dist archive in $repo_root/dist — run ./remote-build-clang.sh first" >&2; exit 1; }

dest="$ctb/tools/clang-std-embed"
echo "== Installing $(basename "$archive") -> $dest =="
rm -rf "$dest"
mkdir -p "$dest"
tar -C "$dest" --strip-components=1 -xJf "$archive"

if ! grep -qx 'tools/clang-std-embed/' "$ctb/.gitignore" 2>/dev/null; then
  # ensure we append as a NEW line even if the file lacks a trailing newline
  [ -s "$ctb/.gitignore" ] && [ -n "$(tail -c1 "$ctb/.gitignore")" ] && echo >> "$ctb/.gitignore"
  echo 'tools/clang-std-embed/' >> "$ctb/.gitignore"
fi

"$dest/bin/clang++" --version | head -1
echo "Installed. Use: $dest/bin/clang++ -std=c++2d --embed-dir=<assets> ..."
