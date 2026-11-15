# Basalt

玄武岩——地壳之下最坚实的那层石头。自研 x86-64 内核（教学/研究项目）。

**当前里程碑 M0**：Limine 引导 → 64 位长模式入口 → framebuffer 输出 `hello world`。

| 文档 | 内容 |
|---|---|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | 架构设计（分层/目录/引导链路/输出通道） |
| [docs/TASK-SPEC.md](docs/TASK-SPEC.md) | 任务书 v2（Limine 方案修订版） |
| [docs/adr/](docs/adr/) | 全部已拍板决策（ADR-001 ~ ADR-006） |
| [third_party/README.md](third_party/README.md) | 开源组件配额登记（Limine / Terminus） |

## 构建（宿主：WSL2 Debian，ADR-005）

所需工具（Debian 13 已预装）：`build-essential`、`xorriso`、`qemu-system-x86`、`python3`。

```bash
# Windows 侧：从 PowerShell 进入
wsl.exe -d Debian
cd /mnt/d/Basalt

make          # 生成字模头文件 + 内核 ELF (build/basalt.elf)
make iso      # 生成 build/basalt.iso（xorriso + limine bios-install）
```

## 运行与验证

```bash
make run      # 打开 QEMU 窗口：黑屏上显示白色 "hello world"（framebuffer 主显示）
make check    # headless 冒烟：断言 COM1 串口日志出现 "hello world"（ADR-003）
```

预期结果：
- `make run`：QEMU 窗口左上角出现 `hello world`，随后内核 `cli; hlt` 静默停机（正常）；
- `make check`：输出 `CHECK PASSED: hello world on COM1`（15 秒超时自动结束）。

## 布局速览

```
boot/limine   Limine 请求标记（base rev 3 + framebuffer + entry point）
boot/x86      entry64.S 64 位入口桩（建栈 → 调 kmain）
arch/x86      HAL：端口 IO / 串口 / framebuffer 控制台
kernel/       kmain.cpp、console 输出抽象、mm 占位接口
lib/kstd      freestanding mem*/str*
third_party/  vendored：limine.h、Limine 引导二进制、Terminus 字模
legacy/       上一代 Multiboot2 骨架（仅存档，不参与构建）
```

## 注意事项

- 一切构建与运行都在模拟器/隔离环境；**不写真实磁盘引导区**；
- `third_party/vendored/` 为带 SHA256 校验记录的原始发布包（不参与构建）；
- 修改字模请改 `tools/genfont.py` 流程而非生成文件（`make font` 重新生成）。
