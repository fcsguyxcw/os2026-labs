# Task 02：功能模块一 —— Trap 上下文恢复（`RESTORE_ALL`）与 Trap 入口（`__alltraps`）

## PROMPT

你是 OS Lab 3 的 Implementation Agent。工作目录为 `/Users/wesley/大三上课程/操作系统/os2026-labs`（分支 `lab3`），实验源码位于 `code/`。
请修改 `code/kern/trap/trapentry.S`，在两处 `#lab3 function1 YOUR CODE` 注释位置（将注释替换为 `#lab3 function1 2412172`）分别实现：
1. 汇编宏 `RESTORE_ALL`：与已提供的 `SAVE_ALL` 对称，从当前内核栈顶 `sp` 指向的 `struct trapframe` 中恢复处理器状态。
2. 汇编入口 `__alltraps` 中 `SAVE_ALL` 之后到 `__trapret` 之前的代码：按 RISC-V 调用约定将当前 `struct trapframe *` 传递给 C 函数 `void trap(struct trapframe *tf)` 并调用之。

操作约束：
- 只允许修改 `code/kern/trap/trapentry.S` 中两处 `#lab3 function1 YOUR CODE` 所在位置。
- 严禁修改 `SAVE_ALL`、`__trapret`、`sret`、`code/kern/trap/trap.h` 中的 `struct pushregs` / `struct trapframe` 定义或 `trap()` 函数签名。

## RELY

- `code/kern/trap/trap.h` 中的 `struct trapframe` 内存布局（共 36 个 `uintptr_t`，每个占 `REGBYTES = 8` 字节）：
  - 偏移 `0*REGBYTES` ~ `31*REGBYTES`：`struct pushregs gpr`（依次对应 `x0/zero`, `x1/ra`, `x2/sp`, `x3/gp`, `x4/tp`, `x5..x7/t0..t2`, `x8..x9/s0..s1`, `x10..x17/a0..a7`, `x18..x27/s2..s11`, `x28..x31/t3..t6`）。
  - 偏移 `32*REGBYTES`：`uintptr_t status`（保存 `sstatus`）。
  - 偏移 `33*REGBYTES`：`uintptr_t epc`（保存 `sepc`）。
  - 偏移 `34*REGBYTES`：`uintptr_t badvaddr`（保存 `sbadaddr`/`stval`）。
  - 偏移 `35*REGBYTES`：`uintptr_t cause`（保存 `scause`）。
- `code/kern/trap/trapentry.S` 中已有的 `SAVE_ALL` 实现：
  - 先执行 `csrw sscratch, sp` 保存原栈顶，再执行 `addi sp, sp, -36 * REGBYTES` 分配 `trapframe`；
  - 将 `x0..x1`、`x3..x31` 存入对应偏移；
  - 用 `csrrw s0, sscratch, x0` 取出原栈顶存入 `2*REGBYTES(sp)`，并将 `sstatus`、`sepc`、`sbadaddr`、`scause` 存入 `32..35*REGBYTES(sp)`。
- `code/libs/riscv.h`：64 位下 `LOAD` 宏定义为 `ld`，`STORE` 宏定义为 `sd`，`REGBYTES` 为 `8`。
- `code/tools/test_lab3_context.S`：`test_lab3_register_context` 在触发 `ebreak` 前向 `t0..t2`、`s0..s1`、`a1..a7`、`s2..s11`、`t3..t6` 写入特征值，在从 Trap 返回后逐一比对这些寄存器是否完好恢复。

## GUARANTEE

- `__alltraps` 在 `SAVE_ALL` 后将指向当前栈顶 `trapframe` 的指针放入 `a0`（`mv a0, sp` / `move a0, sp`），并调用 `jal trap`；`trap()` 返回时 `sp` 仍指向该 `trapframe` 起始地址，随后顺序落入 `__trapret`。
- `RESTORE_ALL` 正确恢复 `sstatus` 和 `sepc` CSR（包括 `trap()` 中对 `tf->epc` 的修改），正确恢复除 `x0` 外的全部通用寄存器 `x1..x31`，且最后一步恢复 `x2(sp)`，使 `sp` 精确回到 Trap 发生前的值。
- 通过 `make grade` 中的 Test 1（`[Test 1] Trap entry and breakpoint return: PASS (35/35)`）。

## SPECIFICATION

- **`__alltraps` 调用约定规格**：
  - Preconditions：`SAVE_ALL` 已执行完毕，`sp` 指向栈上刚构建好的 `struct trapframe` 首地址。
  - Postconditions：第一参数寄存器 `a0 == sp`，调用 `jal trap` 进入 C 函数 `trap(struct trapframe *tf)`；由于 `sp` 是被调用者保存（Callee-saved）寄存器，`trap` 返回后 `sp` 不变，紧接着进入 `__trapret`。
