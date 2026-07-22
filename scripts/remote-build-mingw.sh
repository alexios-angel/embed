#!/usr/bin/env bash
# Drive the std-embed llvm-mingw toolchain matrix on the shared devbox
# (repo: github.com/alexios-angel/llvm-mingw — build-everything.sh).
# A full pass is ~20-24 h (plain) / ~32-40 h (--pgo), so this wrapper is
# asynchronous, unlike remote-build-clang.sh:
#
# Usage: ./remote-build-mingw.sh start [build-everything.sh flags...]
#        ./remote-build-mingw.sh status
#        ./remote-build-mingw.sh fetch
#
#   start   sync this repo (the smokes compile examples/std/*), clone or
#           update ~/projects/llvm-mingw on the box, launch the driver
#           under nohup, detach.
#   status  tail the build log + list produced artifacts.
#   fetch   rsync finished archives + smoke .exes into ../llvm-mingw/dist
#           (sibling checkout), then run the smoke .exes natively via WSL
#           interop when available — the definitive Windows runtime check.
set -euo pipefail
cd "$(dirname "$0")"

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
cmd="${1:-status}"
[ $# -eq 0 ] || shift

case "$cmd" in
  start)
    echo "== Syncing embed repo to $host (smokes need examples/std) =="
    rsync -az --delete \
      --exclude '.git/' \
      --exclude 'dist/' \
      "$repo_root"/ "$host:projects/embed/"

    echo "== Preparing ~/projects/llvm-mingw and launching build-everything.sh =="
    ssh "$host" FLAGS="$*" 'bash -s' <<'REMOTE'
set -euo pipefail
if [ ! -d "$HOME/projects/llvm-mingw/.git" ]; then
  git clone https://github.com/alexios-angel/llvm-mingw "$HOME/projects/llvm-mingw"
else
  git -C "$HOME/projects/llvm-mingw" fetch origin master
  git -C "$HOME/projects/llvm-mingw" checkout master
  git -C "$HOME/projects/llvm-mingw" reset --hard origin/master
fi
cd "$HOME/projects/llvm-mingw"
if pgrep -f build-everything.sh >/dev/null; then
  echo "a build-everything.sh run is already active — not starting another" >&2
  exit 1
fi
nohup ./build-everything.sh $FLAGS > build-everything.log 2>&1 &
echo "started (pid $!) — poll with: ./remote-build-mingw.sh status"
REMOTE
    ;;
  status)
    ssh "$host" 'cd ~/projects/llvm-mingw 2>/dev/null || { echo "no llvm-mingw checkout on the box"; exit 1; }
      if pgrep -f build-everything.sh >/dev/null; then echo "== RUNNING =="; else echo "== NOT RUNNING =="; fi
      echo "--- log tail:"; tail -n 40 build-everything.log 2>/dev/null || echo "(no log yet)"
      echo "--- artifacts:"; ls -lh dist 2>/dev/null || echo "(none yet)"'
    ;;
  fetch)
    dest="$repo_root/../llvm-mingw/dist"
    mkdir -p "$dest"
    echo "== Fetching artifacts into $dest =="
    rsync -a --include 'llvm-mingw-*.tar.xz' --include 'llvm-mingw-*.zip' \
      --include '*.exe' --exclude '*' \
      "$host:projects/llvm-mingw/dist/" "$dest/"
    ls -lh "$dest"
    # WSL interop runs PE binaries natively on the Windows host — the
    # real-Windows runtime check for the compile-time-built smokes.
    if [ -e /proc/sys/fs/binfmt_misc/WSLInterop ] && [ -e "$dest/std-embed-smoke.exe" ]; then
      echo "== Running smoke .exes natively via WSL interop =="
      "$dest/std-embed-smoke.exe" && echo "NATIVE PASS: std::embed"
      if [ -e "$dest/std-fetch-smoke.exe" ]; then
        "$dest/std-fetch-smoke.exe" && echo "NATIVE PASS: std::fetch"
      fi
    fi
    ;;
  *)
    echo "usage: $0 {start [flags...]|status|fetch}" >&2
    exit 1
    ;;
esac
