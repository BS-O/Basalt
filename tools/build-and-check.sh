#!/usr/bin/env bash
# Full M0 build + smoke test in one shot (avoids PS quoting pitfalls).
set -uo pipefail
cd /mnt/d/Basalt

echo "===== kernel ====="
make clean >/dev/null
make 2>&1 | tail -3
echo "===== iso ====="
make iso 2>&1 | tail -2
echo "===== check ====="
make check 2>&1
echo "check_exit=$?"
echo "===== serial.log ====="
cat build/serial.log 2>/dev/null || echo "(no serial.log)"
