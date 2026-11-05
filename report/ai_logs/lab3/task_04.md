# Task 04：代码审查（Code Review）与综合评测验证

## PROMPT

你是 OS Lab 3 的 Review & Verification Agent。请对照 `task_01.md`、`task_02.md`、`task_03.md` 中的 `GUARANTEE` 与 `SPECIFICATION`，以及实验指导书 `lab3_2_1_exercise.md`、`lab3_4_lab_requirement.md` 的约束，对 `code/` 下的全部改动执行静态代码审查与运行时评测核验。

重点核查项：
1. 是否只修改了规定的文件（`code/kern/mm/pmm.c`、`code/kern/mm/best_fit_pmm.c`、`code/kern/trap/trapentry.S`、`code/kern/trap/trap.c`）？
2. 所有 `YOUR CODE` 注释是否均已替换为学号 `2412172`，且保留了 `lab2` / `lab3` 标识注释？
3. `RESTORE_ALL` 是否严格与 `SAVE_ALL` 对称？是否正确处理了 `x2(sp)`、`sstatus`、`sepc`？
4. `interrupt_handler` 和 `exception_handler` 是否满足持续时钟中断、输出统计、满次数关机、断点与非法指令动态长度步进（2 字节压缩指令 vs 4 字节标准指令）以及未知中断/异常兜底输出？

## RELY

- `evidence/t1_mm.diff`、`evidence/t2_trapentry.diff`、`evidence/t3_trap.diff`
- `evidence/build_output.txt`、`evidence/final_qemu.txt`、`evidence/final_grade.txt`
- `evidence/grade_context_out.txt`、`evidence/grade_illegal_out.txt`、`evidence/grade_timer_out.txt`

## GUARANTEE

- 确认无违反 Spec 或课程约束的代码改动；
- 确认 `make grade` 满分 `100/100`，`make qemu` 正常打印 10 次 `100 ticks` 并关机退出。

## SPECIFICATION

- 逐项核对 `git status` 与 `git diff`，输出结构化审查结论。

## Agent Result & Verification

1. **文件范围与注释合规性检查**：
   - `git status -s` 仅包含 `code/kern/mm/pmm.c`、`code/kern/mm/best_fit_pmm.c`、`code/kern/trap/trapentry.S`、`code/kern/trap/trap.c` 四个源码文件，未触碰 `SAVE_ALL`、`struct trapframe`、`trap()` 签名、`Makefile` 或 `tools/*`。
   - 学号注释已全部替换为 `//lab2 function1 2412172`、`//lab2 function2-2 2412172`、`#lab3 function1 2412172`（两处）、`//lab3 function2 2412172`。
2. **`trapentry.S` 审查**：
   - `RESTORE_ALL` 先通过 `s1`、`s2` 恢复 `sstatus`、`sepc`，随后恢复 `x1`、`x3..x31`（覆盖借用的 `s1`、`s2`），最后恢复 `x2(sp)`；`__alltraps` 通过 `move a0, sp` 和 `jal trap` 传递 `struct trapframe *` 参数，返回后顺序进入 `__trapret` 并执行 `sret`，逻辑严密。
3. **`trap.c` 审查**：
   - `riscv_inst_len(tf->epc)` 通过 `((*(const uint16_t *)epc & 0x3) != 0x3) ? 2 : 4` 准确识别 16 位 RVC 压缩指令与 32 位标准指令；
   - `interrupt_handler` 正确重设下一次时钟事件 `clock_set_next_event()`、累加 `ticks`、每 `TICK_NUM (100)` 次调用 `print_ticks()`，并在累计打印达 `SHUTDOWN_AFTER (10)` 次时调用 `sbi_shutdown()`；
   - 全部 3 项自动评测得分为 `35/35 + 25/25 + 40/40 = 100/100`。
- **结论**：`DONE`（通过全部审查与实测）。
