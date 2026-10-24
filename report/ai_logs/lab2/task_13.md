# Task 13：报告 · Buddy System 一节

执行者：Codex w1:p3（Report Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Report Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`。

任务：
1. 在 `report/report.md` 的“功能模块二 · 任务三：Buddy System”一节替换 TODO，写：模块功能与设计（数据结构、order、对齐基准、非 2 的幂区域如何切分、拆分与合并、内部碎片）；如何接入内核（`make DEFS=-DLAB2_PMM_BUDDY` 时 `init_pmm_manager()` 切换到 `buddy_pmm_manager`，默认构建仍是 Best-Fit，grade 不受影响——依据 Task 09 Verification）；两份 prompt 是怎么写的（实现 prompt 与测试 prompt 各自的四段式要点，原文引用 `prompt.md`）；**测试覆盖了哪些核心行为和边界情况，以及为什么这些测试能有效检验实现**——必须用 Task 10 的变异测试结果（`evidence/t10_mutation.txt`）说明：注入 3 种错误后测试分别在哪一组、哪条断言失败；同时列出未覆盖的情况（会 panic 的非法调用、`order 0` 块头无法识别误用等 NOTE）；实现迭代过程（Task 09 → Leader 集成 → Task 10 → 变异测试）。
2. “五、测试与验证”中补 Buddy 的命令与实际输出（`evidence/t10_buddy_qemu.txt` 里 8 组组名和 `buddy_check() succeeded!`，原样短引用）。
3. `report/prompt.md` 末尾逐字追加 Task 09、Task 10 的四段式提示词（`## PROMPT` 到 `## SPECIFICATION` 末尾），格式与已有小节一致。

操作要求：只改 `report/report.md`、`report/prompt.md`；不改代码和日志，不 git commit / push，不运行 Docker。事实只来自 `report/ai_logs/lab2/task_09.md`、`task_10.md`、`evidence/t9_*`、`evidence/t10_*` 和源码 `code/kern/mm/buddy_pmm.c`、`code/kern/mm/pmm.c`；查不到写 TODO。SLUB 一节保持 TODO（另一个 Agent 正在写 SLUB 测试）。完成后回复 ≤8 行。

## RELY

- `task_09.md`、`task_10.md`（含 Leader Verification 与变异测试结论）；`evidence/t9_buddy_qemu.txt`、`t10_buddy_qemu.txt`、`t10_mutation.txt`、`t10_buddy_check.diff`；`code/kern/mm/buddy_pmm.c`。
- 指导书要求：提交两份 prompt，并简要说明测试覆盖了哪些核心行为和边界情况，以及这些测试为什么能够有效检验生成的 Buddy System 实现。

## GUARANTEE

- Buddy 一节无 TODO；`prompt.md` 新增内容与 task 文件逐字一致；其它小节不受影响。

## SPECIFICATION

- Preconditions：Task 09、10 完成，commit `d194217`。
- Postconditions：`git diff --stat` 只显示 `report/report.md`、`report/prompt.md`。
- Edge cases：日志与源码冲突时以源码和 evidence 为准，并在回复中指出。

## Agent Result

Codex 回复（2026-10-24 22:03）：`report.md` 完成 Buddy 设计、接入、8 组测试、三种变异失败及未覆盖边界；`prompt.md` 逐字追加 Task 09、10；SLUB 保留 TODO；未改代码或日志。

## Verification

Leader 审读：数据结构、拆分/合并、接入方式与源码一致；8 组测试表、变异表与 `evidence/t10_*` 一致；指出了 M2 变异副本行号比原文件多 1 行的原因；未覆盖项（非法调用、MAX_ORDER 与伙伴越界分支、order 0 误用 NOTE）如实列出。

状态：**DONE**。
