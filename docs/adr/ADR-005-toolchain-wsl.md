# ADR-005：工具链与运行环境 = WSL2 + GCC + Make + QEMU

- 状态：已接受（2026-11-15，用户拍板）
- 背景：宿主为 Windows（D:\Basalt）。候选：WSL2 / MSYS2 / Docker；编译器 GCC cross vs Clang/LLVM；构建 Make vs CMake。
- 决策：
  - 环境：**WSL2 (Ubuntu)** —— 文件经 /mnt/d/Basalt 直接操作工作区，与主流 OSDev 工具链兼容；
  - 编译器：GCC（宿主 gcc/g++，freestanding 内核编译；如后续需更严格可控，再议 x86_64-elf 交叉工具链，非本里程碑）；
  - 构建：**GNU Make**；镜像：xorriso（El Torito ISO）+ Limine 官方宿主工具 `limine bios-install`；
  - 运行：**QEMU**（`-M pc`，headless 用 `-serial file:` + `-display none` 做冒烟）。
- 安装清单（WSL 内，经用户批准后执行）：`build-essential`（gcc/g++/make/ld）、`xorriso`、`qemu-system-x86`、`git`（拉取 Limine 发布版）、`python3`（字模转换脚本）、`curl`/`unzip`（下载 Limine 二进制）。
- **补充（2026-11-15 实测）**：Debian 13 (trixie, WSL2) 已预装全部清单工具（gcc 14.2.0、g++、make 4.4.1、ld、xorriso、QEMU 10.0.13、python3、git、curl、unzip、xxd）——**零安装**，原清单无需执行。注意默认发行版是 `docker-desktop`（无 bash），所有命令须经 `wsl.exe -d Debian --` 指定发行版。
- 约束：一切构建与运行在模拟器/隔离环境；不写真实磁盘引导区、不修改宿主系统配置。
