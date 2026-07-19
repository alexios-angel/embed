#!/bin/bash
# cloud-init for the std::embed clang build server (Ubuntu 24.04).
# Build dependencies come from linuxbrew via the sibling Brewfile, applied
# by remote-build-clang.sh on each run; cloud-init only bootstraps brew.
# apt provides just what brew's installer itself needs.
set -euxo pipefail

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y build-essential procps curl file git rsync

# Homebrew refuses to run as root: install as the ubuntu user.
sudo -iu ubuntu bash <<'EOSU'
set -euxo pipefail
if [ ! -x /home/linuxbrew/.linuxbrew/bin/brew ]; then
  NONINTERACTIVE=1 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi
echo 'eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"' >> ~/.profile
EOSU

touch /var/lib/cloud/instance/ctembed-ready
