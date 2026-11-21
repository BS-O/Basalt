---
layout: page
title: Basalt Project Guide (English)
permalink: /PROJECT-GUIDE/en/
doc_theme: true
page_lang: en
pair_zh: /PROJECT-GUIDE/
---

> 中文原文：[PROJECT-GUIDE.md](https://basalt.bsct.top/PROJECT-GUIDE/)

# Basalt Project Guide

> Purpose, usage, modification guide and troubleshooting experience for **every file** in the repository. Applies to milestone M1 on the `main` branch (Limine boot / bitmap PMM / bsh shell / plugin mechanism).

| | |
|---|---|
| Doc version | v1.1 (M1, mirrors the Chinese guide) |
| Upstream docs | [ARCHITECTURE.md](https://github.com/BS-O/Basalt/blob/main/docs/ARCHITECTURE.md) · [TASK-SPEC.md](https://github.com/BS-O/Basalt/blob/main/docs/TASK-SPEC.md) · [adr/](https://github.com/BS-O/Basalt/tree/main/docs/adr) |
| Quick links | GitHub: <https://github.com/BS-O/Basalt> · this page online: the site above |

---

## 1. What the project is

Basalt (玄武岩 — the strongest layer of stone beneath the earth's crust) is a **self-built x86-64 kernel** for education/research. Milestone **M1** extends M0 ("boot + hello world") with: a bitmap physical-memory manager, an early-Unix-style boot log, a PS/2 keyboard, the `bsh` framebuffer shell, and the BSPL plugin mechanism. The repository is **BasaltCore** — the kernel core of the future Basalt plugin ecosystem.

Boot chain at a glance:

```
BIOS/UEFI → Limine bootloader (firmware init, 16→32→64-bit long mode, paging, ELF load)
                │ jumps into the kernel ELF entry (already long mode)
                ▼
        boot/x86/entry64.S   serial probe '1' → switch to own stack → probe '2'
                │ rdi = framebuffer request pointer
                ▼
        kernel/kmain.cpp     probe 'K' → boot log → PMM → plugins → shell (never returns)
```

Seeing `12K` followed by the boot log on COM1 means every stage of the chain ran.

## 2. Quick start

### 2.1 Environment

| Item | Requirement |
|---|---|
| Host | Windows 10/11 + WSL2 |
| Distro | **Debian** (note: the default distro may be docker-desktop — always `wsl.exe -d Debian`) |
| Tools | `gcc`/`g++`, `make`, `xorriso`, `qemu-system-x86`, `python3` (all preinstalled on Debian 13) |
| Network | Not needed for building; only for cloning |

### 2.2 Build / run / verify

```bash
cd /mnt/d/Basalt
make          # font header + kernel ELF (build/basalt.elf)
make iso      # bootable ISO incl. the sample hello plugin
make run      # QEMU window: boot log then bsh shell (type on the keyboard)
make check    # headless smoke test: asserts the M1 boot log on COM1
```

### 2.3 Common issues

| Symptom | Fix |
|---|---|
| `bash: not found` | You landed in the docker-desktop distro; add `-d Debian` |
| Edits to Makefile/linker script seem ignored | `make clean` first (the Makefile is not a dependency of .o files) |
| github.com connection reset | Use a local proxy: `git -c http.proxy=socks5h://127.0.0.1:1080 push ...` |
| `make check` fails | See §6.3 troubleshooting triad |

## 3. Directory overview

```
Basalt/
├── Makefile              build entry (targets in §5.2)
├── limine.conf           boot menu config (read by Limine, mounts the plugin module)
├── README.md / README-EN.md  repo front page (zh / en)
├── boot/                 boot protocol layer (L0)
│   ├── limine/           request markers (base rev + framebuffer + memmap + hhdm + module + entry point)
│   ├── x86/              entry64.S 64-bit entry stub
│   └── arm64/            placeholder README only
├── arch/                 hardware abstraction (L1): x86 real, arm64 placeholder
├── kernel/               kernel body (L3): kmain, console, kprintf, mm, shell, plugins
├── lib/                  freestanding base library (L2)
├── third_party/          open-source components (quota registry + vendored archives)
├── tools/                build helpers and diagnostics scripts
├── docs/                 documentation + GitHub Pages site
└── legacy/               previous-generation Multiboot2 skeleton (not built, not in git)
```

## 4. File-by-file reference

### 4.1 Root

| File | Purpose | When to modify |
|---|---|---|
| `Makefile` | All build logic. Targets: `all`, `font`, `iso`, `run`, `check`, `clean`, `site` | Adding sources: update `OBJS`/`vpath`; flags: `KCFLAGS` |
| `limine.conf` | Boot menu: `timeout: 3`; entry `protocol: limine`, `path: boot():/boot/basalt.elf`, `module_path: boot():/boot/plugins/hello.bpl`, `resolution: 1024x768x32` | Resolution/timeout/plugin list |
| `README.md` / `README-EN.md` | Repo front page (Chinese / English); the site embeds both via `make site` | Milestone changes |
| `.gitignore` | Excludes `build/`, `legacy/`, `third_party/vendored/`, generated font header and host tool, logs | New generated artifacts |

### 4.2 boot/limine/ — boot protocol layer

`requests.c` is **the entire conversation with Limine**. It places request structures into the `.limine_requests` section via the linker script:

1. start/end marker arrays — Limine locates the request block by these magic pairs;
2. **base revision 3** — declares protocol revision; checkable with `LIMINE_BASE_REVISION_SUPPORTED`;
3. **framebuffer** request — Limine fills `->response` at boot;
4. **memmap** request — the physical memory map consumed by the PMM (§4.5);
5. **hhdm** request — the higher-half direct map offset;
6. **module** request — the plugin list loaded via `module_path` (§4.5 plugins);
7. **entry point** request — points at `boot_entry`.

**Adding a request**: find `LIMINE_xxx_REQUEST_ID` in `limine.h` → add a `volatile struct limine_xxx_request` here → read `->response` in the kernel. Three steps.

### 4.3 boot/x86/ — entry stub

`entry64.S` (~50 lines) is the first Basalt code executed:

- **probe '1'**: raw COM1 write (poll 0x3FD, write 0x3F8) — no C dependency, proves the CPU reached us;
- `movabsq $boot_stack_top, %rsp`: **64-bit absolute addressing required** — the kernel links at `0xffffffff80000000`, too big for a 32-bit `movq` displacement;
- **probe '2'**: own 16 KiB stack (.bss, 16-byte aligned) is live;
- `lea framebuffer_request(%rip), %rdi`: SysV AMD64 ABI first argument = the request struct address;
- `call kmain`: enters C++, never returns;
- `dbg_mark`: raw serial byte writer callable from C (diagnostics).

`boot/arm64/README.md`: placeholder (ADR-006); no arm64 implementation.

### 4.4 arch/x86/ — HAL

| File | Purpose | Notes |
|---|---|---|
| `io/io.h` | Inline-asm port I/O: `outb`/`inb` (x86-specific; arm64 will provide its own primitives) | header-only |
| `io/kbd.c/.h` | PS/2 keyboard, polled scancode set-1 → ASCII (Shift/CapsLock; Enter=10, BS=8, Tab=9); `kbd_init()` drains the controller | used by the shell |
| `cpu/halt.h` | `basalt::halt()` (C++) / `halt_forever()` (C): `cli; hlt` loop — the legal terminal state | |
| `console/serial.h/.c` | 16550 UART, COM1=0x3F8, 115200 8N1, polled. `serial_init()` sets baud/FIFO/line; `serial_putc()` has a THRE timeout guard | debug/CI only (ADR-003) |
| `console/fb.h/.c` | Framebuffer text console: `fb_init()` validates 32bpp + RGB model and clears; `fb_putc()` draws 8×16 glyphs bit by bit; auto-wrap + line scroll; `fb_clear()` wipes | glyphs from the generated `terminus_font.h` |

**fb.c pixel-format assumption**: 32bpp, `memory_model == 1` (Limine RGB linear), one `uint32_t` per pixel (0x00RRGGBB, white = 0x00FFFFFF). QEMU std VGA satisfies it; on BGR/24bpp hardware `fb_init` disables the sink safely (serial still works; a `!` marker appears in the log).

### 4.5 kernel/ — kernel body

| File | Purpose |
|---|---|
| `kmain.cpp` | C++ entry `extern "C" void kmain(limine_framebuffer_request*)`: serial bootstrap → banner → framebuffer → memmap/PMM → plugins → `shell_run()` (never returns) |
| `console/console.h/.cpp` | **Output abstraction** (arch-independent). `fb_info` deliberately carries no limine types; `init()` assembles serial (always) + fb (optional); `putc()` maps `'\n'` → fb newline + serial `\r\n` |
| `lib/kprintf.cpp/.h` | printf subset for the boot log: `%s %c %d %u %x %%` + `kvprintf` (va_list) for the C-ABI bridge |
| `mm/pmm.h/.c` | **Bitmap PMM**: 1 bit per 4KiB frame over the Limine memmap; first-fit allocation; HHDM offset stored; `pmm_alloc_frame/pmm_free_frame/pmm_total_frames/pmm_used_frames` |
| `shell/shell.c` | `bsh` (pure C): help/ver/clear/echo/meminfo/plugins/halt; line editing: printable chars, backspace, Enter; banner reports installed plugins or the GitHub hint |
| `shell/shell_cxx.cpp` | C-ABI bridge: `sh_*` symbols expose the C++ subsystems (console/kprintf/halt) to the pure-C shell |
| `plugins/plugins.h/.c` | **BSPL registry**: scans Limine modules for `magic 'BSPL' + api + init + name/version offsets`; registers up to 16; `plugins_boot()` calls each init; `plugin_name(i)` resolves names relative to the module base |

**BSPL plugin format (v1)**: flat blob, header `struct bplugin_header { magic=0x4253504C; api=1; init; name_off; version_off; }`, strings at module-relative offsets. `tools/bplugin.py` emits samples.

### 4.6 lib/kstd/ — freestanding base library

| File | Purpose |
|---|---|
| `string.h/.c` | `memset/memcpy/memmove/strlen`. The compiler emits calls to these even under `-ffreestanding`, so they must exist; plain byte loops, no UB |

### 4.7 third_party/ — open-source components (quota governance)

| File/dir | Purpose | License |
|---|---|---|
| `README.md` | **Quota registry**: rules (negotiate first, ≤25%, per-item registration) + current entries | — |
| `limine/limine.h` | Limine Boot Protocol ABI header (v12.10.0, base rev 3) | BSD-2 (0BSD-style), `LICENSE` alongside |
| `limine/limine.c` + `Makefile` + `limine-bios-hdd.h` | **Host tool** source: compiled on the fly by `make iso`; used for `bios-install` (writes the BIOS boot record **into the ISO file only — never a real disk**) | same |
| `limine/limine-bios-cd.bin` | BIOS El Torito boot image for the ISO | same |
| `limine/limine-bios.sys` | BIOS stage 3 — **must be in the ISO** or boot PANICs "Stage 3 file not found" (M0 field record) | same |
| `limine/limine-uefi-cd.bin`, `BOOTX64.EFI`, `BOOTIA32.EFI` | UEFI boot images/executables (ISO EFI catalog; USB use later) | same |
| `terminus/ter-u16n.bdf` | Terminus 8×16 regular BDF source | SIL OFL 1.1, `OFL.TXT` alongside |
| `terminus/terminus_font.h` | **Generated** (not in git): 95×16-byte glyph array from `genfont.py` | data inherits OFL |
| `vendored/` | **Not in git**: original release archives with SHA256 records | — |

**Upgrading Limine**: download the new binary tarball → verify SHA256 (the `digest` field of the GitHub release API) → refresh `vendored/` → rerun `tools/vendor-extract.sh` → `make clean && make && make iso` → `make check`.

### 4.8 tools/ — build helpers and diagnostics

| Script | Purpose | When |
|---|---|---|
| `genfont.py` | BDF → C header. `python3 tools/genfont.py <in.bdf> <out.h>`; aligns each glyph to the 8×16 cell via FONTBOUNDINGBOX baseline | font changes (usually via `make font`) |
| `vendor-extract.sh` | Extracts `limine.h`, licenses, host tool, boot binaries, BDF from vendored archives | component upgrades |
| `build-and-check.sh` | `make clean → make → iso → check` in one shot | daily verification |
| `bplugin.py` | Emits a sample BSPL plugin blob (`hello 0.1.0`), copied into the ISO by `make iso` | plugin experiments |
| `m1-screendump.sh` | Boots QEMU headless, screendump at t=6s, converts to `build/m1-boot.png` | visual verification |
| `debug-screendump.sh` | Same idea, generic | black screen / hang — **look at the picture** |
| `debug-int.sh` | Adds `-d int -D` exception logging | triple faults / exception vectors |
| `debug-regs.sh` | `info registers` via the monitor | which layer is the CPU stuck in (real mode = BIOS, long mode = kernel) |

**Triad mantra**: `debug-screendump` first (Limine panics print to screen), then `debug-regs` for the mode, then `debug-int` for the vector.

### 4.9 docs/ — documentation and Pages site

| File | Purpose |
|---|---|
| `ARCHITECTURE.md` | Layering, boot chain, M0 field records, M1 subsystems (v2.0) |
| `TASK-SPEC.md` | Task spec v2: the Multiboot2→Limine revision diff |
| `adr/ADR-001..006` | Six accepted decisions |
| `PROJECT-GUIDE.md` / `PROJECT-GUIDE-EN.md` | This guide (zh / en) |
| `index.html`, `_layouts/`, `_config.yml`, `_includes/` | GitHub Pages site (see §7.3) |

### 4.10 legacy/

The previous-generation Multiboot2/GRUB skeleton (with build artifacts). **Not built, not in git, not counted in quota.** For archaeology only (ADR-001).

### 4.11 Generated files (not in git)

| Path | Producer | Notes |
|---|---|---|
| `build/*.o/.d` | make | objects + dep files |
| `build/basalt.elf` | ld | kernel ELF (high-half linked) |
| `build/plugin-hello.bpl` | bplugin.py | sample plugin module |
| `build/basalt.iso` | xorriso + limine bios-install | bootable image |
| `build/serial.log` | make check | COM1 log |
| `build/m1-boot.png` | m1-screendump.sh | visual evidence |
| `third_party/limine/limine` | make iso | compiled host tool |
| `third_party/terminus/terminus_font.h` | genfont.py | glyph array |

### 4.12 Standalone site repo (Jihulab GitLab Pages mirror)

`D:\Basalt-site` (Jihulab project `BS-O/basalt`) mirrors the GitHub `docs/` site. Differences:

- `_config.yml` `baseurl`: GitHub auto-derives from the repo path; the GitLab side uses `""` (unique-domain root serving);
- `.gitlab-ci.yml`: `ruby:3.2` image, `gem install "jekyll:3.9.5" kramdown-parser-gfm` (GFM input needs this gem explicitly) → `jekyll build -d public` → `public/` as artifacts;
- relative doc links are rewritten to absolute GitHub URLs (the standalone repo has no sibling files).

**Jihulab SaaS limits (measured 2026-11-19)**: instance policy forbids public Pages access level; custom-domain API/UI removed (404). The mirror is login-gated; public promotion uses GitHub Pages.

**GitLab CI field records**: ① an unquoted script string containing `: ` is parsed as a YAML mapping — pipeline fails with 0 jobs; ② bare ruby images lack `kramdown-parser-gfm`; ③ multi-gem `gem install` needs `name:ver` requirement syntax, not `-v`.

## 5. Build system

### 5.1 Data flow

```
ter-u16n.bdf ──genfont.py──▶ terminus_font.h ─┐
                                              ▼
requests.c entry64.S serial.c fb.c kbd.c console.cpp kprintf.cpp string.c
pmm.c plugins.c shell.c shell_cxx.cpp kmain.cpp
   └──── gcc/g++ (-ffreestanding ...) ────▶ *.o ──ld (x86_64.ld)──▶ basalt.elf
bplugin.py ──▶ hello.bpl ─────────────────────────────────┐
limine-bios-cd.bin + limine-bios.sys + limine-uefi-cd.bin │
limine.conf + basalt.elf + hello.bpl ──▶ xorriso ──▶ basalt.iso ──▶ limine bios-install
```

### 5.2 Make targets

| Target | What it does | Depends on |
|---|---|---|
| `all` (default) | link `build/basalt.elf` | all `.o` + linker script |
| `font` | regenerate `terminus_font.h` | BDF + genfont.py |
| `iso` | build host tool → assemble ISO (incl. plugin) → `bios-install` | ELF + limine.conf + boot binaries |
| `run` | QEMU with window | iso |
| `check` | headless smoke: 15s timeout, grep COM1 for the M1 boot log | iso |
| `site` | sync README.md/README-EN.md into the site embeds | — |
| `clean` | remove build/ and the generated font header | — |

### 5.3 Compile flags, one by one (KCFLAGS)

| Flag | Why |
|---|---|
| `-ffreestanding -fno-builtin` | no host environment; string functions must be self-provided |
| `-fno-pic -fno-pie` | the kernel owns its layout |
| `-fno-stack-protector` | no TLS/stack-guard infrastructure |
| `-fno-asynchronous-unwind-tables` | no .eh_frame (discarded at link) |
| `-mno-red-zone` | kernel-mode interrupts would clobber the red zone |
| `-mno-mmx -mno-sse -mno-80387 -mgeneral-regs-only` | SIMD/FPU state is never saved; forbid the compiler emitting them |
| `-mcmodel=kernel` | high-half (-2GiB) code model; `small` produces truncated R_X86_64_32 relocs (M0 field record) |
| `-fno-exceptions -fno-rtti -fno-use-cxa-atexit` (C++) | the kernel C++ subset |
| `-Wall -Wextra -O2 -g -MMD -MP` | full warnings, optimization, debug info, auto headers |

Linker script (`linker/x86_64.ld`): `KERNEL_VMA=0xffffffff80000000`; three PHDRS (text RX / rodata R / data RW) **page-aligned apart** — Limine forbids different-permission segments sharing a 4K page; `.limine_requests` is `KEEP`-ed in its own section.

## 6. Run and verify

### 6.1 `make run` expected

QEMU 1024×768: banner + boot log, then the `bsh` prompt — type on the keyboard (`help`, `meminfo`, `plugins`…). Closing the window exits.

### 6.2 `make check` logic

`timeout 15 qemu ... -serial file:build/serial.log` → grep for `BasaltCore is ready` + `plugins` → `CHECK PASSED`. Serial log bytes:

| Byte | Meaning |
|---|---|
| `1` | entry64.S executed (CPU reached Basalt) |
| `2` | own stack switched |
| `K` | kmain entered (C++ world alive) |
| `!` | framebuffer unavailable (console degraded to serial only) |

### 6.3 Troubleshooting triad (headless)

```bash
bash tools/debug-screendump.sh   # read build/screendump.png: Limine panics print on screen
bash tools/debug-regs.sh         # info registers: EIP in BIOS = fell back; long mode = kernel layer
bash tools/debug-int.sh          # -d int log: exception vectors and faulting RIP
```

Three real M0 faults located this way (see ARCHITECTURE.md §3): missing limine-bios.sys, low-half PHDR rejection, shared-page permission panic.

### 6.4 Manual QEMU commands

```bash
# serial to terminal
qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso -serial stdio -no-reboot

# with monitor
qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso -monitor stdio

# GDB (in another shell: gdb build/basalt.elf, target remote :1234)
qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso -s -S -display none
```

## 7. Architecture and extension points

### 7.1 Layering and dependency rules

```
L3 kernel/ (C++)   may include itself + L2 headers
L2 lib/kstd/ (C)   may include itself
L1 arch/x86/ (C)   may include itself + C ABI exposed by L0
L0 boot/ (asm+C)   may include limine.h only
extern "C" at every language boundary
```

### 7.2 Common extensions

| Goal | Steps |
|---|---|
| New output sink (e.g. VGA text) | implement in `arch/x86/console/` → wire in `console.cpp` init/putc → declare in `console.h` |
| New Limine request | three-step recipe in §4.2; IDs live in `limine.h` |
| Virtual memory (M2) | page tables via PMM frames + HHDM translation; `map_page` style API |
| New plugin | `python3 tools/bplugin.py <name> <ver> x.bpl` → add `module_path` in limine.conf → `make iso` |
| New open-source component | register in `third_party/README.md` (purpose/license/share) → user approval → then code (25% cap) |
| arm64 | touch only `arch/arm64/`, `boot/arm64/`; kernel body unchanged (ADR-006) |

### 7.3 Docs-as-site (GitHub Pages)

`docs/` doubles as the Jekyll site root. Pages deploys "main branch /docs". The site carries: home (promo), `/PROJECT-GUIDE/` (zh) + `/PROJECT-GUIDE/en/`, `/readme/` (zh) + `/readme/en/` — the top-right 中/EN switch flips languages and remembers the choice.

## 8. Constraints and red lines

1. **Build and run only in the emulator/sandbox**; `limine bios-install` writes into the generated ISO file, never a real disk or boot sector;
2. Milestone scope discipline: expanding scope requires a plan negotiation first;
3. Open-source intake: negotiate → register → introduce; 25% cap;
4. arm64 stays a placeholder;
5. Credential hygiene: no tokens in the repo or any file (one-shot `git -c` args or a credential manager).

## 9. FAQ

| Question | Answer |
|---|---|
| Is the RWX-segment link warning normal? | The PHDRS split removed it in M0; if it reappears, check the linker-script segmentation |
| Why does the log start with `12K`? | Three probe bytes (§6.2); harmless debug heritage kept for diagnostics |
| Why does the plugin hint mention GitHub? | With 0 plugins the kernel/shell prints "search 'basalt plugin' on GitHub" — the ecosystem entry point |
| Limine PANIC on screen? | Screenshot the message, run the §6.3 triad, compare with ARCHITECTURE.md field records |
| Why did the task spec move from Multiboot2 to Limine? | Current Limine dropped Multiboot2; the user approved the spec revision (ADR-002, diff in TASK-SPEC.md) |
| Just rerun the smoke test | `make check` (reuses the existing ISO) |

## 10. Documentation map

| Want to know | Go to |
|---|---|
| What each file does and how to change it | **this guide** (zh / en here) |
| Architecture, layering, field records | docs/ARCHITECTURE.md |
| Task spec and its revision | docs/TASK-SPEC.md |
| Why a decision was made | docs/adr/ADR-001..006 |
| Open-source quota registry | third_party/README.md |

### 4.13 M1 new files (2026-11-21, BasaltCore narrative)

| File | Purpose |
|---|---|
| `kernel/mm/pmm.c` | Bitmap PMM: Limine memmap → 1 bit/4KiB frame, first-fit, HHDM offset |
| `kernel/lib/kprintf.cpp/.h` | Boot-log formatting (%s %c %d %u %x + kvprintf va_list bridge) |
| `arch/x86/io/kbd.c/.h` | PS/2 polled keyboard (scancode set-1 → ASCII, Shift/CapsLock) |
| `kernel/shell/shell.c` | bsh shell (pure C): help/ver/clear/echo/meminfo/plugins/halt |
| `kernel/shell/shell_cxx.cpp` | C-ABI bridge exposing C++ subsystems as sh_* symbols |
| `kernel/plugins/plugins.c/.h` | BSPL registry: Limine modules → detect/register/init |
| `tools/bplugin.py` | Emits the sample BSPL plugin shipped in the ISO |
| `tools/m1-screendump.sh` | M1 framebuffer screenshot script |
| `limine.conf` | gained `module_path` for the plugin |

**Brand narrative**: the repository is **BasaltCore** — the kernel core of the Basalt system. README and ARCHITECTURE carry the "basalt plugin ecosystem coming" roadmap; the boot log and shell banner print the GitHub search hint when no plugin is installed.
