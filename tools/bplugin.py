#!/usr/bin/env python3
"""Emit a minimal Basalt plugin module for testing the plugin registry.

A Basalt plugin module is a flat blob whose first bytes are a little-endian
bplugin_header (see kernel/plugins/plugins.c). This script emits a header +
name + version strings blob with fixed virtual addresses (M1: the module is
identity-mapped by Limine at a conventional address chosen by the loader;
here we place data at offset 0x40 and point the header at it).

Usage: python3 tools/bplugin.py <name> <version> <out.bin>
"""
import struct
import sys

MAGIC = 0x4253504C  # 'BSPL'
API = 1

def main():
    name, version, out = sys.argv[1], sys.argv[2], sys.argv[3]
    hdr = struct.pack("<IIQQQ", MAGIC, API, 0, 0, 0)  # init=0 (none)
    blob = bytearray()
    blob += hdr
    blob += b"\x00" * ((-len(blob)) % 0x40)  # pad to 0x40
    name_off = len(blob)
    blob += name.encode() + b"\x00"
    ver_off = len(blob)
    blob += version.encode() + b"\x00"
    # fix up header fields (name/version at their blob offsets; Limine maps
    # modules in place, so offset == address within the module and the
    # registry treats them as offsets from the module base at scan time)
    blob[16:24] = struct.pack("<Q", name_off)
    blob[24:32] = struct.pack("<Q", ver_off)
    with open(out, "wb") as f:
        f.write(bytes(blob))
    sys.stderr.write("wrote %s (%d bytes): plugin '%s' v%s\n" % (out, len(blob), name, version))

if __name__ == "__main__":
    main()
