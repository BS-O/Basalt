# ADR-003：输出通道 = framebuffer 主显示 + 调试串口

- 状态：已接受（2026-11-15，用户拍板）
- 背景：候选有 VGA 文本模式、串口、Multiboot2 framebuffer、Limine framebuffer。用户选定 framebuffer 作为 hello world 输出通道；冒烟验证另需可自动断言的通道。
- 决策：
  1. **主显示：Limine framebuffer**（32bpp 线性 RGB/BGR，分辨率以 Limine 响应为准，默认请求 1024×768）；8×16 字形绘制（字模见 ADR-004）。
  2. **调试串口 COM1（16550 轮询）**：仅供日志与 CI 断言，构建开关可关；不属于用户可见的"输出通道"替换项。
- 理由：QEMU 窗口直接可见（用户验收直观）；串口使 `make check` 可 headless 断言，避免脆弱的截图像素比对。
- 抽象层：`kernel/console/` 多 sink 装配，架构无关；arm64 将来提供自己的 sink。
