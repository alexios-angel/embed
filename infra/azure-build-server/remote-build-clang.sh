#!/usr/bin/env bash
# Build the std::embed clang toolchain on the Azure build server, smoke-test
# std::embed with the fresh compiler, and fetch the dist archive into ./dist.
#
# Usage: ./remote-build-clang.sh [target]     (default: native = linux-x86_64)
#
# Environment overrides:
#   FORK_URL   llvm-project fork to build      (default: alexios-angel/llvm-project)
#   BRANCH     branch with the std::embed work (default: std-embed)
#   LINK_JOBS  parallel links, ~4 GB RAM each  (default: 4 — fits 32 GiB
#              alongside 8 compile jobs; raise on a 64 GiB box)
set -euo pipefail
cd "$(dirname "$0")"

FORK_URL="${FORK_URL:-https://github.com/alexios-angel/llvm-project.git}"
BRANCH="${BRANCH:-std-embed}"
LINK_JOBS="${LINK_JOBS:-4}"
TARGET="${1:-native}"

ip=$(./server.sh ip)
[ -n "$ip" ] || { echo "no public ip in terraform output — has this been applied?" >&2; exit 1; }

repo_root=$(git -C .. rev-parse --show-toplevel)

echo "== Syncing embed repo to $ip =="
rsync -az --delete \
  --exclude '.git/' \
  --exclude 'dist/' \
  --exclude 'infra/azure-build-server/.terraform/' \
  --exclude '*.tfstate*' --exclude 'tfplan' \
  "$repo_root"/ "ubuntu@$ip:embed/"

echo "== Fetching $BRANCH from $FORK_URL and building ($TARGET) =="
ssh "ubuntu@$ip" FORK_URL="$FORK_URL" BRANCH="$BRANCH" LINK_JOBS="$LINK_JOBS" TARGET="$TARGET" 'bash -s' <<'REMOTE'
set -euo pipefail
if [ ! -d "$HOME/llvm-project/.git" ]; then
  git clone --branch "$BRANCH" "$FORK_URL" "$HOME/llvm-project"
else
  git -C "$HOME/llvm-project" fetch origin "$BRANCH"
  git -C "$HOME/llvm-project" checkout "$BRANCH"
  git -C "$HOME/llvm-project" reset --hard "origin/$BRANCH"
fi

LLVM_SRC="$HOME/llvm-project" \
BUILD_ROOT="$HOME/llvm-build" \
DIST_DIR="$HOME/dist" \
JOBS="$(nproc)" \
LINK_JOBS="$LINK_JOBS" \
  bash "$HOME/embed/scripts/build-clang/build.sh" "$TARGET"
REMOTE

echo "== Smoke test: compile+run a std::embed example with the new clang++ =="
ssh "ubuntu@$ip" 'bash -s' <<'REMOTE'
set -euo pipefail
# trailing slash: match the install DIRECTORY, never the .tar.xz next to it
tool=$(ls -dt "$HOME"/dist/clang-std-embed-*-linux-*/ | head -1); tool="${tool%/}"
src="$HOME/embed/examples/std/basic/source"
"$tool/bin/clang++" -std=c++2d -Wno-c++2d-extensions \
  -I "$HOME/embed/include" \
  "--embed-dir=$src" \
  "$src/local_file.c++" -o /tmp/std-embed-smoke
/tmp/std-embed-smoke
echo "SMOKE-TEST-PASSED with $tool/bin/clang++"
REMOTE

echo "== Fetching dist archive =="
mkdir -p "$repo_root/dist"
rsync -a --include 'clang-std-embed-*.tar.xz' --include 'clang-std-embed-*.zip' --exclude '*' \
  "ubuntu@$ip:dist/" "$repo_root/dist/"
ls -lh "$repo_root"/dist/clang-std-embed-*
echo "Done. Next: ./sync-to-ctbrowser.sh to install into compile-time-browser/tools."
