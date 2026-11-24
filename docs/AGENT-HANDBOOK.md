---
layout: page
title: Basalt 交接手册（Agent Handbook）
doc_theme: true
page_lang: zh
---

# Basalt / BasaltCore 交接手册（Agent Handbook）

> **写给谁**：所有将参与 Basalt 后续开发的 AI 编码工具（opencode、Pi、trae、zcode、Codex、以及任何遵循 AGENTS.md 约定的 agent），以及新加入的人类协作者。
> **怎么用**：先读仓库根目录的 `AGENTS.md`（30 秒速览 + 十大坑），再通读本文。读完你应当能够：不破坏任何东西地完成一次构建/冒烟/部署；理解每个历史决策的来龙去脉；避开前人踩过的全部坑；明确下一步该做什么、怎么做。
> **最后更新**：2026-11-25 · 内核里程碑 M1 · 由 DSH 会话（lead）整理，历经 DeepSeek / GLM / Kimi / MiniMax 等多模型接力开发

---

## 1. 项目是什么

**Basalt（玄武岩）——地壳之下最坚实的那层石头。**

Basalt 是一个**自研 x86-64 操作系统内核**的教学/研究项目，仓库中的内核本体叫 **BasaltCore**。它不是 Linux、不基于任何现成内核框架，从裸机（Limine 引导后的 64 位长模式入口）开始，用三种语言分层搭建：

- **汇编（GNU as）**：只做 64 位入口桩（Limine 已代劳固件初始化与模式切换）；
- **C（gcc，freestanding）**：硬件抽象层（端口 IO / 串口 / 键盘 / framebuffer）与核心服务（PMM / 插件注册表）；
- **C++17 子集**（无异常/无 RTTI/无静态析构）：内核入口与输出抽象层。

**生态愿景**：BasaltCore 只是地基。未来将上线 **basalt 插件生态**——内核在启动日志和 shell 横幅中报告已安装插件；没有任何插件时提示 *"search 'basalt plugin' on GitHub (github.com/BS-O/Basalt)"*。内核保持小而坚实，功能通过插件生长。

**硬边界（务必遵守，违反需重新与用户协商）**：

1. 一切构建与运行只在模拟器（QEMU）/隔离环境中；`limine bios-install` 只写生成的 ISO 文件，**严禁触碰真实磁盘/引导区**；
2. 不做完整内核功能（进程调度、文件系统、网络、用户态等）除非先出方案协商；
3. arm64 只占位不实现（目录 + README + 条件编译桩）；
4. 开源组件引入必须先协商后登记，全项目 ≤25% 配额；
5. 凭据卫生：token 不进仓库、不写进任何文件、用后吊销；
6. 遇到需要拍板的事，停下来问用户（选项 + 利弊 + 推荐），不要替用户做决定。

## 2. 仓库与工作区地图

| 位置 | 是什么 | 备注 |
|---|---|---|
| **github.com/BS-O/Basalt** | 主仓库（内核全部代码 + 网站源码 + 全部文档），public，分支 `main` | 唯一权威源 |
| **jihulab.com/BS-O/basalt** | 网站镜像仓库（只镜像 `docs/` 的站点内容 + GitLab CI），public 项目 | 用户明确决定：**内核不上传极狐**，BasaltCore 只在 GitHub |
| **D:\Basalt** | 主工作区（= GitHub 仓库的本地检出） | 所有内核/网站开发在这里 |
| **D:\Basalt-site** | 极狐站点镜像的本地仓库 | `docs/` 内容的手工同步副本，`baseurl=""` |
| **D:\Basalt-edgeone** | EdgeOne Pages 部署包 + 单文件版构建区 | 未入 git；含 `build-single.ps1` |
| **D:\Basalt\legacy\** | 上一代 Multiboot2/GRUB 骨架存档 | 不构建、不入 git、不计配额（ADR-001） |
| **D:\Basalt\.acl-recovery\** | 2026-11-21 ACL 修复事故的备份与恢复脚本 | 可删；留作权限事故档案 |

**域名与站点**：

| URL | 状态 | 用途 |
|---|---|---|
| **https://basalt.bsct.top/** | ✅ 公开主站 | GitHub Pages，`docs/CNAME` = `basalt.bsct.top`（DNS 由用户在 bsct.top 控制台配置） |
| https://bs-o.github.io/Basalt/ | 301 → 自定义域名 | — |
| https://basalt-1057b3.pages.jihulab.net/ | ⚠️ 登录可见 | 极狐 Pages 镜像（实例级策略禁公开 + 证书曾过期，见 §8） |
| ob.bsct.top | 预留 | EdgeOne 自定义域名候选（bsct.top 的 ICP 备案状态未确认，绑定前需确认） |

