#!/usr/bin/env bash
# Capture CPU state a few seconds after boot via QEMU monitor.
set -uo pipefail
cd /mnt/d/Basalt

{ sleep 5; echo "info registers"; sleep 1; echo "quit"; } | \
  timeout 15 qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso \
      -display none -serial file:build/serial.log -monitor stdio -no-reboot \
      > build/monitor.log 2>&1 || true

echo "===== monitor.log ====="
cat build/monitor.log
echo "===== serial.log ====="
cat build/serial.log 2>/dev/null || echo "(empty)"
