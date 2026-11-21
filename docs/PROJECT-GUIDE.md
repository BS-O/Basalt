---
layout: page
title: Basalt 项目详细指南
doc_theme: true
page_lang: zh
pair_en: /PROJECT-GUIDE/en/
---

# Basalt 项目详细指南

> 覆盖仓库内**每一个文件**的用途、使用方法、修改指引与排障经验。
> 适用于 `main` 分支 M0 里程碑（Limine 引导 / framebuffer + 串口 hello world）。

| | |
|---|---|
| 文档版本 | v1.0（随 M0 提交） |
| 上游文档 | [ARCHITECTURE.md](https://github.com/BS-O/Basalt/blob/main/docs/ARCHITECTURE.md)（设计） · [TASK-SPEC.md](https://github.com/BS-O/Basalt/blob/main/docs/TASK-SPEC.md)（任务书 v2） · [adr/](https://github.com/BS-O/Basalt/tree/main/docs/adr)（决策记录） |
| 快速链接 | GitHub：<https://github.com/BS-O/Basalt> · 本页线上版见仓库 Pages |

---

## 1. 项目是什么

Basalt（玄武岩——地壳之下最坚实的那层石头）是一个**自研 x86-64 内核**的教学/研究项目。当前里程碑 **M0** 严格限定范围：**能启动 + 输出 hello world**，不做进程、文件系统、网络、驱动等任何完整内核功能。

引导链路一图流：

```
BIOS/UEFI → Limine 引导程序（固件初始化、16位→32位→64位长模式、页表、ELF 装载）
                │  跳入内核 ELF 入口（已是 64 位长模式）
                ▼
        boot/x86/entry64.S   串口探针 '1' → 切自建栈 → 探针 '2'
                │  rdi = framebuffer 请求指针
                ▼
        kernel/kmain.cpp     探针 'K' → console 初始化 → "hello world" → cli;hlt
                │                     │                    │
                ▼                     ▼                    ▼
        framebuffer 主显示     COM1 调试串口         make check 冒烟断言
```

串口日志里看到 `12Khello world` 即代表整条链路逐段验证通过。

## 2. 快速上手

### 2.1 环境要求

| 项 | 要求 |
|---|---|
| 宿主 | Windows 10/11 + WSL2 |
| 发行版 | **Debian**（注意：默认发行版可能是 docker-desktop，务必 `wsl.exe -d Debian`） |
| 工具 | `gcc`/`g++`、`make`、`xorriso`、`qemu-system-x86`、`python3`（Debian 13 全部预装，无需额外安装） |
| 网络 | 构建不需要网络；仅克隆仓库需要 |

验证环境的一条命令：

```bash
wsl.exe -d Debian -- bash -c "gcc --version | head -1; make --version | head -1; qemu-system-x86_64 --version | head -1; xorriso --version 2>/dev/null | head -1"
```

### 2.2 构建 / 运行 / 验证（三条命令）

```bash
# 进入仓库（Windows 侧路径映射）
cd /mnt/d/Basalt

make          # 1) 生成字模头文件  2) 编译链接内核 → build/basalt.elf
make iso      # 3) 编译 Limine 宿主工具  4) 组装 ISO  5) 写入 BIOS 引导记录
make run      # 打开 QEMU 窗口：左上角黑底白字 "hello world"
make check    # headless 冒烟：断言 COM1 出现 "hello world" → CHECK PASSED
```

### 2.3 常见问题

| 现象 | 处理 |
|---|---|
| `bash: not found` | 你连到了 docker-desktop 发行版，加 `-d Debian` |
| 改了 Makefile/链接脚本后行为没变 | `make clean` 后重来（Makefile 本身不在 .o 依赖里） |
| 直连 github.com 被重置 | 走本地代理：`git -c http.proxy=socks5h://127.0.0.1:1080 push ...`（按你实际代理端口调整） |
| `make check` 失败 | 见 §6.3 排障三板斧 |

## 3. 目录总览

```
Basalt/
├── Makefile              构建入口（目标详解见 §5.2）
├── limine.conf           引导菜单配置（Limine 启动时读取）
├── README.md             项目门面：快速上手三连
├── .gitignore            排除 build/legacy/vendored/生成物
├── boot/                 引导协议层（L0）
├── arch/                 硬件抽象层（L1）：x86 实装，arm64 占位
├── kernel/               内核主体（L3）：入口、console 抽象、mm 占位
├── lib/                  freestanding 基础库（L2）
├── third_party/          开源组件（配额登记 + vendored 原始包）
├── tools/                构建辅助与诊断脚本
├── docs/                 文档 + 本指南 + GitHub Pages 站点
└── legacy/               上一代 Multiboot2 骨架存档（不构建、不上传）
```

## 4. 文件逐个详解

### 4.1 根目录

| 文件 | 用途 | 何时修改 |
|---|---|---|
| `Makefile` | 全部构建逻辑。目标：`all`（内核 ELF）、`font`（字模生成）、`iso`、`run`、`check`、`clean` | 加新源文件时改 `OBJS` 与 `vpath`；调编译选项时改 `KCFLAGS` |
| `limine.conf` | 引导菜单：`timeout: 3`；唯一条目 `protocol: limine` + `path: boot():/boot/basalt.elf` + `resolution: 1024x768x32` | 改分辨率/超时/内核路径时；新增引导条目时 |
| `README.md` | 项目门面与快速上手 | 里程碑变化时更新状态 |
| `.gitignore` | 排除 `build/`、`legacy/`、`third_party/vendored/`、生成的字模头与宿主工具二进制、日志 | 新增生成物类型时 |

**Makefile 关键变量**：`KCFLAGS` 是全部内核编译选项（含义见 §5.3）；`LIMINE` 指向 vendored 宿主工具源码并现场编译；`FONT` 是生成字模头的路径，`fb.o` 显式依赖它（改字模源后 `make` 自动重新生成）。

**limine.conf 语法要点**（v12）：菜单条目以 `/` 开头的标题行开启；`path` 是 Limine 协议的键（`kernel_path` 为别名）；`boot():/boot/basalt.elf` 表示"引导卷上的 /boot/basalt.elf"；分辨率不可用时 Limine 自动回退。

### 4.2 boot/limine/ —— 引导协议层

| 文件 | 用途 |
|---|---|
| `requests.c` | **Basalt 与 Limine 的全部对话**。定义三组请求标记（详见下） |

`requests.c` 内含四个节（section），由链接脚本聚合进镜像前部：

1. `.limine_requests_start` / `.limine_requests_end`：协议标记数组，Limine 靠这对 magic 在 ELF 里圈出请求区；
2. **base revision 3**：声明"我说的是 v12 协议第 3 版"；启动后可用 `LIMINE_BASE_REVISION_SUPPORTED` 宏检验 Limine 是否接受；
3. **framebuffer 请求**：要一块图形输出；Limine 启动时把响应指针填进 `response` 字段（内核据此拿到显存）；
4. **entry point 请求**：指定入口为 `boot_entry`（entry64.S）。

**加新请求的方法**（例：M1 要内存图）：在 `limine.h` 里找到对应 `LIMINE_xxx_REQUEST_ID` 与 request 结构 → 在本文件仿照 framebuffer 请求添加一个 `volatile struct limine_xxx_request` → 内核里读 `->response`。就这三步。

### 4.3 boot/x86/ —— 入口桩

| 文件 | 用途 |
|---|---|
| `entry64.S` | Limine 跳入后的第一段 Basalt 代码（约 50 行） |

逐段解释：

- **探针 '1'**：直接轮询 COM1 LSR 端口(0x3FD)写 0x3F8——不依赖任何 C 代码，用于确认"CPU 确实进入了我们";
- `movabsq $boot_stack_top, %rsp`：**必须用 64 位绝对寻址**。内核链接在高半区 `0xffffffff80000000`，普通 `movq` 的 32 位符号数装不下；
- **探针 '2'**：自建 16KiB 栈（.bss，16 字节对齐）已就位；
- `lea framebuffer_request(%rip), %rdi`：SysV AMD64 ABI 第 1 参，把请求结构地址交给 kmain；
- `call kmain`：进入 C++，一去不返；
- `dbg_mark`：C 侧可调用的裸串口写字例程（M0 诊断用，约定见 §6.2）。

**boot/arm64/README.md**：占位说明（ADR-006），M0 无任何 arm64 实现。

### 4.4 arch/x86/ —— 硬件抽象层（HAL）

| 文件 | 用途 | 备注 |
|---|---|---|
| `io/io.h` | 端口 IO 内联汇编：`outb`/`inb`（x86 专属，arm64 将来给出自己的原语） | 头文件-only |
| `cpu/halt.h` | `basalt::halt()`（C++）与 `halt_forever()`（C）：`cli; hlt` 死循环——M0 的合法终态 | |
| `console/serial.h/.c` | 16550 UART 驱动，COM1=0x3F8，115200 8N1，轮询（无中断）。`serial_init()` 配置波特率/FIFO/线路；`serial_putc()` 带 THRE 超时保护 | 仅供调试与 CI（ADR-003） |
| `console/fb.h/.c` | framebuffer 文本控制台：`fb_init()` 校验 32bpp + RGB 内存模型并清屏；`fb_putc()` 按 8×16 字形逐位绘制；自动换行 + 逐行滚动（memmove 整屏上移一行再清底行）；格式不符则保持禁用 | 字形来自生成的 `terminus_font.h` |

**fb.c 的像素格式假设**：32bpp、`memory_model == 1`（Limine 的 RGB 线性模式）、每像素一个 `uint32_t`（0x00RRGGBB 白=0x00FFFFFF）。QEMU std VGA 满足；换真实硬件或别的模拟器若给 BGR/24bpp，`fb_init` 会安全禁用（串口仍可用，日志出现 `!` 标记）。

### 4.5 kernel/ —— 内核主体

| 文件 | 用途 |
|---|---|
| `kmain.cpp` | C++ 入口。`extern "C" void kmain(struct limine_framebuffer_request*)`：判空链（请求指针 → 响应 → framebuffer 数量）取第一块 framebuffer 填 `fb_info` → `console::init` → `write("hello world\n")` → `basalt::halt()`。探针 'K' 在函数第一行 |
| `console/console.h/.cpp` | **输出抽象层**（架构无关）。`fb_info` 结构刻意不含 limine 类型（console 层不知道引导协议）；`init()` 装配串口（必装）+ framebuffer（可选）；`putc()` 对 `'\n'` 特判：fb 走换行、串口补 `\r\n`；`write()` 逐字符泵 |
| `mm/pmm.h` | **占位接口**（M0 无实现）：`pmm_init/pmm_alloc_frame/pmm_free_frame` 签名已冻结，桩行为=空操作。M1 实现时消费 Limine memmap 请求 + HHDM |

### 4.6 lib/kstd/ —— freestanding 基础库

| 文件 | 用途 |
|---|---|
| `string.h/.c` | `memset/memcpy/memmove/strlen`。内核是 `-ffreestanding` 编译，编译器仍会生成对这四个符号的调用，必须自己提供（否则链接报 undefined）。实现为最朴素的逐字节循环，够用且无 UB |

### 4.7 third_party/ —— 开源组件（配额治理）

| 文件/目录 | 用途 | 许可证 |
|---|---|---|
| `README.md` | **配额登记表**：引入规则（先协商后引入、≤25%、逐项登记）+ 当前两项登记 | — |
| `limine/limine.h` | Limine Boot Protocol 的 ABI 头（v12.10.0，base revision 3）。内核与请求标记共用 | BSD-2（0BSD 型），`LICENSE` 同目录 |
| `limine/limine.c` + `Makefile` + `limine-bios-hdd.h` | **Limine 宿主工具**源码：`make iso` 时现场编译成 `limine` 可执行文件，用它执行 `bios-install`（把 BIOS 引导记录写进 ISO——**只写镜像文件，绝不碰真实磁盘**） | 同上 |
| `limine/limine-bios-cd.bin` | BIOS 光盘引导映像（ISO El Torito 启动用） | 同上 |
| `limine/limine-bios.sys` | BIOS stage 3 主体——**ISO 里必须有**，否则 PANIC "Stage 3 file not found"（M0 踩坑实录） | 同上 |
| `limine/limine-uefi-cd.bin` | UEFI 光盘引导映像（`--efi-boot` 挂载） | 同上 |
| `limine/BOOTX64.EFI` / `BOOTIA32.EFI` | UEFI 可执行（做 USB 启动盘时用，M0 未使用） | 同上 |
| `limine/limine-bios-pxe.bin` | PXE 网络引导映像（未使用） | 同上 |
| `limine/3RDPARTY.md` | Limine 自身的第三方组件清单 | — |
| `terminus/ter-u16n.bdf` | Terminus 8×16 常规体 BDF 字体源文件 | SIL OFL 1.1，`OFL.TXT` 同目录 |
| `terminus/terminus_font.h` | **生成文件**（不入 git）：`genfont.py` 把 BDF 转成 95×16 字节的字形数组 | 数据继承 OFL |
| `vendored/` | **不入 git**：带 SHA256 校验记录的原始发布包（limine-src / limine-binary / terminus） | — |

**升级 Limine 的方法**：下载新版 binary 包 → 核对 SHA256（GitHub release API 的 digest 字段）→ 更新 `vendored/` → 重跑 `tools/vendor-extract.sh` → `make clean && make && make iso` → `make check`。

### 4.8 tools/ —— 构建辅助与诊断

| 脚本 | 用途 | 何时使用 |
|---|---|---|
| `genfont.py` | BDF → C 头文件转换器。用法：`python3 tools/genfont.py <in.bdf> <out.h>`。内部按 FONTBOUNDINGBOX 的基线偏移把每字符对齐到 8×16 单元格 | 改字模/字号时（一般通过 `make font` 间接调用） |
| `vendor-extract.sh` | 从 vendored 包提取 `limine.h`、许可证、宿主工具、引导二进制、BDF 到 `third_party/` 各处 | 引入/升级组件时 |
| `build-and-check.sh` | `make clean → make → iso → check` 一条龙 + 打印串口日志 | 日常构建验证 |
| `debug-screendump.sh` | headless 启动 QEMU，6 秒后经监视器 `screendump` 抓帧 → 转 PNG 到 `build/screendump.png` | 黑屏/挂死时**看画面** |
| `debug-int.sh` | 同上但加 `-d int -D` 异常日志，输出尾部 60 行 | 定位**三重故障/异常向量** |
| `debug-regs.sh` | 经监视器 `info registers` 抓 CPU 状态 | 判断 CPU 卡在哪层（实模式=BIOS、长模式=内核） |

**诊断三板斧口诀**：先 `debug-screendump` 看画面文字（Limine panic 会写在屏幕上），再 `debug-regs` 看模式，最后 `debug-int` 抓异常。

### 4.9 docs/ —— 文档与 Pages 站点

| 文件 | 用途 |
|---|---|
| `ARCHITECTURE.md` | 架构设计 v1.1：分层、目录、引导链路、输出通道、**M0 踩坑记录** |
| `TASK-SPEC.md` | 任务书 v2：Multiboot2→Limine 的修订对照（删除线/加粗体） |
| `adr/ADR-001..006` | 六条已拍板决策：legacy 归档 / Limine 协议 / 输出通道 / Terminus 字模 / WSL 工具链 / arm64 占位 |
| `PROJECT-GUIDE.md` | **本文件** |
| `index.html`、`_layouts/`、`_config.yml` | GitHub Pages 站点（见 §7.3） |

### 4.12 独立站点仓库（极狐 GitLab Pages 镜像）

`D:\Basalt-site`（对应 jihulab 项目 `BS-O/basalt`）是本站的 GitLab Pages 适配版，与 GitHub `docs/` 同源。差异点：

- `_config.yml` 的 `baseurl`：GitHub 用仓库路径（自动），GitLab 侧为空串（唯一域名根路径服务）；
- `.gitlab-ci.yml`：ruby:3.2 镜像内 `gem install "jekyll:3.9.5" kramdown-parser-gfm`（GFM 输入必须显式装此 gem）→ `jekyll build -d public` → `public/` 声明为 artifacts；
- 链接改写：PROJECT-GUIDE 内相对链接已改为 GitHub 绝对链接（独立仓库里没有目标文件）。

**极狐 SaaS 实测限制（2026-11-19）**：Pages 站点访问级别实例级禁开 public（API 报"公开访问被禁用时，Pages 访问级别不能为公开"）；自定义域名管理 API 与 UI 均已下线（404）。站点因此登录可见，公开宣传以 GitHub Pages 为准；将来极狐放开后激活路径：`pages_access_level=public` + 界面加域名。

**GitLab CI 踩坑记录**：① script 行未加引号串内含 `: ` 会被 YAML 当映射，流水线 0 作业即 failed；② `kramdown: input: GFM` 在裸 ruby 镜像缺 `kramdown-parser-gfm`；③ `gem install` 多 gem 带版本必须用 `name:ver` 需求语法，不能 `-v`。

### 4.10 legacy/ —— 上一代存档

早期 Multiboot2/GRUB 路线的完整旧骨架（含构建产物）。**不参与构建、不入 git、不计任何配额**。仅在考古/对比时查看（ADR-001）。

### 4.11 不在 git 里、由构建生成的文件

| 路径 | 生成者 | 说明 |
|---|---|---|
| `build/*.o/.d` | make | 目标文件与依赖图 |
| `build/basalt.elf` | ld | 内核 ELF（高半区链接） |
| `build/iso_root/` | make iso | ISO 源目录 |
| `build/basalt.iso` | xorriso + limine bios-install | 可引导镜像 |
| `build/serial.log` | make check | COM1 日志 |
| `build/screendump.png` | debug 脚本 | 诊断截图 |
| `third_party/limine/limine` | make iso | 编译出的宿主工具 |
| `third_party/terminus/terminus_font.h` | genfont.py | 字形数组 |

## 5. 构建系统详解

### 5.1 数据流

```
ter-u16n.bdf ──genfont.py──▶ terminus_font.h ─┐
                                              ▼
requests.c entry64.S serial.c fb.c console.cpp kmain.cpp string.c
   └────────── gcc/g++ (-ffreestanding ...) ──▶ *.o ──ld (x86_64.ld)──▶ basalt.elf
                                                                      │
limine-bios-cd.bin + limine-uefi-cd.bin + limine.conf + basalt.elf    │
   └──────────────▶ xorriso（El Torito + EFI 目录）──▶ basalt.iso ◀───┘
                                       │
                        limine bios-install（写 BIOS 引导记录进 ISO）
```

### 5.2 Make 目标

| 目标 | 做什么 | 依赖 |
|---|---|---|
| `all`（默认） | 链接 `build/basalt.elf` | 全部 `.o` + 链接脚本 |
| `font` | 重新生成 `terminus_font.h` | BDF + genfont.py |
| `iso` | 编宿主工具 → 组 ISO → `bios-install` | ELF + limine.conf + 引导二进制 |
| `run` | QEMU 带窗口运行 ISO | iso |
| `check` | headless 冒烟：15s 超时，grep COM1 日志找 "hello world" | iso |
| `clean` | 删除 build/ 与生成的字模头 | — |

### 5.3 编译选项逐条理由（KCFLAGS）

| 选项 | 为什么 |
|---|---|
| `-ffreestanding -fno-builtin` | 无宿主环境；禁用内置函数假设（string 函数须自备） |
| `-fno-pic -fno-pie` | 内核自管布局，不要位置无关代码 |
| `-fno-stack-protector` | 没有 TLS/栈保护设施 |
| `-fno-asynchronous-unwind-tables` | 不要 .eh_frame（已链接丢弃） |
| `-mno-red-zone` | 内核态中断会踩红区，必须禁 |
| `-mno-mmx -mno-sse -mno-80387 -mgeneral-regs-only` | 未启用 SIMD/FPU 保存，防编译器偷偷生成 SSE |
| `-mcmodel=kernel` | 高半区（-2GiB）代码模型，配 `-fno-pic`；small 模型会产生 R_X86_64_32 截断重定位（M0 踩坑实录） |
| `-fno-exceptions -fno-rtti -fno-use-cxa-atexit`（C++） | 内核 C++ 子集三禁 |
| `-Wall -Wextra -O2 -g -MMD -MP` | 全警告 + 优化 + 调试信息 + 自动依赖头 |

链接脚本（`linker/x86_64.ld`）要点：`KERNEL_VMA=0xffffffff80000000`；三段 PHDRS（text RX / rodata R / data RW）**页对齐隔离**——Limine 禁止不同权限段共享 4K 页；`.limine_requests` 独立节用 `KEEP` 防丢弃。

## 6. 运行与验证

### 6.1 `make run` 预期

QEMU 窗口 1024×768，左上角黑底白字 `hello world`，然后画面静止——**这是正常的**（内核 `cli; hlt` 停机）。关窗即退。

### 6.2 `make check` 判定逻辑

`timeout 15 qemu ... -serial file:build/serial.log` → `grep -q "hello world"` → 打印 `CHECK PASSED`。串口日志含义：

| 字节 | 含义 |
|---|---|
| `1` | entry64.S 开头执行到（CPU 已进 Basalt） |
| `2` | 自建栈切换完成 |
| `K` | kmain 已进入（C++ 世界活了） |
| `!` | framebuffer 不可用（console 降级为仅串口） |

### 6.3 排障三板斧（headless）

```bash
bash tools/debug-screendump.sh   # 看 build/screendump.png：Limine panic 会写在屏上
bash tools/debug-regs.sh         # info registers：EIP 在 BIOS 区=回退了；长模式=内核层问题
bash tools/debug-int.sh          # -d int 日志：抓异常向量与出错 RIP
```

M0 实际用这三板斧定位过的三个问题（详见 ARCHITECTURE.md §3 踩坑记录）：limine-bios.sys 缺失、PHDR 低地址拒载、权限段共页 panic。

### 6.4 手动 QEMU 命令合集

```bash
# 带串口到终端
qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso -serial stdio -no-reboot

# 带监视器（Ctrl+Alt+2 切换）
qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso -monitor stdio

# GDB 调试（另一终端 gdb 后 target remote :1234）
qemu-system-x86_64 -M pc -m 256M -cdrom build/basalt.iso -s -S -display none
```

## 7. 架构与扩展点

### 7.1 分层与依赖规则

```
L3 kernel/（C++）   只准 include 自身 + L2 头
L2 lib/kstd/（C）   只准 include 自身
L1 arch/x86/（C）   只准 include 自身 + L0 暴露的 C ABI
L0 boot/（asm+C）   只准 include limine.h
跨语言边界一律 extern "C"
```

### 7.2 常见扩展怎么做

| 想做什么 | 步骤 |
|---|---|
| 加一个输出 sink（如 VGA 文本） | `arch/x86/console/` 新增实现 → `console.cpp` 的 `init/putc` 装配 → 头文件进 `console.h` 声明区 |
| 加一个 Limine 请求（M1 内存管理前置） | 见 §4.2 的三步法；memmap/hhdm 两个请求 ID 都在 `limine.h` |
| 实现 PMM（M1） | 在 `kernel/mm/` 落地 `pmm.h` 冻结的接口，数据源 = memmap 响应，虚拟地址换算 = HHDM offset |
| 引入新开源组件 | 先在 `third_party/README.md` 评估登记（用途/许可证/占比）→ 用户拍板 → 才落代码（25% 上限） |
| arm64 | 只动 `arch/arm64/`、`boot/arm64/`，内核主体不动（ADR-006） |

### 7.3 文档即站点（GitHub Pages）

`docs/` 同时是 Jekyll 站点根（`_config.yml` + `_layouts/default.html` + `index.html`）。仓库 Settings → Pages 选择 "Deploy from a branch: main /docs" 后，`https://bs-o.github.io/Basalt/` 即为本站：首页 `index.html`（项目介绍）+ `PROJECT-GUIDE.md`（渲染成本页网页）。

## 8. 约束与红线（务必遵守）

1. **一切构建与运行只在模拟器/隔离环境**；`limine bios-install` 只作用于生成的 ISO 文件，严禁对真实磁盘/引导区做任何写操作；
2. 里程碑范围纪律：M0 = 启动 + hello world。扩大范围先过方案协商；
3. 开源引入：先协商、后登记、再引入，25% 上限；
4. arm64 只占位不实现；
5. 凭据安全：token 不进仓库、不写进任何文件（用一次性 `git -c` 参数或凭据管理器）。

## 9. FAQ

| 问题 | 答案 |
|---|---|
| 链接时 RWX 段警告正常吗 | M0 的 PHDRS 已分段消除该警告；若再现属 freestanding 内核常见现象，先核对链接脚本分段 |
| 为什么是 `12K` 开头 | 三个探针字节，见 §6.2；它们是无害的调试遗产，M1 清理时移除 |
| 为什么 console 里没有 kprintf | M0 范围只到 write()；格式化打印留给 M1+（有意控制范围） |
| Limine 屏幕上 PANIC 了怎么办 | 拍照/截屏读错误文本，对照 §6.3 三板斧与 ARCHITECTURE.md 踩坑记录 |
| 为什么任务书从 Multiboot2 改成 Limine | Limine 现行版已移除 Multiboot2 支持；用户拍板修订任务书（ADR-002，修订对照见 TASK-SPEC.md） |
| 如何只重跑冒烟不改任何东西 | `make check`（ISO 已在则直接跑） |

## 10. 文档地图（查什么去哪份）

| 想了解 | 去看 |
|---|---|
| 每个文件怎么用、怎么改 | **本文**（PROJECT-GUIDE.md） |
| 架构设计、分层、踩坑记录 | docs/ARCHITECTURE.md |
| 任务书的条款与修订对照 | docs/TASK-SPEC.md |
| 某个决策为什么这么定 | docs/adr/ADR-001..006 |
| 开源配额与组件登记 | third_party/README.md |

### 4.13 M1 新增文件（2026-11-21，BasaltCore 叙事）

| 文件 | 用途 |
|---|---|
| `kernel/mm/pmm.c` | 位图 PMM：Limine memmap → 1bit/4KiB 帧，first-fit 分配，HHDM 偏移 |
| `kernel/lib/kprintf.cpp/.h` | 启动日志格式化（%s %c %d %u %x + kvprintf va_list 桥） |
| `arch/x86/io/kbd.c/.h` | PS/2 键盘轮询驱动（scancode set-1 → ASCII，Shift/CapsLock） |
| `kernel/shell/shell.c` | bsh shell（纯 C）：help/ver/clear/echo/meminfo/plugins/halt |
| `kernel/shell/shell_cxx.cpp` | C-ABI 桥：sh_* 符号把 C++ 子系统暴露给纯 C 的 shell |
| `kernel/plugins/plugins.c/.h` | BSPL 插件注册表：Limine module → 探测/登记/init 调用 |
| `tools/bplugin.py` | 生成示例 BSPL 插件（hello 0.1.0），ISO 携带演示 |
| `tools/m1-screendump.sh` | M1 framebuffer 截图诊断脚本 |
| `limine.conf` | 新增 `module_path` 挂载插件 |

**品牌叙事**：仓库即 **BasaltCore**（Basalt 系统的核心内核）。README 与架构文档已写入"未来上线 Basalt 插件生态（basalt plugin）"路线；启动日志与 shell 横幅在 0 插件时提示 *"search 'basalt plugin' on GitHub"*。
