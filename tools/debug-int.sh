#!/usr/bin/env bash
# Boot with QEMU exception logging to pinpoint the faulting instruction.
set -uo pipefail
cd /mnt/d/Basalt

rm -f build/qemu-int.log build/serial.log
{ sleep 8; echo "screendump build/scr.ppm"; sleep 1; echo "quit"; } | \
  timeout 20 qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso \
      -display none -serial file:build/serial.log -monitor stdio -no-reboot \
      -d int -D build/qemu-int.log >/dev/null 2>&1 || true

echo "===== serial.log ====="
cat build/serial.log 2>/dev/null || echo "(empty)"
echo "===== qemu-int.log (last 60 lines) ====="
tail -60 build/qemu-int.log 2>/dev/null || echo "(none)"
