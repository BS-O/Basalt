#!/usr/bin/env bash
# Copy every file the Limine host tool needs from the vendored binary tarball.
set -euo pipefail
ROOT=/mnt/d/Basalt
rm -rf /tmp/x-bin
mkdir -p /tmp/x-bin
tar -xJf "$ROOT/third_party/vendored/limine-binary-12.10.0.tar.xz" -C /tmp/x-bin
BIND=/tmp/x-bin/limine-binary
echo "=== binary package contents ==="
ls "$BIND"
cp "$BIND"/* "$ROOT/third_party/limine/"
echo "=== third_party/limine now ==="
ls "$ROOT/third_party/limine"
