#!/usr/bin/env bash
# M1 framebuffer screendump: boot QEMU headless, capture screen, convert to PNG.
set -uo pipefail
cd /mnt/d/Basalt

{ sleep 6; echo "screendump build/scr.ppm"; sleep 1; echo quit; } | \
  timeout 15 qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso \
      -display none -serial file:build/serial.log -monitor stdio -no-reboot \
      >/dev/null 2>&1 || true

python3 - <<'EOF'
import zlib, struct
with open("build/scr.ppm", "rb") as f:
    d = f.read()
assert d.startswith(b"P6")
i = 2
v = []
while len(v) < 3:
    while d[i:i+1].isspace():
        i += 1
    if d[i:i+1] == b"#":
        while d[i:i+1] != b"\n":
            i += 1
        continue
    s = i
    while not d[i:i+1].isspace():
        i += 1
    v.append(int(d[s:i]))
i += 1
w, h, _ = v
pix = d[i:i+w*h*3]
png = bytearray(b"\x89PNG\r\n\x1a\n")
def ch(t, p):
    c = struct.pack(">I", len(p)) + t + p
    return c + struct.pack(">I", zlib.crc32(t + p) & 0xffffffff)
png += ch(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
raw = bytearray()
for y in range(h):
    raw += b"\x00" + pix[y*w*3:(y+1)*w*3]
png += ch(b"IDAT", zlib.compress(bytes(raw), 9))
png += ch(b"IEND", b"")
open("build/m1-boot.png", "wb").write(png)
print("PNG:", w, "x", h)
EOF