## 3. 系统现状（内核 M1）

### 3.1 启动链路

```
BIOS/UEFI
  → Limine v12.10.0（固件初始化、16→32→64 位长模式、建页表、装载 ELF 与插件模块、设 1024×768 图形模式）
  → boot/x86/entry64.S  boot_entry（Basalt 第一行代码）
       串口探针 '1' → movabsq 切自建 16KiB 栈(.bss) → 探针 '2'
       → lea framebuffer_request(%rip),%rdi → call kmain
  → kernel/kmain.cpp  探针 'K'
       串口引导 → 横幅 → framebuffer → 内存(PMM) → 键盘 → 插件扫描/初始化
       → "BasaltCore is ready. starting shell..."
  → kernel/shell/shell.c  bsh 交互 shell（PS/2 键盘 + framebuffer 回显，永不返回）
```

### 3.2 启动日志实录（COM1，无时间戳——用户拍板的"初代感"）

```
12K!BasaltCore 0.1.0 (x86-64) - Limine Boot Protocol, base revision 3
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

basalt#
```

开头的 `12K` 与 `!` 是调试探针字节（`1`=进入入口桩、`2`=栈切换完成、`K`=进入 kmain、`!=`framebuffer 不可用降级标记）。**它们是有意保留的诊断遗产**，清理决策见 §7.4。

### 3.3 子系统清单（M1 全部）

| 子系统 | 入口文件 | 说明 |
|---|---|---|
| 引导请求 | `boot/limine/requests.c` | 6 组请求：base revision 3、framebuffer、memmap、hhdm、module（插件）、entry point |
| 入口桩 | `boot/x86/entry64.S` | ~50 行；`movabsq` 取高地址；探针字节 |
| 输出抽象 | `kernel/console/` | 双 sink（framebuffer 主显示 + COM1 调试串口）；`fb_clear()/fb_available()` |
| kprintf | `kernel/lib/kprintf.cpp/.h` | `%s %c %d %u %x %%` + `kvprintf`（va_list 桥） |
| PMM | `kernel/mm/pmm.c/.h` | 位图分配器：1 bit/4KiB 帧、first-fit 线性扫描、HHDM 偏移存储 |
| PS/2 键盘 | `arch/x86/io/kbd.c/.h` | 轮询 scancode set-1 → ASCII（Shift/CapsLock；Enter=10 BS=8 Tab=9）；**无中断，用 `pause` 自旋** |
| bsh shell | `kernel/shell/` | 纯 C 的 `shell.c` + `shell_cxx.cpp`（C-ABI 桥 sh_* 符号）；命令：help/ver/clear/echo/meminfo/plugins/halt |
| 插件注册表 | `kernel/plugins/plugins.c/.h` | 扫描 Limine module → BSPL 头校验 → 登记（≤16 个）→ `plugins_boot()` 调 init |

### 3.4 BSPL 插件格式（v1）

插件 = Limine `module_path` 装载的平铺二进制，首部为小端结构：

```c
struct bplugin_header {          // 共 32 字节
    uint32_t magic;   // 0x4253504C —— 必须与 tools/bplugin.py 保持一致
    uint32_t api;     // = 1（BASALT_PLUGIN_API）
    uint64_t init;    // init 函数指针（0 = 无，视为 init ok）
    uint64_t name;    // name 字符串的【模块内偏移】（不是绝对地址！）
    uint64_t version; // version 字符串的模块内偏移
};
```

- **生成**：`python3 tools/bplugin.py <name> <version> <out.bpl>`（当前示例 `hello 0.1.0`，无 init 函数）；
- **挂载**：`limine.conf` 的 `module_path: boot():/boot/plugins/hello.bpl`；
- **解析**：`plugins.c` 把 name/version 当作 **模块基址 + 偏移**（`base + h->name`）——历史上曾当绝对地址用导致打印插件名时崩溃，这是 M1 修过的真实 bug；
- **已知限制**：无 PIC/重定位处理（init 函数指针只能为 0）、无卸载、无内核服务 API、上限 16 个。

### 3.5 bsh shell 命令

`help` `ver` `clear` `echo ...` `meminfo` `plugins` `halt`。行编辑支持退格（`\b \b` 可视擦除）与 Tab→空格；无历史、无补全、命令长上限 128；0 插件时横幅与 `plugins` 命令都输出 GitHub 搜索提示。

