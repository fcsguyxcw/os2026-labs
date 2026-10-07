# Final Status

Completed Tasks: Task 01–06（构建与地址验证、Bug 1 修复、GDB 四阶段采集、截图与报告、Review、修订）

Tests:
- `make clean && make`：通过（evidence/t1_build.txt）
- `make qemu`：修复后输出 `(THU.CST) os is loading ...`，`Domain0 Next Address : 0x0000000080200000`（evidence/t2_qemu.txt）
- GDB：复位 0x1000 → OpenSBI 0x80000000 → kern_entry 0x80200000 → kern_init 0x8020000a，全部命中（evidence/t3_*.txt）
- `make grade`：未执行，压缩包缺 tools/grade.sh

Remaining Problems:
- 报告中学号、姓名待填写。
- 截图为证据原文渲染图，不是桌面截屏。
- 旧环境（QEMU 4.1.1 / OpenSBI v0.4）行为未实测，报告中已标注。

AI Collaboration Summary: Leader 写规格并逐项复核原始证据；Codex 执行 5 个任务；pi 完成 1 轮 Review，发现 2 项表述问题并已修订。Bug 1（OpenSBI fw_dynamic 下一阶段地址为 0）由 Leader 从日志定位、Codex 修复验证。

Files Changed:
- code/Makefile（3 行：gdb-multiarch；qemu/debug 改为 `-kernel $(UCOREIMG)`）
- report/report.md、report/prompt.md、report/images/01–06.png
- report/ai_logs/lab1/*

Status: COMPLETE
