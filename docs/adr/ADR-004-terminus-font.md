# ADR-004：字模引入 Terminus（SIL OFL 1.1）

- 状态：已接受（2026-11-15，用户拍板）
- 背景：framebuffer 画字需要字模点阵。候选：自研 8×8（0% 配额、形状朴素）vs 开源字模（美观、计入配额）。
- 决策：引入 **Terminus Font** 位图字模（SIL Open Font License 1.1），取 8×16 ASCII 字形（95 个可打印字符）。
- 实施方式：由 `tools/` 下自有转换脚本从 BDF 源生成 C 头文件（约 4KiB 数据）进 `third_party/terminus/`，同目录保留许可证文本与来源记录。
- 配额影响：计入 25% 开源配额，数据文件约 4KiB，占比极小（<1%）。