### 3.6 已知 bug / 技术债（诚实清单）

| # | 问题 | 位置 | 建议 |
|---|---|---|---|
| 1 | 启动日志 `hhdm +0x0`：HHDM 偏移被 `%x`（32 位）截断为 0，实际应为 `0xffff800000000000` 量级 | `kernel/kmain.cpp` 的 kprintf 调用 | 改用 `%llx` + `(unsigned long long)` |
| 2 | PMM 位图静态容量：`MAX_FRAMES = 65536` 帧 = **256 MiB**，代码注释却写 "tracks up to 1 GiB"（数学错误）；更大内存会被静默截断 | `kernel/mm/pmm.c:16` | M2 改动态位图（按 max_paddr 从可用内存中挖位图） |
| 3 | 探针字节 `1`/`2`/`K`/`!` 仍在串口输出 | `entry64.S`、`kmain.cpp` | CI 已改断言启动日志内容，探针可清理；`K` 建议保留 |
| 4 | 无 IDT/异常处理：任何内核错误 = 三重故障直接重启，无 panic | 全局 | M2 首要任务 |
| 5 | 键盘轮询 `pause` 自旋（无中断、无 IDT，`hlt` 会永久挂起） | `arch/x86/io/kbd.c` | 与 #4 一起解决（IRQ1 + 环形缓冲） |
| 6 | fb 滚屏逐字节拷贝（1024×768 下可接受但慢） | `arch/x86/console/fb.c` | 低优先级 |
| 7 | kbd 无 0xE0 扩展键前缀处理（方向键不可用） | `arch/x86/io/kbd.c` | shell history 前置条件 |
| 8 | shell 命令分发是硬编码 strcmp 链 | `kernel/shell/shell.c` | 命令表重构 |
| 9 | 版本字符串 "0.1.0" 在 kmain/shell/kprintf 注释多处硬编码 | 多处 | 抽 `kernel/version.h` |
| 10 | `_config.yml` description 仍是 M0 文案；站点 title 是 "Basalt OS" 而品牌已改 BasaltCore | `docs/_config.yml` | 文案清理 |
| 11 | M0 时代的探针 '1','2' 与 CI 断言解耦后未清理（同 #3） | — | — |

## 4. 网站现状

### 4.1 架构与设计语言演变史（全部可追溯）

| 版本 | 提交 | 设计 | 结局 |
|---|---|---|---|
| v1 Claude 风 | 9fd2982 | 奶油 #faf9f5 + 珊瑚 #cc785c + 衬线大标题 | 被 v2 取代 |
| v2 Apple 风 | e5fa1a0 | 44px 黑导航、白/羊皮纸 tile、Action Blue #0066cc | 被 v3 取代 |
| v3 OpenCode 风 | c2e1eb7 | 通体等宽、奶油画布、ASCII 方括号标记、深色 TUI hero | **风格内核保留** |
| **v4 渐变主导（现行）** | 5a4d7f9→4618afa | 深蓝→蓝→绿全屏渐变 + 毛玻璃深色面板 + 浅色文字 + v3 的等宽字体与 ASCII 标记 | **当前** |

> v4 诞生背景：用户先要求"深蓝、蓝、绿渐变"，第一版做成了"白纸+窄渐变边"，用户反馈"网页没改颜色"——教训：**渐变必须是主视觉，不能当描边**（§8 挫折 #26）。

### 4.2 当前设计要点

- **背景**（body，fixed）：两层 radial 光晕 + 160° 线性渐变 `#050a1c → #0b1e4d → #1d4ed8 → #0e7490 → #059669`；
- **内容面板** `.paper`：`rgba(6,10,24,0.52)` + `backdrop-filter: blur(26px) saturate(150%)`，max-width **1440px**（用户要求加宽后的值）；
- **文档主题** `.paper.doc`：由页面 front matter `doc_theme: true` 触发——纯黑 `#0a0a0d` 底、白字、无模糊、`.md-page` max-width **1080px**（2026-11-22 应用户"略微缩窄"要求从 1200px 收窄；背景未动）；
- **字体**：JetBrains Mono（Berkeley Mono 的开源替代，Google Fonts 加载；大陆访问 googleapis 偏慢，自托管是待办）；
- **ASCII 图标体系**：`[+]` 特性、`[$]` 命令、`[*]`/`[#]` 章节标签、`M1 · xxx [x]` 里程碑——零 SVG；
- **交互**：按压 `scale(0.95)`；语言切换胶囊（黑底白字分段）。

### 4.3 页面清单与双语机制

