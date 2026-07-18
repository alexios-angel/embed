#!/bin/bash
# cloud-init for the std::embed clang build server (Ubuntu 24.04).
# Toolchain for building llvm-project (clang + lld) via
# embed/scripts/build-clang/build.sh.
set -euxo pipefail

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y \
  build-essential g++ gcc \
  cmake ninja-build make \
  git python3 rsync ccache \
  xz-utils zip unzip pkg-config htop

# LLVM rebuilds benefit hugely from ccache across incremental patches
sudo -u ubuntu ccache --set-config max_size=30G || true

touch /var/lib/cloud/instance/ctembed-ready
