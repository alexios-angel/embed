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

# Converge build dependencies from the repo's Brewfile (single source of
# truth; cloud-init only bootstraps brew itself).
if [ -x /home/linuxbrew/.linuxbrew/bin/brew ]; then
  export HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_ENV_HINTS=1
  BUNDLE="$HOME/embed/infra/azure-build-server/Brewfile"
  /home/linuxbrew/.linuxbrew/bin/brew bundle check --file="$BUNDLE" >/dev/null 2>&1 \
    || /home/linuxbrew/.linuxbrew/bin/brew bundle install --file="$BUNDLE"
  /home/linuxbrew/.linuxbrew/bin/ccache --set-config max_size=30G || true
fi

# Build with linuxbrew's latest clang when installed (brew install llvm):
# build.sh takes CC/CXX from the environment for native builds, and having
# brew's bin on PATH lets its ld.lld check switch host links to lld.
if [ -x /home/linuxbrew/.linuxbrew/bin/brew ]; then
  BREW_LLVM="$(/home/linuxbrew/.linuxbrew/bin/brew --prefix llvm 2>/dev/null || true)"
  if [ -n "$BREW_LLVM" ] && [ -x "$BREW_LLVM/bin/clang++" ]; then
    export CC="$BREW_LLVM/bin/clang" CXX="$BREW_LLVM/bin/clang++"
    # llvm is keg-only and lld is its own formula; both bins must be findable
    # (this also puts brew's cmake/ninja/ccache ahead of any apt ones)
    export PATH="$BREW_LLVM/bin:/home/linuxbrew/.linuxbrew/bin:$PATH"
    # keg-only curl isn't symlinked into the prefix: point CMake's FindCURL
    # (LLVM_ENABLE_CURL, for __builtin_std_fetch) at the brew keg
    BREW_CURL="$(/home/linuxbrew/.linuxbrew/bin/brew --prefix curl 2>/dev/null || true)"
    if [ -n "$BREW_CURL" ] && [ -d "$BREW_CURL" ]; then
      export CMAKE_PREFIX_PATH="$BREW_CURL${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
    fi
  fi
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

echo "== Smoke test: compile-time std::fetch (needs network + --fetch-allow) =="
ssh "ubuntu@$ip" 'bash -s' <<'REMOTE'
set -euo pipefail
tool=$(ls -dt "$HOME"/dist/clang-std-embed-*-linux-*/ | head -1); tool="${tool%/}"
src="$HOME/embed/examples/std/fetch/source"
"$tool/bin/clang++" -std=c++2d \
  "--fetch-allow=https://example.com/" \
  "--fetch-allow=https://example.com/**" \
  "$src/fetch_url.c++" -o /tmp/std-fetch-smoke
/tmp/std-fetch-smoke
# Negative: without --fetch-allow the fetch comes back NotAuthorized and
# the example's static_assert must fail the compile.
if "$tool/bin/clang++" -std=c++2d \
  "$src/fetch_url.c++" -o /tmp/std-fetch-smoke-denied 2>/dev/null; then
  echo "FETCH-NEGATIVE-FAILED: compiled without --fetch-allow" >&2
  exit 1
fi
echo "FETCH-NEGATIVE-PASSED: no --fetch-allow => compile error, as intended"
REMOTE

echo "== Fetching dist archive =="
mkdir -p "$repo_root/dist"
rsync -a --include 'clang-std-embed-*.tar.xz' --include 'clang-std-embed-*.zip' --exclude '*' \
  "ubuntu@$ip:dist/" "$repo_root/dist/"
ls -lh "$repo_root"/dist/clang-std-embed-*
echo "Done. Next: ./sync-to-ctbrowser.sh to install into compile-time-browser/tools."