| URL | 语言 | 内容 |
|---|---|---|
| `/` | 双语 chrome | 宣传首页（M1 内容：TUI 启动日志实录、特性列表、三秒上手、文档链接） |
| `/PROJECT-GUIDE/` ↔ `/PROJECT-GUIDE/en/` | zh ↔ en | 项目详细指南（黑底文档主题） |
| `/readme/` ↔ `/readme/en/` | zh ↔ en | README 渲染（黑底文档主题） |

**双语实现三层**：

1. **chrome 文案**：所有界面元素带 `data-zh`/`data-en` 属性，JS 一键切换（`localStorage['basalt-lang']` 持久化）；
2. **文档正文**：文档页是单语言的，front matter `page_lang` + `pair_zh`/`pair_en` 声明配对页；切语言 = `location.href` 跳到配对页（首次进入若与 localStorage 偏好不符会自动重定向）；
3. **链接跟随**：导航与页脚的指南/readme 链接带 `data-href-zh`/`data-href-en`，切语言时同步换 href。

**内容源**：中文文档是仓库里的 `.md`（PROJECT-GUIDE.md 等）；README 中英双语（README.md / README-EN.md）经 `make site`（tools/sync-readme-embed.sh）同步进 `docs/_includes/readme-{,-en-}embed.md`（仓库相对链接重写为 GitHub 绝对链接）并提交。

### 4.4 发布管线

```
编辑 D:\Basalt\docs\* 
  → git commit + push（github.com 需代理，见 §9）
  → GitHub Pages 自动构建（验证：GET 页面 + 正则断言，或 API builds/latest）
极狐镜像：复制变更文件到 D:\Basalt-site → git push（jihulab.com 直连）→ CI 流水线
单文件版：重抓 5 个渲染页到 D:\Basalt-edgeone → powershell build-single.ps1 → 浏览器验证 5 标签
```

**本地没有 Jekyll**（WSL 装 ruby 曾挂起 25 分钟被放弃）——站点预览只能靠 Pages 构建后 HTTP 验证，或临时 `gem install jekyll`（需后台跑、给足超时）。

### 4.5 EdgeOne / 单文件版

- `D:\Basalt-edgeone\basalt-single.html`（~128KB）：**零依赖单文件**——渐变主题 + 5 页签（home/guide-zh/guide-en/readme-zh/readme-en）+ 文档黑底主题动态切换 + 中英切换 + hash 直达。已逐页签实测；
- 构建脚本 `build-single.ps1`：抓取线上 5 个渲染页 → 抽取内容 → URL 重写为锚点 → 注入页签 JS。**注意它必须保持 ASCII-only 正则**（§8 挫折 #6）；
- EdgeOne Pages 上传动作在用户侧控制台完成（单文件或整个文件夹均可），尚未执行。

## 5. 构建与验证速查

**环境事实**：宿主 Windows + WSL2；发行版 **Debian 13 (trixie)**（默认发行版是 docker-desktop，无 bash——必须 `-d Debian`）；预装 gcc 14.2.0 / g++ / make 4.4.1 / xorriso 1.5.6 / QEMU 10.0.13 / python3；**github.com 从 Windows 直连常被重置 → 走本地代理 `socks5h://127.0.0.1:1080`；jihulab.com 直连**。

```bash
# 内核构建 + 冒烟（一切在 WSL Debian 里）
wsl.exe -d Debian -- bash -c 'cd /mnt/d/Basalt && make && make iso && make check'
# 通过标志：CHECK PASSED: M1 boot log on COM1

# 交互运行（QEMU 窗口，键盘直接敲 bsh）
wsl.exe -d Debian -- bash -c 'cd /mnt/d/Basalt && make run'

# framebuffer 截图（headless 诊断）
wsl.exe -d Debian -- bash /mnt/d/Basalt/tools/m1-screendump.sh   # → build/m1-boot.png

# 网站编辑后的发布
cd D:\Basalt; git add docs; git commit; git -c http.proxy=socks5h://127.0.0.1:1080 push origin main
# 验证：Invoke-WebRequest 页面内容 + 正则断言（不要只靠截图）
```

**调试三板斧**（headless 定位引导故障）：`tools/debug-screendump.sh`（看画面，Limine panic 写在屏上）→ `tools/debug-regs.sh`（EIP 在 BIOS 区=回退；长模式=内核层）→ `tools/debug-int.sh`（`-d int` 抓异常向量）。

