# BasaltCore 操作系统架构设计

> Basalt(玄武岩)——地壳之下最坚实的那层石头。**BasaltCore 是 Basalt 系统的核心内核**，
> 未来将在其上生长出 Basalt 插件生态（basalt plugin）。C++/C 为主、汇编为辅，从裸机开始构建。

| | |
|---|---|
| 文档版本 | v2.0（M1：PMM/启动日志/键盘/shell/插件机制） |
| 目标架构 | x86-64 主体；arm64 仅占位（ADR-006） |
| 引导 | Limine Boot Protocol（ADR-002，任务书已修订，见 docs/TASK-SPEC.md） |
| 语言策略 | C++17 子集（子系统层） + C（硬件层） + 汇编（仅 64 位入口桩） |
| 状态 | ✅ M1：PMM 位图分配器 + 初代 Linux 风启动日志 + bsh shell + 插件机制（冒烟全绿） |

---

## 1. 总体分层

```
┌──────────────────────────────────────────────────────────┐
│ L3 子系统层【C++】  kmain / console 抽象 / (mm 占位接口)   │
├──────────────────────────────────────────────────────────┤
│ L2 基础库【C/C++】  lib/kstd (mem*、str*)、lib/cxx (占位)  │
├──────────────────────────────────────────────────────────┤
│ L1 HAL【C】        arch/x86: io端口 / 串口 / fb console    │
├──────────────────────────────────────────────────────────┤
│ L0 启动层【汇编】   boot/x86: Limine 请求标记 + 64位入口桩  │
│                    (固件初始化、模式切换、页表全部由        │
│                     Limine 完成 —— 见 ADR-002)             │
├──────────────────────────────────────────────────────────┤
│ 引导程序: Limine (BSD-2-Clause, 不计入25%配额口径之外的    │
│           部分: limine.h 头文件计入并登记, 见 ADR-002)      │
└──────────────────────────────────────────────────────────┘
```

分层规则：单向依赖；跨语言边界一律 `extern "C"`；内核态 C++ 关异常/RTTI/静态析构、禁 SSE/红区（`-mno-red-zone -mno-sse -mno-mmx -mno-80387`、`-ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie`）。

## 2. 目录结构

```
Basalt/
├── docs/                 ARCHITECTURE.md / TASK-SPEC.md / adr/（决策记录）
├── boot/                 引导协议层
│   ├── limine/           limine.h（vendored, BSD-2）+ boot.S 请求标记
│   ├── x86/              entry64.S（64 位入口桩：建栈→收 fb→调 kmain）
│   └── arm64/            【预留】仅 README
├── arch/
│   ├── x86/              cpu/（寄存器、halt） io/（端口） console/（fb、serial）
│   └── arm64/            【预留】仅 README
├── kernel/
│   ├── kmain.cpp         C++ 入口：extern "C" kmain(...)
│   ├── console/          输出抽象层（多 sink 装配）
│   └── mm/               【占位】仅接口头
├── lib/                  kstd/（mem*/str*） cxx/（运行支撑占位）
├── third_party/          开源接入点：README 登记配额与许可证
├── tools/                mkiso.sh / check.sh / 字模转换脚本
├── linker/x86_64.ld      链接脚本（VMA=LMA=1MiB）
├── Makefile              all / iso / run / check / clean
└── README.md             构建与运行说明
```

## 3. 引导链路（M0 实况）

```
BIOS/UEFI → Limine 引导（其内部完成 16位→保护模式→长模式、页表、ELF 装载）
   │  跳入内核 ELF 入口时：已是 64 位长模式，中断关闭，栈可用（但 Basalt 自建）
   ▼
boot/x86/entry64.S  _start（汇编约 30 行）
   │  ① mov rsp, boot_stack_top（.bss 内 16KiB, 16 字节对齐）
   │  ② 收集 framebuffer 请求响应（指针存 .limine_requests 静态结构）
   │  ③ mov rdi, <framebuffer 响应指针>; call kmain
   ▼
kernel/kmain.cpp（C++）
   │  console::init(fb, serial) → 写 "hello world" 到 fb + 串口
   │  → arch::halt()（cli; hlt 循环）
```

要点：
- **Basalt 不再自研 16 位实模式/保护模式/长模式切换代码**——该链路由 Limine 完成（ADR-002）。`boot/` 只含 Limine 请求标记与 64 位入口桩。
- Limine 请求标记：位于 `.limine_requests` 节，由链接脚本保证位于镜像前部。本里程碑请求三个：**base revision 3**、**framebuffer**、**entry point**。
- `limine.h` 从 Limine 官方发布 vendored 到 `third_party/limine/`，登记入 third_party（BSD-2-Clause）。
- 地址布局：内核按 **高半区 VMA=0xffffffff80000000** 链接（Limine 协议硬性要求"PHDR 必须在高半区"，物理存放位置由 Limine 决定并完成映射），代码模型 `-mcmodel=kernel`；Limine 同时提供 HHDM 高半区直映射（基址经 hhdm 请求取得），供后续 MM 阶段使用——M0 不用。
- M0 不建 IDT、不开中断；串口初始化仅轮询。

