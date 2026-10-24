# Task 03：交叉 Review（page_init ↔ Best-Fit）

执行者：Codex w1:p2 审 Best-Fit（task_02），Codex w1:p3 审 page_init（task_01）。
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Review Agent。只读审查，**不要修改任何文件**（只允许新建下面指定的 review 结果文件），不要 git commit / push。

背景：Task 01（page_init）和 Task 02（Best-Fit）已实现，Leader 已在 `code/kern/mm/pmm.c` 的 `init_pmm_manager()` 把管理器切换为 `&best_fit_pmm_manager`，并在 Docker 镜像 os2026-dbg 中实际运行：`make qemu` 输出见 `evidence/t3_qemu.txt`，`make grade` 结果 20/20 见 `evidence/t3_grade.txt`。你所在的沙箱无法访问 Docker，所以不要尝试运行编译或 QEMU；只做代码审查。

任务：对照被审任务 `task_0X.md` 中的 GUARANTEE 和 SPECIFICATION，逐条审查 `git -C /Users/wesley/大三上课程/操作系统/os2026-labs diff code/` 中对应文件的实现。重点：
- 是否有违反 SPECIFICATION 的分支（含 edge cases）；
- 虚拟地址 / 物理地址 / 页号 / 描述符下标是否混用；
- 链表、`nr_free`、`PG_property`、`PG_reserved` 的不变量是否在每个分支都保持；
- 测试没有覆盖、但 spec 要求的行为。

输出：把结论写入 `<lab2 日志目录>/evidence/t3_review_<被审对象>.md`（page_init 或 best_fit），格式：每条发现一行，`[MUST_FIX|SHOULD_FIX|NOTE] 文件:行号 — 问题 — 依据（spec 的哪一条）`；最后一行写 `结论：PASS` 或 `结论：NEEDS_FIX`。回复 ≤10 行摘要。不要把推测写成确定结论，不确定的标 NOTE。

## RELY

- 被审规格：`task_01.md`（page_init）、`task_02.md`（Best-Fit）。
- 运行证据：`evidence/t3_qemu.txt`、`evidence/t3_grade.txt`。
- 原始骨架（只读）：`/Users/wesley/大三上课程/操作系统/课程代码包/lab2.zip`。

## GUARANTEE

- 不修改任何源码和已有日志文件；只新建一个 review 结果文件。
- 每条发现都给出文件行号和 spec 依据。

## SPECIFICATION

- Preconditions：工作区包含 Task 01、02 的实现和 Leader 的一行切换改动。
- Postconditions：review 文件存在，最后一行为 PASS 或 NEEDS_FIX。
- Edge cases：发现测试无法覆盖的问题时照样报告；没有发现问题时写明“未发现违反 spec 的问题”并 PASS。

## Agent Result

- w1:p2 审 Best-Fit → `evidence/t3_review_best_fit.md`：**PASS**。未发现违反 spec 的问题；NOTE：现有两个自检没有专门构造“同长度取低地址”“三处 n==0 断言”“总空闲页够但最大单块不够”“前后两侧同时合并”这几种情况，这些分支只经过静态检查。
- w1:p3 审 page_init → `evidence/t3_review_page_init.md`：**NEEDS_FIX**。MUST_FIX：`page_init` 在把全部描述符置 `PG_reserved` 之前没有确认描述符数组本身落在内存内；以本次 `end=0xffffffffc0206078` 推算，若 size=0x208000，描述符物理区间 `[0x80207000, 0x8020c140)` 超过内存末端 `0x80208000`，循环会写越界内存。

## Verification

Leader 复核 MUST_FIX：推导成立，是 Task 01 的 SPECIFICATION 漏掉了“描述符数组放不下”这一分支（Case 3 只考虑了“数组放得下但后面没有完整页”）。这是 Leader 写 spec 时的遗漏，记为 bugs.md Bug 2，派 Task 04 修复。Best-Fit 的 NOTE 写进报告“改进空间 / 测试覆盖”部分。

状态：Best-Fit review **DONE**（PASS）；page_init review **DONE**（NEEDS_FIX → Task 04）。