**凭据与推送**：历史 token 均已在对话中泄露并被用户轮换。新会话开始时**向用户索要新 token 或确认 Credential Manager 已缓存**（Windows 凭据管理器当前已缓存 GitHub 凭据，`git push origin main` 可直接用）。推送模式：`git -c http.proxy=socks5h://127.0.0.1:1080 push origin main`（GitHub）/ `git push origin main`（jihulab）。**绝不把 token 写进任何文件**。

## 6. 任务书演变与现行约束

| 版本 | 内容 | 状态 |
|---|---|---|
| 原始任务书 | Multiboot2 引导、8086→16→32/64 全自研、framebuffer 输出 hello world、25% 配额、四类事项须协商 | 部分被修订 |
| **v2**（docs/TASK-SPEC.md） | 用户拍板改用 **Limine**（其现行版已删 Multiboot2）：协议改 Limine Boot Protocol；16 位/固件初始化归 Limine；Basalt 代码从 64 位入口开始 | ✅ M0 据此完成 |
| **M1 扩围**（用户逐项批准） | 完整版内核：PMM + 初代风启动日志（无时间戳）+ **键盘+framebuffer shell**（用户明确不选串口通道）+ BSPL 插件机制；品牌改 BasaltCore；**内核不上传极狐** | ✅ 已完成并验证 |
| 待定 | M2 及以后范围未拍板（§7 是建议非承诺） | — |

**四类必须先问用户的事**（原始任务书条款，持续有效）：①引入任何开源项目；②可能损坏磁盘/覆盖引导区/修改宿主机的操作；③超出当前里程碑范围的额外功能；④与 x86 主体冲突的 arm64 做法。

## 7. 路线图（建议，均需用户确认后开工）

### 7.1 M2 内核稳固化（推荐下一里程碑）

| 任务 | 入口文件 | 验收标准 | 估量 |
|---|---|---|---|
| IDT + 异常处理 + panic | 新建 `arch/x86/cpu/idt.c`、`isr.S`；kmain 注册 | 故意 `int $3`/空指针解引用 → 屏幕出寄存器转储 panic 而非三重重启 | ~250 行 |
| 中断驱动键盘 + PIT tick | `kbd.c` 改 IRQ1 + 环形缓冲；PIT 100Hz | `kbd_getchar` 不再自旋；meminfo 可加 uptime | ~200 行 |
| PMM 动态位图 | `pmm.c` | 2GiB VM 下 meminfo 正确；修 #2 注释错 | ~100 行 |
| 修 hhdm 打印截断 | `kmain.cpp` | 日志显示真实 HHDM 值 | 10 分钟 |
| 自己的 GDT | 新建 `arch/x86/cpu/gdt.c` | lgdt 后一切正常 | ~80 行 |
| 串口双向 shell 开关 | Makefile `CONSOLE=serial` | CI 可脚本化交互测试 | ~80 行 |

### 7.2 M3 shell/体验

bsh v2：命令历史（kbd 需先支持 0xE0 方向键）、Tab 补全、fb 颜色输出（fb_putc 加属性参数）、`uptime`/`splash` 等命令。

### 7.3 M4 插件生态（Basalt 的差异化主线）

1. **BSPL v2**：PIC + slide 重定位（插件可用 init 真正执行代码）；内核服务表（console_putc / pmm_alloc / kprintf 指针表传给 init）；
2. **插件仓库模板**：GitHub 上建 basalt-plugin-template 仓 + 索引页（README 插件列表）；
3. **`basalt plugin install` 体验**：需要先有 VFS/FAT（远期），或先用"构建期打包"过渡；
4. **版本管理**：api 版本协商、不兼容插件拒绝装载的明确报错。

### 7.4 网站与治理待办

| 事项 | 说明 |
|---|---|
| GitHub Actions CI | ubuntu runner 跑 `make check`（apt 装 qemu/xorriso），README 加 badge |
| EdgeOne 上线 | 用户在腾讯云控制台上传 `D:\Basalt-edgeone`（单文件或文件夹）；视备案情况绑 ob.bsct.top |
| 字体自托管 | JetBrains Mono woff2 进 `docs/assets/fonts/`，摆脱 googleapis 依赖（大陆速度） |
| 文案清理 | `_config.yml` description 更新到 M1；站点 title 决定用 "Basalt OS" 还是 "BasaltCore" |
| 探针清理 | CI 断言已不依赖探针；删 '1'/'2'，'K' 保留（改动需重验 make check） |
| 里程碑时间线页 | 站点加 /milestones/（C1 旧提案） |
| 仓库门面 | About 描述、topics（osdev/x86-64/limine/kernel/cpp）、social preview（C2 旧提案） |
| 单文件构建脚本入仓 | 把 D:\Basalt-edgeone\build-single.ps1 挪进 tools/ 并加 Makefile target |

