#!/usr/bin/env bash
set -euo pipefail

# Install required system packages for Ubuntu/Debian.
sudo apt-get update
sudo apt-get install -y build-essential gcc gdb meson ninja-build pkg-config

echo "Setup complete."