### M0 实测踩坑记录（2026-11-15，供后续里程碑参考）

1. **内核 PHDR 必须在高半区**：低地址链接会被 Limine 拒载（`Lower half PHDRs are not allowed`）；
2. **不同权限的 PHDR 不得共享同一 4K 页**：`.limine_requests`（只读）与 `.text`（可执行）页对齐隔离，否则 `PHDRs with different permissions sharing the same memory page` panic；
3. 高半区 + `-mcmodel=kernel` 配套（small 模型会产生 R_X86_64_32 截断重定位）；入口桩取 `.bss` 高地址须用 `movabsq`；
4. ISO 内必须带 `limine-bios.sys`（BIOS stage 3），否则 BIOS 路径 PANIC；
5. 调试手段：QEMU 监视器 `screendump` + `-d int` 异常日志 + COM1 探针字节（入口桩 '1'/'2'、kmain 'K'）三板斧，headless 全程可定位。

## 4. 输出通道（ADR-003）

| 通道 | 实现 | 用途 |
|---|---|---|
| framebuffer（主显示） | Limine framebuffer 请求 → 32bpp RGB/BGR 线性帧，写显存画 8×16 字形 | QEMU 窗口直接可见 "hello world" |
| 串口 COM1（调试） | 16550 轮询（0x3F8），仅调试与 CI | `make check` headless 断言 |

抽象层（架构无关，arm64 将来提供自己的 sink）：

```cpp
// kernel/console/console.h
namespace basalt::console {
    void init(const void* fb_response);  // fb 未就绪时仅串口可用
    void putc(char c);
    void write(const char* s);
}
```

字模：Terminus 8×16（SIL OFL 1.1，ADR-004），由 `tools/` 脚本转成 C 数组，登记入 third_party。

## 5. 构建 / 运行 / 验证

- 工具链：WSL2 + GCC（宿主 gcc/g++，freestanding 模式）+ GNU Make（ADR-005）。
- 镜像：`xorriso` 生成 El Torito ISO（含 Limine BIOS/UEFI 引导文件 + `limine.conf` + 内核），再用 Limine 官方 `limine` 宿主工具 `bios-install` 写入 BIOS 引导记录。
- 模拟器：QEMU（`-M pc -m 256M`），可 `-serial stdio` 看调试串口。
- 验证：`make check` headless 跑 QEMU，断言串口日志出现 `hello world`；`make run` 开窗口目视 framebuffer 上的 "hello world"。

## 6. 扩展接口（任务书要求）

| 接入点 | 现状 |
|---|---|
| 输出抽象层 | `kernel/console/`，多 sink 装配 ✅ |
| 内存管理占位 | `kernel/mm/pmm.h` 仅接口声明，不实现 ✅ |
| 架构抽象 | `arch/x86` 实装，`arch/arm64` 目录+README+编译桩 ✅ |
| 开源组件接入 | `third_party/`（配额规则+许可证登记） ✅ |
| 引导协议 | `boot/limine`（Limine 协议）；将来加协议 = 新增 `boot/<proto>/` 目录 |

## 7. 决策记录

所有已拍板决策见 `docs/adr/`（ADR-001 ~ ADR-006）；任务书修订全文见 `docs/TASK-SPEC.md`。

## 8. M1 新增子系统（2026-11-21）

| 子系统 | 位置 | 说明 |
|---|---|---|
| PMM 物理内存管理 | `kernel/mm/pmm.c` | Limine memmap 位图分配器（1 bit/4KiB 帧，first-fit 扫描），HHDM 偏移来自 Limine |
| 启动日志 | `kernel/kmain.cpp` | 初代 Linux 风逐行日志（无时间戳）：fb0/ttyS0/mem/i8042/plugins |
| kprintf | `kernel/lib/kprintf.cpp` | %s %c %d %u %x 子集 + kvprintf(va_list) C-ABI 桥 |
| PS/2 键盘 | `arch/x86/io/kbd.c` | 轮询式 scancode set-1 → ASCII（含 Shift/CapsLock） |
| bsh shell | `kernel/shell/` | framebuffer 交互 shell：help/ver/clear/echo/meminfo/plugins/halt；C-ABI 桥（shell_cxx.cpp）连接 C++ 子系统 |
| 插件机制 | `kernel/plugins/` | Limine module 装载的 BSPL 格式插件；启动与 shell 横幅均报告插件数，0 个时提示去 GitHub 搜 'basalt plugin' |

**插件格式（BSPL v1）**：`magic 'BSPL' + api(1) + init 指针 + name/version 偏移`；`tools/bplugin.py` 生成示例插件，`limine.conf` 的 `module_path` 挂载。

**未来**（Basalt 插件生态）：插件仓库索引、`basalt plugin install`、插件间调用 ABI —— 见 README「插件体系」。
