# Task 06：按 Review 01 修订报告

执行者：Codex（Report Agent，Herdr 面板 w9:p7）

## PROMPT

你是 OS Lab1 的 Report Agent。请按 Review 结果修订 `report/report.md`（仓库 `/home/wesly/courses/os2026`，Windows 路径 `\\wsl$\Ubuntu-24.04\home\wesly\courses\os2026`）。先完整阅读 `report/ai_logs/lab1/review_01.md`。

只做以下 4 处修改，其他内容不动：
1. Review 问题 1（第 266 行附近“报告生成前后对禁止修改的已有文件做内容哈希核对”）：改为有据可查的表述——Task 02、Task 03 的执行脚本用 sha256 清单校验仓库原有文件未被改动；删掉“报告生成前后”的说法。同一段最后的“本任务不执行 commit 或 push”改为：“代码与报告最终提交到仓库 `lab1` 分支。”
2. Review 问题 2（第 258 行表格“修复后 timeout 10 make qemu”一行）：把“timeout 退出码 124 为正常”改为“被 `timeout` 终止（QEMU 日志 `terminating on signal 15 ... (timeout)`），属预期结束方式”。
3. 第二章 AI 工具表 pi 那一行的备注改为：“Review：审查报告与代码改动，见 [review_01.md](ai_logs/lab1/review_01.md)”。
4. 第六章“AI 协作开发的经验”列表：把 Task 04 条目中“pi 被指定承担 Review，但目前没有可引用的该轮复核结果。”删除，并在 Task 04 之后新增两条：
   - `[Task 05](ai_logs/lab1/task_05.md)：Review Agent（pi）逐项对照 evidence 审查报告，结论 NEEDS_FIX，提出 2 项表述问题（无记录的哈希核对说法、借用 Task 01 的退出码），未发现数据错误；见 [review_01.md](ai_logs/lab1/review_01.md)。`
   - `[Task 06](ai_logs/lab1/task_06.md)：Report Agent 按 Review 修订上述表述。`

允许修改：仅 `report/report.md`。禁止：修改其他文件；git commit/push。
输出：回复 ≤6 行，列出 4 处修改是否完成。

## RELY

- `report/ai_logs/lab1/review_01.md` 问题清单与建议。
- `evidence/t2_qemu.txt` 末行：`qemu-system-riscv64: terminating on signal 15 from pid 467 (timeout)`。
- 哈希校验依据：Review 01 问题 1 依据第 2 点（`os2026-t2-run.sh`、`os2026-t3-run.sh` 中的 sha256 清单与 `PASS: all original repository files unchanged`）。

## GUARANTEE

- 4 处修改全部完成，报告其余文字不变。

## SPECIFICATION

- Preconditions：Review 01 已完成。
- Postconditions：`report.md` 中不再出现“报告生成前后”“退出码 124”“未见该轮 Review 结果”“目前没有可引用的该轮复核结果”“本任务不执行 commit 或 push”。

## Agent Result

Codex 回复（2026-10-07 15:13）：4 处修改全部完成，仅改 `report/report.md`。

## Verification

Leader 检索 `report.md`：“报告生成前后”“退出码 124”“未见该轮 Review”“目前没有可引用”“本任务不执行 commit” 均已不存在；第 35、258、266、293–294 行为新表述。

状态：**DONE**。
