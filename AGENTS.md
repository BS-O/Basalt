# AGENTS.md — Basalt / BasaltCore

> **本文件是 AI 编码工具（opencode / Pi / trae / zcode / Codex 等）的自动入口。**
> 完整背景、30 条踩坑实录、路线图请读 **[docs/AGENT-HANDBOOK.md](docs/AGENT-HANDBOOK.md)**（也是网站页面 /AGENT-HANDBOOK/）。

## 这是什么

**BasaltCore** —— 自研 x86-64 内核（教学/研究），Limine 引导，C++17 子集 + C + GNU as。
当前里程碑 **M1**：位图 PMM、初代 Unix 风启动日志、PS/2 键盘 + framebuffer shell（bsh）、BSPL 插件机制。
它是未来 **basalt 插件生态** 的内核核心。

## 30 秒上手（一切在 WSL Debian 里）

```bash
wsl.exe -d Debian -- bash -c 'cd /mnt/d/Basalt && make && make iso && make check'
# 通过标志：CHECK PASSED: M1 boot log on COM1
wsl.exe -d Debian -- bash -c 'cd /mnt/d/Basalt && make run'   # QEMU 窗口，键盘直接敲 bsh
```

## 十大坑（先读这个，能省你半天）

1. WSL 必须指定发行版：`wsl.exe -d Debian`（默认是 docker-desktop，无 bash）；
2. 复杂命令写成 `tools/*.sh` 脚本执行——PowerShell 双引号会先吞 `$()`，WSL 的 `/tmp` 会跨调用清空；
3. Windows PowerShell 5.1 把无 BOM 的 .ps1 按 ANSI 读——脚本里的中文正则会静默失效，正则只用 ASCII；
4. github.com 从 Windows 直连常被重置：`git -c http.proxy=socks5h://127.0.0.1:1080 push origin main`；jihulab.com 直连；
5. 内核必须高半区链接（`0xffffffff80000000`）+ `-mcmodel=kernel`；Limine 禁止不同权限段共享 4K 页；ISO 必须带 `limine-bios.sys`；
6. BSPL 插件头里的 name/version 是**模块内偏移**不是绝对地址（解引用前 `base + off`）；
7. M1 无 IDT/中断——键盘用 `pause` 自旋，`hlt` 会永久挂起；任何内核错误 = 三重故障直接重启（无 panic）；
8. 无本地 Jekyll（ruby 安装会挂起）——站点验证靠 Pages 构建后 HTTP 正则断言；
9. **token 永不落盘**：推送走 Windows 凭据管理器或一次性 URL 参数；对话里出现过的 token 一律视为已泄露，用后吊销；
10. 四类改动必须先问用户：引入开源组件、危险操作（真实磁盘/引导区/宿主机）、超出当前里程碑范围、arm64 实现。

## 协作契约

方案选项（附利弊与推荐）→ 用户拍板 → 实现 → **自动断言 + 截图验证** → 决策写入 `docs/adr/`。诚实报告失败与未验证项。

## 地图

| 内容 | 位置 |
|---|---|
| 完整交接手册（背景/挫折/路线图） | [docs/AGENT-HANDBOOK.md](docs/AGENT-HANDBOOK.md) |
| 逐文件详解（中/英） | docs/PROJECT-GUIDE.md · docs/PROJECT-GUIDE-EN.md |
| 架构设计 | docs/ARCHITECTURE.md |
| 任务书（Limine 修订版） | docs/TASK-SPEC.md |
| 决策记录 | docs/adr/ADR-001~006 |
| 开源配额登记 | third_party/README.md |
| 网站 | https://basalt.bsct.top/ （源码 = 本仓库 docs/） |
| 站点镜像仓（极狐，仅网站） | jihulab.com/BS-O/basalt ↔ 本地 D:\Basalt-site |
| EdgeOne 包 + 单文件版 | 本地 D:\Basalt-edgeone（不入 git） |
| 旧代码存档 | legacy/（不构建不入 git） |
