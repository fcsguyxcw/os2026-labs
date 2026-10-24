# Task 15：全文独立核对（Review）

执行者：Codex w1:p2（Review Agent；未参与 report.md 后半部分的写作）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Review Agent。只读审查，**不修改任何文件**，只新建一个结果文件；不 git commit / push，不运行 Docker。

任务：逐节审查 `report/report.md` 和 `report/ai_logs/lab2/分支任务演示步骤.md`，找出：
1. 与证据不一致的事实（数值、地址、行号、分数、时间、谁做了什么）——对照 `report/ai_logs/lab2/evidence/`、各 `task_XX.md` 和源码（`code/`、`/Users/wesley/大三上课程/操作系统/tmp/qemu-src/`）；
2. 把推测写成事实、或把“未验证”写成“已通过”的地方；
3. 指导书报告要求（`/Users/wesley/大三上课程/操作系统/实验指导书/lab2/lab2_5_labs_requirement.md` 第 1～7 条，以及 `lab2_2_1_exercise.md`、`lab2_4_qemu_mmu_debug.md` 中每个练习提出的问题）没有回答到的地方；
4. 初学者答辩时容易被追问却讲不清的表述（术语没解释、跳步）。

输出：`report/ai_logs/lab2/evidence/t15_report_review.md`，每条一行：`[MUST_FIX|SHOULD_FIX|NOTE] 文件:行号 — 问题 — 依据`；最后一行 `结论：PASS` 或 `结论：NEEDS_FIX`。只报告有依据的问题，不确定的标 NOTE。回复 ≤10 行摘要。

## RELY

- 被审文件：`report/report.md`、`report/ai_logs/lab2/分支任务演示步骤.md`；提示词原文 `report/prompt.md`。
- 证据：`report/ai_logs/lab2/`（`overview.md`、`task_01.md`～`task_14.md`、`bugs.md`、`final_summary.md`、`evidence/`）。
- 最终验证：`evidence/final_diff_check.txt`、`final_qemu_default.txt`、`final_qemu_LAB2_PMM_BUDDY.txt`、`final_qemu_LAB2_SLUB_TEST.txt`、`final_grade.txt`。

## GUARANTEE

- 不修改任何已有文件；只新建 `t15_report_review.md`。

## SPECIFICATION

- Postconditions：结果文件存在，最后一行为 PASS 或 NEEDS_FIX。
- Edge cases：报告引用的证据文件不存在或链接错误时记为 MUST_FIX。

## Agent Result

Codex 回复（2026-10-24）：`evidence/t15_report_review.md`，结论 **PASS**；2 条 NOTE——① 演示稿准备步骤没有提醒先 `./os2026.sh clean`（Makefile 不会因为 `DEFS` 变化自动重编，沿用扩展配置的目标文件会让 ELF 与历史地址不同）；② `report.md` 只提 SUM/MXR 而没解释含义。

## Verification

Leader 采纳两条：演示稿终端 1 在 `debug` 前加 `./os2026.sh clean </dev/null` 并写明原因；`report.md` 第 1 问关键分支处补 SUM、MXR 的一句话解释（依据 QEMU `cpu_helper.c:268-282`）。

状态：**DONE**。
