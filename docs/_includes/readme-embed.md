# BasaltCore

[![ci](https://github.com/BS-O/Basalt/actions/workflows/ci.yml/badge.svg)](https://github.com/BS-O/Basalt/actions/workflows/ci.yml)

**玄武岩——地壳之下最坚实的那层石头。**

BasaltCore 是 **Basalt 系统的核心内核**：一个从裸机开始构建的自研 x86-64 内核（教学/研究项目）。它只是整个 Basalt 生态的地基——**未来将上线 Basalt 插件体系**（basalt plugin），内核本体保持小而坚实。

| | |
|---|---|
| 当前里程碑 | **M1**：PMM 内存管理 + 初代 Unix/Linux 风启动日志 + framebuffer shell + 插件机制 |
| 引导 | Limine Boot Protocol（v12.10.0） |
| 语言 | C++17 子集（子系统层）+ C（硬件层）+ GNU as（入口桩） |
| 运行 | WSL2 + QEMU，一切在模拟器中，不碰真实磁盘 |

## 快速开始

```bash
cd /mnt/d/Basalt     # WSL2 Debian
make && make iso     # 内核 ELF → 含示例插件的可引导 ISO
make run             # QEMU 窗口：启动日志 → bsh shell（键盘直接输入）
make check           # headless 冒烟：断言 COM1 出现 M1 启动日志
```

启动后你会看到初代 Linux 风格的启动横幅与逐行日志，随后进入 **bsh** shell：

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

shell 命令：`help` `ver` `clear` `echo` `meminfo` `plugins` `halt`。

## 插件体系（起步中）

- **启动时**与 **shell 启动时**都会报告已安装的插件数量与名字；
- 没有任何插件时，会提示：*“search 'basalt plugin' on GitHub (github.com/BS-O/Basalt)”*；
- 插件是经 Limine `module_path` 装载的 `BSPL` 格式二进制（`tools/bplugin.py` 生成示例）；
- 未来计划：插件仓库索引、插件间调用 ABI、`basalt plugin install` 生态。

## 文档

- [AGENTS.md](AGENTS.md) —— AI 编码工具入口（opencode/Pi/trae/zcode 等自动读取）
- [docs/AGENT-HANDBOOK.md](https://github.com/BS-O/Basalt/blob/main/docs/AGENT-HANDBOOK.md) —— 交接手册：完整背景、踩坑实录、路线图（后续开发者的主通道）
- [docs/PROJECT-GUIDE.md](https://github.com/BS-O/Basalt/blob/main/docs/PROJECT-GUIDE.md) —— 每个文件的用途与用法
- [docs/ARCHITECTURE.md](https://github.com/BS-O/Basalt/blob/main/docs/ARCHITECTURE.md) —— 分层架构与引导链路
- [docs/TASK-SPEC.md](https://github.com/BS-O/Basalt/blob/main/docs/TASK-SPEC.md) —— 任务书 v2（Limine 修订版）
- [docs/adr/](https://github.com/BS-O/Basalt/tree/main/docs/adr/) —— 决策记录 ADR-001~006
- 网站：<https://basalt.bsct.top/>（GitHub Pages）

## 许可与配额

内核代码 100% 自研；第三方仅 vendored 的 `limine.h`（BSD-2）与 Terminus 字模（OFL 1.1），合计 <2%，配额登记见 [third_party/README.md](https://github.com/BS-O/Basalt/blob/main/third_party/README.md)。