### 7.5 远期（不承诺顺序）

VMM（页表/HHDM 窗口 API）→ VFS+FAT → 插件从文件系统装载 → 用户态+syscall → SMP/APIC。每一步都要走"方案→确认→实现→验证"。

## 8. 挫折实录（30 条，前人用血泪换的）

> 格式：**现象** → 根因 → 解法。遇到类似症状先来这里查。

### A. 沙箱与宿主环境

1. **归档移动全部被拒**（"对路径的访问被拒绝"）→ DSH 沙箱在工作区根有"禁止删除"保护项（移动=复制+删除）→ 用户把会话切到完全权限后完成。**教训**：批量移动/删除类操作先确认沙箱策略。
2. **`wsl.exe` 报"拒绝访问 E_ACCESSDENIED"** → 沙箱限制；放宽后仍拒 → 最终在完全权限下可用。
3. **WSL 默认发行版是 docker-desktop**（无 bash，`bash: not found`）→ 一律 `wsl.exe -d Debian --`。
4. **WSL `/tmp` 在两次调用之间被清空**（空闲自动关机）→ 多步操作必须写成单脚本一次跑完（tools/ 下所有脚本皆因此而生）。
5. **WSL 里 `apt install ruby` 挂起 25 分钟+** → 放弃本地 Jekyll，站点预览依赖 Pages 构建 + HTTP 验证。如需本地 Jekyll，后台跑给足超时。
6. **PowerShell 双引号字符串会把 `$()` / `$var` 先插值再传给 bash** → bash 收到的是残缺命令。**解法**：复杂命令写成 tools/*.sh 脚本文件执行。
7. **PS5.1 把无 BOM 的 .ps1 按 ANSI 读 → 脚本里的中文正则全变乱码**（build-single.ps1 的首页导航链接替换静默失效）。**解法**：正则只用 ASCII（按 href 值匹配而非中文文案），或脚本存成 UTF-8 with BOM。
8. **github.com 从 Windows 直连被重置** / **SourceForge 下载拿到的是 HTML 跳转页** → Windows 侧走代理 `socks5h://127.0.0.1:1080`；SourceForge 用 WSL curl。
9. **Invoke-RestMethod 发中文 JSON body 变乱码**（ISO-8859-1）→ 用 `[Text.Encoding]::UTF8.GetBytes` 传字节流，或发 base64（GitLab commits API 用 encoding=base64）。

### B. 内核与 Limine

10. **Limine 拒载**："Lower half PHDRs are not allowed" → 内核必须高半区链接（`KERNEL_VMA=0xffffffff80000000`）。
11. **Limine panic**："PHDRs with different permissions sharing the same memory page" → `.limine_requests`（只读）与 .text（可执行）必须页对齐隔离（`ALIGN(0x1000)`）。
12. **BIOS 引导 PANIC "Stage 3 file not found"** → ISO 里必须有 `limine-bios.sys`（bios-install 的 Reminder 提示不是废话）。
13. **链接错误 R_X86_64_32 truncation** → 高半区必须 `-mcmodel=kernel`（small 模型不行）。
14. **入口桩取 .bss 高地址挂死** → `movq` 装不下 0xffffffff… 地址，必须 `movabsq`。
15. **打印插件名三重故障** → BSPL 头里的 name/version 是**模块内偏移**，被当绝对地址解引用。解法：`base + h->name`。
16. **键盘 `hlt` 等待永久挂起** → M1 无 IDT/中断，hlt 后无人唤醒。解法：`pause` 自旋（IDT 落地后改中断驱动）。
17. **日志显示 `hhdm +0x0`** → `%x` 是 32 位，u64 的 HHDM 偏移被截断。待修（§3.6 #1）。
18. **kprintf 输出中文/长整型不工作** → printf 子集只支持 %s%c%d%u%x；%lld 需要"吃掉 l"特判（已实现）。

### C. 构建系统

19. **Makefile "No rule to make target build/kprintf.o"** → vpath 按扩展名分开列（kernel/lib 在 %.cpp 列表、kernel/shell 在两个列表都要有）。
20. **"fatal error: console/console.h"** → kernel/ 不在 include 路径 → `-Ikernel`。
21. **改了 Makefile/链接脚本但 make 说 Nothing to be done** → Makefile 不是 .o 的依赖；`make clean` 或 `rm -f` 目标文件。
22. **iso 目标 "Nothing to be done"**（只改了 recipe 没改依赖）→ `rm -f build/basalt.iso` 强制重建。

### D. 网站 / Jekyll / GitLab CI

23. **.md 里的原生 HTML 块被 kramdown 逐字转义**（页面显示源码）→ 首页用纯 .html（带 front matter 让 Liquid 处理 relative_url）；.md 只放纯 markdown。
24. **暗色带上标题看不见**（同特异性选择器，后声明者胜）→ 提高选择器特异性 `.band-dark h2.display-lg`。
25. **ASCII 字标被读成"ADEV"** → 像素字形太抽象 → 用全 █ 块按 5 行重绘，逐字母校对。
26. **GitLab CI 流水线 0 个作业直接 failed** → script 里未加引号的字符串含 `: ` 被 YAML 当成映射 → 全部加引号。
27. **GitLab CI "Can't use --version with multiple gems"** → `gem install a b -v 1.0` 不合法 → 用 `gem install "a:1.0" b`。
28. **GitLab CI 缺 kramdown-parser-gfm** → `kramdown: input: GFM` 在裸 ruby 镜像必须显式安装该 gem。
29. **极狐 Pages 平台限制**（非配置可解）：Pages 公开访问级别实例级禁用（API 明报）；自定义域名 API/UI 已下线（404）；TLS 证书出现过期（ERR_CERT_DATE_INVALID）。→ 公开宣传以 GitHub Pages 为准，极狐侧当登录可见的镜像；若要解锁只能向极狐提工单。
30. **"网页没改颜色"** → 渐变做成了白纸描边而非主视觉 → 重做为全屏渐变 + 毛玻璃深色面板 + 浅色文字。**教训：视觉需求要按"主导/点缀"确认层级，不要只按字面执行。**

### E. 凭据与协作

31. **多枚 token 在对话明文出现** → 全部建议用户吊销轮换（用户执行了）。**规则：token 只走一次性 URL 参数或凭据管理器，永不落盘、永不写进文件。**
32. **细粒度 PAT 推送 403 "Permission denied to BS-O"** → 缺 Contents RW；Pages API 403 → 缺 Pages RW。**教训**：要 token 时明确列出所需权限清单（经典 token：repo+workflow；细粒度：Contents RW + Pages RW + Administration RW）。
33. **GitHub Pages 构建验证**：`GET /repos/BS-O/Basalt/pages/builds/latest`（需 token）；页面级验证用 `Invoke-WebRequest` + 正则断言（如 `max-width: 1080px`）。

### F. 工具与验证

34. **`read_image`/视觉模型可用性随会话模型波动** → 优先用 DOM 断言（browser_eval + getComputedStyle）与像素采样（System.Drawing GetPixel 统计色分布）兜底；vision_describe 可用时用于语义确认。
35. **长命令超时转后台** → `job_output(wait:true)` 收尾；卡死 `job_kill`。
36. **wsl bash -c 内嵌 python heredoc 被引号搞坏** → 写成 tools/*.sh（内含 `<<'EOF'` 单引号 heredoc）再执行。

## 9. 开发经验与协作约定

1. **决策流程**（用户原始任务书规定，持续有效）：先出方案选项（含利弊与推荐）→ 用户明确同意 → 才落地。四类事项（开源引入/危险操作/扩围/arm64）必须问。
2. **决策留痕**：每个拍板写一条 `docs/adr/ADR-NNN-*.md`（背景/备选/决策/后果）。已有 ADR-001~006。
3. **验证文化**：每个里程碑必须有**自动断言**（make check）+ **视觉证据**（screendump）。改一行也要跑 `make check`。
4. **双端同步顺序**：GitHub 主仓先行 → 极狐镜像跟随（手动复制变更文件）→ EdgeOne/单文件按需重建。三处不同步是技术债。
5. **文档即站点**：docs/ 既是仓库文档又是网站源。写文档时记住它在双端都会出现。
6. **与用户沟通**：中文；给选项时附推荐与代价；重大冲突（如 Limine vs Multiboot2）要摆事实给三条路；用户 decisive，给了答案就执行不再反复确认。
7. **代码风格**：分层依赖单向（L3→L2→L1→L0）；跨语言一律 extern "C"；C++ 三禁（异常/RTTI/atexit）；编译 flags 每条有理由（见 PROJECT-GUIDE §5.3）；提交信息一行式动词开头。
8. **诚实报告**：失败/部分完成/未验证都要明说。本手册 §3.6 与 §8 就是这个文化的产物。

## 10. 文件索引（速查）

> 逐文件详解看 `docs/PROJECT-GUIDE.md`（§4，比本文更细）。此处只列 M1 后新增/易混淆的：

- `kernel/mm/pmm.c` — 位图 PMM（容量债 #2）
- `kernel/lib/kprintf.cpp/.h` — 含 kvprintf（C 桥需要）
- `arch/x86/io/kbd.c/.h` — 轮询键盘（pause 自旋）
- `kernel/shell/shell.c` + `shell_cxx.cpp` — 纯 C shell + C-ABI 桥（这个双文件模式是"纯 C 命令层调 C++ 子系统"的既定模式，照抄即可）
- `kernel/plugins/plugins.c/.h` — BSPL 注册表（偏移解析）
- `tools/bplugin.py` — 插件 blob 生成器（与 plugins.c 的 magic/结构必须同步改）
- `tools/m1-screendump.sh` — M1 截图
- `limine.conf` — module_path 挂插件
- `docs/AGENT-HANDBOOK.md` — 本文件（也是站点页面 /AGENT-HANDBOOK/）
- `AGENTS.md`（仓库根）— agent 快速上手卡
- `D:\Basalt-edgeone\build-single.ps1` — 单文件构建（ASCII-only 正则！）

## 11. 给下一代 agent 的第一批任务建议

按性价比排序，全部走"方案→确认→实现→验证"：

1. **修 hhdm 打印截断**（10 分钟，§3.6 #1）——热身；
2. **GitHub Actions CI**：ubuntu 容器 `make check`（~1h）——从此每次 push 自动冒烟；
3. **IDT + panic**（半天~1 天，§7.1）——内核从"错即重启"变"错能说话"，后续一切开发的调试体验质变；
4. **PMM 动态位图**（半天）——解容量债；
5. **EdgeOne 上线**（用户控制台 10 分钟，agent 给指引）；
6. **插件真 init**：BSPL v2 PIC + 内核服务表（1~2 天）——生态起点；
7. **网站字体自托管 + 文案清理**（30 分钟）。

**第一次接触本项目的验证清单**（证明你理解了环境）：`make check` 通过 → `make run` 键盘能敲 → `basalt.bsct.top` 三页可达 → 单文件 5 页签可切 → 能说出 Limine 三大坑。

## 12. 附录

### 12.1 网络与凭据速记

- github.com：Windows 直连不稳 → `git -c http.proxy=socks5h://127.0.0.1:1080 ...`（本机 1080 SOCKS5 常驻）；jihulab.com / basalt.bsct.top 直连；
- 推送凭据：Windows 凭据管理器已缓存（`git push origin main` 直推）；若失效向用户要新 token（GitHub 经典 token 需 repo+workflow；或细粒度 Contents RW + Pages RW），用一次性 URL 参数方式，用后提醒吊销；
- GitHub API 无 token 时限 60 次/小时，够用但查询多时带 token。

### 12.2 DSH 会话特有注意事项（其他工具可跳过）

- 文件策略历史曾从 workspace-write → danger-full-access（用户手动切）；批量移动/删除在受限策略下可能被"禁止删除"保护拦截；
- `ask_user_question` 是与用户拍板的正式通道；选项第一项标注"（推荐）"；
- 视觉工具（read_image/vision_describe）可用性随会话模型变化；像素采样（System.Drawing）是可靠兜底；
- 后台 job：超时命令自动转后台，`job_output(wait:true)` 收；卡死 `job_kill`。

### 12.3 术语表

| 术语 | 含义 |
|---|---|
| Limine | 现代引导程序，BSD-2 许可；v12.10.0，Limine Boot Protocol base revision 3 |
| HHDM | Higher-Half Direct Map——Limine 把全部物理内存映射到高半区的窗口，偏移经 hhdm 请求取得 |
| BSPL | Basalt Plugin——本项目的插件二进制格式（v1：magic+api+init+偏移） |
| bsh | basalt shell——M1 的 framebuffer 交互 shell |
| paper / paper.doc | 网站的两套主题：渐变毛玻璃（首页）/ 纯黑阅读（文档页） |
| 探针字节 | entry64.S/kmain 往 COM1 写的 '1'/'2'/'K'，用于 headless 判断引导推进到哪一 stage |
| 三板斧 | screendump / info registers / -d int 三个 QEMU 监视器诊断手段 |

---

*本手册本身也是文档即站点的一部分（/AGENT-HANDBOOK/）。发现它与实际不符时，第一件事就是更新它——这就是它的工作方式。*
