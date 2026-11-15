# ADR-002：引导采用 Limine Boot Protocol（任务书修订）

- 状态：已接受（2026-11-15，用户拍板，方案 B）
- 背景：原任务书规定引导协议 Multiboot2。用户在引导项目选择中倾向"优秀的开源项目"。事实核查：Limine（BSD-2-Clause）现行版本已移除 Multiboot1/2 支持，仅支持自有 Limine Boot Protocol。
- 备选方案：A) 纯 GRUB2（协议不变，Limine 不可用）；B) 纯 Limine（修订任务书）；C) Limine 主 + GRUB2 兼容双协议。
- 决策：**B）纯 Limine**。用户明示接受任务书修订：引导协议由 Multiboot2 改为 Limine Boot Protocol；"16 位实模式切换"等固件初始化由 Limine 完成，Basalt 自身代码从 64 位长模式入口开始。
- 后果：
  - Basalt 不再实现 32→64 长模式切换、页表初建、ELF 装载（Limine 全部代劳），引导侧代码量最小（~80 行）；
  - framebuffer 由 Limine 直接映射并经响应结构给出（无需解析 Multiboot2 MBI）；
  - 失败回退预案：若 Limine 路线验证受阻，回退自研 8086 引导扇区（B2）或 GRUB2/Multiboot2 路线，需重新协商；
  - 配额口径：Limine 本体为运行时外部组件不计入 25%；vendored 的 `limine.h` 计入（BSD-2，<1%）。
