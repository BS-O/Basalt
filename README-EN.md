# BasaltCore

[![ci](https://github.com/BS-O/Basalt/actions/workflows/ci.yml/badge.svg)](https://github.com/BS-O/Basalt/actions/workflows/ci.yml)

> Read this page in Chinese: [README.md](README.md) · English version on the site: <https://basalt.bsct.top/readme/en/>

**Basalt — the strongest layer of stone beneath the earth's crust.**

BasaltCore is the **core kernel of the Basalt system**: a self-built x86-64 kernel written from bare metal (an educational / research project). It is deliberately just the foundation of the wider Basalt ecosystem — **a Basalt plugin system (basalt plugin) is planned next**, while the kernel itself stays small and solid.

| | |
|---|---|
| Current milestone | **M1**: PMM memory management + early-Unix/Linux-style boot log + framebuffer shell + plugin mechanism |
| Boot | Limine Boot Protocol (v12.10.0) |
| Languages | C++17 subset (subsystem layer) + C (hardware layer) + GNU as (entry stub) |
| Runs on | WSL2 + QEMU — everything inside an emulator, never touching a real disk |

## Quick start

```bash
cd /mnt/d/Basalt     # WSL2 Debian
make && make iso     # kernel ELF -> bootable ISO with a sample plugin
make run             # QEMU window: boot log -> bsh shell (type on the keyboard)
make check           # headless smoke test: assert the M1 boot log on COM1
```

On boot you get an early-Linux-style banner and line-by-line log, then the **bsh** shell:

```
BasaltCore 0.1.0 (x86-64) - Limine Boot Protocol, base revision 3
Copyright (c) 2026 Basalt Project. booting...

fb0      : 1024x768 x 32 bpp, pitch 4096 - framebuffer console up
ttyS0    : COM1 16550, 115200 8N1 - debug/CI console
mem     : usable 260492 KiB, 65377 frames managed by bitmap PMM (hhdm +0x0)
i8042    : PS/2 keyboard ok (polled)
plugins  : 1 plugin(s) detected: hello
plugins  : 1/1 initialized ok

BasaltCore is ready. starting shell...

BasaltCore shell (bsh) v0.1.0
1 plugin(s) installed: hello
Type 'help' for commands.

basalt# ver
BasaltCore 0.1.0 (M1) - x86-64 kernel, Limine Boot Protocol
```

Shell commands: `help` `ver` `clear` `echo` `meminfo` `plugins` `halt`.

## Plugin system (getting started)

- Both the **boot log** and the **shell banner** report how many plugins are installed and their names;
- With no plugins installed, the kernel prints: *"search 'basalt plugin' on GitHub (github.com/BS-O/Basalt)"*;
- A plugin is a `BSPL`-format binary loaded as a Limine `module_path` module (`tools/bplugin.py` emits a sample);
- Planned: a plugin registry index, a plugin-to-plugin call ABI, and a `basalt plugin install` ecosystem.

## Documentation

- [docs/PROJECT-GUIDE.md](docs/PROJECT-GUIDE.md) — purpose and usage of every file (Chinese; [English guide](docs/PROJECT-GUIDE-EN.md))
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — layering and the boot chain
- [docs/TASK-SPEC.md](docs/TASK-SPEC.md) — task spec v2 (Limine revision)
- [docs/adr/](docs/adr/) — decision records ADR-001~006
- Website: <https://basalt.bsct.top/> (GitHub Pages)

## License and quota

The kernel code is 100% self-written. The only third-party assets are the vendored `limine.h` (BSD-2) and the Terminus bitmap glyphs (OFL 1.1) — together under 2% of the 25% quota, registered in [third_party/README.md](third_party/README.md).
