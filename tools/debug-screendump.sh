#!/usr/bin/env bash
# Headless boot diagnosis: screendump via QEMU monitor + PNG conversion in one run.
set -euo pipefail
cd /mnt/d/Basalt

{ sleep 6; echo "screendump build/scr.ppm"; sleep 1; echo "quit"; } | \
  timeout 15 qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso \
      -display none -serial file:build/serial.log -monitor stdio -no-reboot >/dev/null 2>&1 || true

ls -l build/scr.ppm

python3 - <<'EOF'
import zlib, struct
with open("/mnt/d/Basalt/build/scr.ppm", "rb") as f:
    data = f.read()
assert data.startswith(b"P6")
idx = 2
vals = []
while len(vals) < 3:
    while idx < len(data) and data[idx:idx+1].isspace():
        idx += 1
    if data[idx:idx+1] == b"#":
        while data[idx:idx+1] != b"\n":
            idx += 1
        continue
    start = idx
    while idx < len(data) and not data[idx:idx+1].isspace():
        idx += 1
    vals.append(int(data[start:idx]))
idx += 1
w, h, maxval = vals
pix = data[idx:idx + w*h*3]
png = bytearray()
png += b"\x89PNG\r\n\x1a\n"
def chunk(tag, payload):
    c = struct.pack(">I", len(payload)) + tag + payload
    c += struct.pack(">I", zlib.crc32(tag + payload) & 0xffffffff)
    return c
png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
raw = bytearray()
for y in range(h):
    raw += b"\x00" + pix[y*w*3:(y+1)*w*3]
png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
png += chunk(b"IEND", b"")
with open("/mnt/d/Basalt/build/screendump.png", "wb") as f:
    f.write(png)
print(f"PNG written: {w}x{h}")
EOF

echo '=== serial.log ==='
wc -c build/serial.log 2>/dev/null || echo "(no serial.log)"
cat build/serial.log 2>/dev/null || true
