#!/usr/bin/env bash
# Build the std::embed clang toolchain on the shared devbox, smoke-test
# std::embed with the fresh compiler, and fetch the dist archive into ./dist.
#
# Usage: ./remote-build-clang.sh [target]     (default: native = linux-x86_64)
#
# The box is github.com/alexios-angel/infra (sibling checkout ../infra),
# reached via the `devbox` ssh alias that `../infra/azure-build-server/
# server.sh ssh-config` writes; DEVBOX_HOST=ubuntu@<ip> overrides.
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

host="${DEVBOX_HOST:-devbox}"
if ! ssh -o ConnectTimeout=5 "$host" true 2>/dev/null; then
  cat >&2 <<EOF
cannot reach '$host' — likely one of:
  alias missing:    ../infra/azure-build-server/server.sh ssh-config
  box deallocated:  ../infra/azure-build-server/server.sh start
  your IP changed:  ../infra/azure-build-server/server.sh allow-ip
or set DEVBOX_HOST=ubuntu@<ip>.
EOF
  exit 1
fi

repo_root=$(git rev-parse --show-toplevel)

echo "== Syncing embed repo to $host =="
rsync -az --delete \
  --exclude '.git/' \
  --exclude 'dist/' \
  "$repo_root"/ "$host:projects/embed/"

echo "== Fetching $BRANCH from $FORK_URL and building ($TARGET) =="
ssh "$host" FORK_URL="$FORK_URL" BRANCH="$BRANCH" LINK_JOBS="$LINK_JOBS" TARGET="$TARGET" 'bash -s' <<'REMOTE'
set -euo pipefail
if [ ! -d "$HOME/projects/llvm-project/.git" ]; then
  git clone --branch "$BRANCH" "$FORK_URL" "$HOME/projects/llvm-project"
else
  git -C "$HOME/projects/llvm-project" fetch origin "$BRANCH"
  git -C "$HOME/projects/llvm-project" checkout "$BRANCH"
  git -C "$HOME/projects/llvm-project" reset --hard "origin/$BRANCH"
fi

# Converge build dependencies from the repo's Brewfile (single source of
# truth; the devbox bootstraps brew but installs nothing — box-level ccache
# sizing lives in its cloud-init).
if [ -x /home/linuxbrew/.linuxbrew/bin/brew ]; then
  export HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_ENV_HINTS=1
  BUNDLE="$HOME/projects/embed/scripts/build-clang/Brewfile"
  /home/linuxbrew/.linuxbrew/bin/brew bundle check --file="$BUNDLE" >/dev/null 2>&1 \
    || /home/linuxbrew/.linuxbrew/bin/brew bundle install --file="$BUNDLE"
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

LLVM_SRC="$HOME/projects/llvm-project" \
BUILD_ROOT="$HOME/llvm-build" \
DIST_DIR="$HOME/dist" \
JOBS="$(nproc)" \
LINK_JOBS="$LINK_JOBS" \
  bash "$HOME/projects/embed/scripts/build-clang/build.sh" "$TARGET"
REMOTE

echo "== Smoke test: compile+run a std::embed example with the new clang++ =="
ssh "$host" 'bash -s' <<'REMOTE'
set -euo pipefail
# trailing slash: match the install DIRECTORY, never the .tar.xz next to it
tool=$(ls -dt "$HOME"/dist/clang-std-embed-*-linux-*/ | head -1); tool="${tool%/}"
src="$HOME/projects/embed/examples/std/basic/source"
"$tool/bin/clang++" -std=c++2d -Wno-c++2d-extensions \
  -I "$HOME/embed/include" \
  "--embed-dir=$src" \
  "$src/local_file.c++" -o /tmp/std-embed-smoke
/tmp/std-embed-smoke
echo "SMOKE-TEST-PASSED with $tool/bin/clang++"
REMOTE

echo "== Smoke test: compile-time std::fetch (needs network + --fetch-allow) =="
ssh "$host" 'bash -s' <<'REMOTE'
set -euo pipefail
tool=$(ls -dt "$HOME"/dist/clang-std-embed-*-linux-*/ | head -1); tool="${tool%/}"
src="$HOME/projects/embed/examples/std/fetch/source"
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
  "$host:dist/" "$repo_root/dist/"
ls -lh "$repo_root"/dist/clang-std-embed-*
echo "Done. Next: ./scripts/sync-to-ctbrowser.sh to install into compile-time-browser/tools."
