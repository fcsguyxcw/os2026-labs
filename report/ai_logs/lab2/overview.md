# Lab2 AI 协作总览

Leader：Claude Code（Claude Opus 5.5）。子 Agent 通过 Herdr 终端复用器调度：两个 Codex CLI（面板 w1:p2、w1:p3）。每个任务的 PROMPT / RELY / GUARANTEE / SPECIFICATION 写在 `task_0X.md`，Leader 用 `herdr agent prompt` 发一条调度消息，要求 Agent 读取规格文件执行。提示词格式依据：实验指导书 `lab0.5/3_prompt_structure.md`、10-13 实验课宫晓利老师要求、10-17 助教妙记“vibe coding for OS”。

截止时间：2026-10-26（10-13 实验课宫晓利老师原话“Lab 2 的截止时间是 26 号”，按会议日期推算）。

| Task | Agent | 内容 | 状态 |
|------|-------|------|------|
| [01](task_01.md) | Codex w1:p2 | page_init | DONE（运行由 Leader 执行，见 Bug 1） |
| [02](task_02.md) | Codex w1:p3 | Best-Fit 管理器；Leader 切换 `init_pmm_manager` | DONE，grade 20/20 |
| [03](task_03.md) | Codex w1:p2 / w1:p3 | 交叉 Review | Best-Fit PASS；page_init NEEDS_FIX → Bug 2 |
| [04](task_04.md) | Codex w1:p2 | 修复描述符数组越界 | DONE；越界分支运行时 UNVERIFIED |
| [05](task_05.md) | Codex w1:p3 | 分支任务：QEMU 源码静态分析与调试方案 | DONE（经 Task 07 实测验证） |
| [06](task_06.md) | Codex w1:p2 | 报告 P0 部分、First-Fit 思考题、Challenge | DONE |
| [07](task_07.md) | Leader | 分支任务：Docker 中双 gdb 实际调试（两轮）+ satp 旧 TLB 表项现象 | DONE |
| [08](task_08.md) | Codex w1:p3 | 报告分支任务 6 问、演示步骤 | DONE；演示稿命令经 Leader 批处理验证 |
| [09](task_09.md) | Codex w1:p2 | Buddy System 实现（第一份 prompt）；Leader 加编译开关 | DONE |
| [10](task_10.md) | Codex w1:p2 | Buddy 测试（第二份 prompt） | DONE；8 组通过，3 个变异全部检出 |
| [11](task_11.md) | Codex w1:p3 | SLUB 实现（第一份 prompt） | DONE |
| [12](task_12.md) | Codex w1:p2 | SLUB 测试（第二份 prompt） | DONE；9 组通过，3 个变异全部检出 |
| [13](task_13.md) | Codex w1:p3 | 报告 Buddy 一节 | DONE |
| [14](task_14.md) | Codex w1:p3 | 报告收尾：SLUB、知识点、总结、final_summary | DONE（指出 Task 08 记录不一致，已更正） |
| [15](task_15.md) | Codex w1:p2 | 全文独立核对 | PASS，2 条 NOTE 已采纳 |

## 调度消息原文

- 2026-10-24 → w1:p2：「你是 OS Lab2 的 Implementation/Test Agent。请完整阅读 …/report/ai_logs/lab2/task_01.md 中的 PROMPT、RELY、GUARANTEE、SPECIFICATION 四节（其余小节忽略），严格按其中要求执行：只改允许改的文件，编译和测试在 docker 镜像 os2026-dbg 里跑，证据存到 …/evidence/。完成后按 PROMPT 里的输出要求回复，不要编造任何没有实际运行得到的结果。」
- 2026-10-24 → w1:p3：「你是 OS Lab2 的 Implementation/Test Agent。请完整阅读 …/report/ai_logs/lab2/task_02.md 中的 PROMPT、RELY、GUARANTEE、SPECIFICATION 四节（其余小节忽略），严格按其中要求执行：只改允许改的文件，编译和测试在 docker 镜像 os2026-dbg 里跑，证据存到 …/evidence/。注意 PROMPT 第 4 条的验证分两步，第一步做完先回复我，等我通知后再做第二步。不要编造任何没有实际运行得到的结果。」
- 2026-10-24 → w1:p2、w1:p3（Task 03）：「你是 OS Lab2 的 Review Agent。请阅读 …/task_03.md 的 PROMPT/RELY/GUARANTEE/SPECIFICATION 并执行。你负责审查 Best-Fit（或 page_init）…，结果写入 …/evidence/t3_review_<对象>.md。只读审查，不改源码。」
- 2026-10-24 → w1:p2（Task 04）：「你是 OS Lab2 的 Debug Agent。请阅读 …/task_04.md 的 PROMPT/RELY/GUARANTEE/SPECIFICATION 并执行。不要尝试 Docker，不要改 page_init 以外的代码。」

Bug 记录见 [bugs.md](bugs.md)，原始输出见 `evidence/`，最终状态见 [final_summary.md](final_summary.md)。
- 2026-10-24 → 其余任务（Task 05、06、08～13）的调度消息格式相同：「你是 OS Lab2 的 <角色> Agent。请阅读 …/task_0X.md 的 PROMPT/RELY/GUARANTEE/SPECIFICATION 并执行。<只改哪些文件 / 不要尝试 Docker / 上一步 Leader 的验证结果>」。完整内容以各 task 文件为准。

## 编译开关（扩展任务）

- 默认 `make`：Best-Fit，`make grade` 20/20。
- `make DEFS=-DLAB2_PMM_BUDDY`：系统物理内存管理器换成 Buddy，`check` 运行 `buddy_check`。
- `make DEFS=-DLAB2_SLUB_TEST`：在 `check_alloc_page()` 之后运行 `slub_init(); slub_check();`。