- **`RESTORE_ALL` 恢复顺序与寄存器约束规格**：
  - Preconditions：`sp` 指向当前 `struct trapframe` 首地址；C 处理函数可能已经更新了 `33*REGBYTES(sp)` 处的 `tf->epc` 或 `32*REGBYTES(sp)` 处的 `tf->status`。
  - Postconditions：
    1. **先恢复 CSR，再恢复借用的通用寄存器**：由于 RISC-V 不能直接从内存写 CSR，需先用通用寄存器（如 `s1`、`s2`）从 `32*REGBYTES(sp)` 和 `33*REGBYTES(sp)` 加载 `status` 和 `epc`，执行 `csrw sstatus, s1` 和 `csrw sepc, s2`（`sbadaddr` 与 `scause` 是硬件进入 Trap 时写入的只读现场报告，`sret` 返回时无需写回）。
    2. **恢复通用寄存器 `x1`、`x3..x31`**：依次使用 `LOAD` 从 `1*REGBYTES(sp)`、`3*REGBYTES(sp)` .. `31*REGBYTES(sp)` 恢复各通用寄存器（这会覆盖第 1 步临时借用的 `s1`、`s2`）。`x0` 恒为 0 无需恢复。
    3. **最后恢复 `x2(sp)`**：必须把 `LOAD x2, 2*REGBYTES(sp)` 放在整个宏的最后一条指令执行，因为一旦修改 `x2(sp)`，后续基于 `(sp)` 的偏移寻址基准就会改变；而 `2*REGBYTES(sp)` 中保存的正是 `SAVE_ALL` 分配 36 个寄存器空间之前的原始 `sp`，直接加载即可一步完成栈指针恢复与 TrapFrame 栈空间释放。

## Agent Result

1. 在 `code/kern/trap/trapentry.S` 的 `RESTORE_ALL` 宏（`#lab3 function1 2412172`）中：
   - 先使用 `LOAD s1, 32*REGBYTES(sp)` 与 `LOAD s2, 33*REGBYTES(sp)` 取出栈上保存（并可能被 C 处理函数修改过）的 `status` 和 `epc`，通过 `csrw sstatus, s1` 和 `csrw sepc, s2` 写回 CSR 寄存器。
   - 随后依次执行 `LOAD x1, 1*REGBYTES(sp)` 及 `LOAD x3..x31` 恢复除 `x0` 和 `x2` 外的全部通用寄存器（同时覆盖了刚才借用的 `s1`、`s2`）。
   - 最后执行 `LOAD x2, 2*REGBYTES(sp)` 恢复原始栈指针 `sp`，一步完成栈帧回收与 `sp` 还原。
2. 在 `__alltraps`（`#lab3 function1 2412172`）中：
   - 执行 `move a0, sp` 将当前栈顶 `struct trapframe *` 作为第一参数传入 `a0`，随后执行 `jal trap` 调用 C 语言入口 `trap(struct trapframe *tf)`，返回后顺序落入 `__trapret`。

## Verification

- 编译验证：`os2026.sh build` 零警告通过汇编与链接（见 `evidence/build_output.txt`）。
- 单元测试验证：`make grade` 中 Test 1（`trap entry, register restore, and breakpoint EPC`）检验了 `ebreak` 前后 26 个通用寄存器上下文是否被分毫不差地保存和恢复，结果 `PASS (35/35)`（见 `evidence/final_grade.txt` 与 `evidence/grade_context_out.txt`）。
- 差异补丁：已保存至 `evidence/t2_trapentry.diff`。

### 给初学者的大白话原理解释

当 CPU 突然遇到中断或异常（Trap）时，就像你正在书桌上写作业突然要去接电话：为了回来能接着写，你得先把桌面上的笔、草稿纸和刚写到第几行全部拍照存进一个叫 `trapframe` 的档案袋里，这就是 `SAVE_ALL`。拍完照后，`__alltraps` 把档案袋的地址递给 C 语言的 `trap()` 函数（`move a0, sp; jal trap`），请它按章办事。等 C 函数处理完（比如把“写到第几行”往后挪一行以跳过断点指令），`RESTORE_ALL` 就照着档案袋把所有东西原封不动摆回桌面，最后一步再把装着档案袋的抽屉合上（恢复 `x2/sp`），通过 `sret` 指令回到刚才被打断的地方继续运行。之所以必须最后恢复 `sp`，是因为档案袋本身就放在 `sp` 指着的桌面上，如果先把 `sp` 改走了，剩下的寄存器就找不到档案袋在哪儿了。
