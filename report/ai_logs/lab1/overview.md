# Lab1 AI 协作总览

Leader：Claude Code（Claude Opus 5.5）。子 Agent 通过 Herdr 终端复用器调度：Codex CLI（GPT-6.1-Sol medium，面板 w9:p7）负责实现、测试、调试、报告；pi（deepseek-flash，面板 w9:p8）负责 Review。每个任务的 PROMPT / RELY / GUARANTEE / SPECIFICATION 写在 `task_0X.md`，Leader 通过 `herdr agent prompt` / `herdr pane send-text` 发送调度消息，要求 Agent 读取规格文件执行。

| Task | Agent | 内容 | 状态 |
|------|-------|------|------|
| [01](task_01.md) | Codex | 构建、make qemu、符号地址；gdb 改为 gdb-multiarch | 构建 DONE，启动 FAILED → Bug 1 |
| [02](task_02.md) | Codex | 修复 Bug 1：loader → `-kernel` | DONE |
| [03](task_03.md) | Codex | 练习 1/2 GDB 四阶段数据采集 | DONE |
| [04](task_04.md) | Codex | 测试截图渲染 + 撰写 report.md | DONE |
| [05](task_05.md) | pi | Review 报告与 Makefile | NEEDS_FIX（2 项表述） |
| [06](task_06.md) | Codex | 按 Review 修订 | DONE |

Bug 记录见 [bugs.md](bugs.md)，原始输出见 `evidence/`，最终状态见 [final_summary.md](final_summary.md)。
