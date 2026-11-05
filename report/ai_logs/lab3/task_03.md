# Task 03：功能模块二 —— 中断与异常处理功能（`interrupt_handler` 与 `exception_handler`）

## PROMPT

你是 OS Lab 3 的 Implementation Agent。工作目录为 `/Users/wesley/大三上课程/操作系统/os2026-labs`（分支 `lab3`），实验源码位于 `code/`。
请修改 `code/kern/trap/trap.c`，在 `//lab3 function2 YOUR CODE` 注释位置（将注释替换为 `//lab3 function2 2412172`）实现以下两个函数：
```c
void interrupt_handler(struct trapframe *tf);
void exception_handler(struct trapframe *tf);
```

操作约束：
- 只允许在 `code/kern/trap/trap.c` 的 `//lab3 function2 YOUR CODE` 区域内编写代码（包括必要的 `#include <sbi.h>`、宏定义 `#ifndef SHUTDOWN_AFTER` 及静态辅助函数）。
- 严禁修改函数签名，严禁修改 `trap.c` 中规定以外的其他已有函数（`print_ticks`、`idt_init`、`trap_in_kernel`、`print_trapframe`、`print_regs`、`trap_dispatch`、`trap`）。

## RELY

- `code/kern/trap/trap.c` 已有定义：
  - `#define TICK_NUM 100`
  - `static void print_ticks(void)`：打印 `"%d ticks\n", TICK_NUM`（即 `"100 ticks\n"`）。
  - `void print_trapframe(struct trapframe *tf)`：打印完整 `trapframe`，其中包含 `"  cause    0x%08x\n", tf->cause`。
  - `trap_dispatch(struct trapframe *tf)`：当 `(intptr_t)tf->cause < 0` 时调用 `interrupt_handler(tf)`，否则调用 `exception_handler(tf)`。
- `code/kern/driver/clock.h` & `clock.c`：
  - `extern volatile size_t ticks;`
  - `void clock_set_next_event(void);`
- `code/libs/sbi.h`：`void sbi_shutdown(void);`
- `code/libs/riscv.h` 中断与异常编码宏：
  - 中断号：`IRQ_U_SOFT (0)`、`IRQ_S_SOFT (1)`、`IRQ_H_SOFT (2)`、`IRQ_M_SOFT (3)`、`IRQ_U_TIMER (4)`、`IRQ_S_TIMER (5)`、`IRQ_H_TIMER (6)`、`IRQ_M_TIMER (7)`、`IRQ_U_EXT (8)`、`IRQ_S_EXT (9)`、`IRQ_H_EXT (10)`、`IRQ_M_EXT (11)`。
  - 异常号：`CAUSE_MISALIGNED_FETCH (0x0)`、`CAUSE_FAULT_FETCH (0x1)`、`CAUSE_ILLEGAL_INSTRUCTION (0x2)`、`CAUSE_BREAKPOINT (0x3)`、`CAUSE_MISALIGNED_LOAD (0x4)`、`CAUSE_FAULT_LOAD (0x5)`、`CAUSE_MISALIGNED_STORE (0x6)`、`CAUSE_FAULT_STORE (0x7)`、`CAUSE_USER_ECALL (0x8)`、`CAUSE_SUPERVISOR_ECALL (0x9)`、`CAUSE_HYPERVISOR_ECALL (0xa)`、`CAUSE_MACHINE_ECALL (0xb)`。
- 评测脚本 `code/tools/grade.sh` 与测试源码 `code/tools/test_lab3.c` 的精确匹配要求：
  - Test 1 检查输出包含 `"Breakpoint at"`，且 `ebreak` 异常返回后继续执行后续指令。
  - Test 2 执行 `.word 0x0000007b`（4 字节非法指令），随后直接调用：
    - `tf.cause = IRQ_S_SOFT | ((uintptr_t)1 << 63); interrupt_handler(&tf);`
    - `tf.cause = 15; interrupt_handler(&tf);`
    - `tf.cause = 20; exception_handler(&tf);`
    并检查输出必须同时包含：
    - `"Illegal instruction at"`
    - `"Supervisor software interrupt"`
    - `"Unknown interrupt: cause 15"`
    - `"cause    0x00000014"`（由 `print_trapframe(tf)` 输出 `tf->cause == 20` 产生）
  - Test 3 等待 `ticks >= 200`，检查输出包含 `"100 ticks"` 与 `"[Test 3] Sustained supervisor timer: PASS (40/40, ticks=200)"`。

## GUARANTEE

- `interrupt_handler` 和 `exception_handler` 为非 `static` 函数（因为 `test_lab3.c` 通过 `extern void interrupt_handler(struct trapframe *tf);` 和 `extern void exception_handler(struct trapframe *tf);` 直接调用它们）。
- 编译无任何警告（`-Werror`）。
- `make grade` 三个测试全部 PASS，总分 `100/100`。
- `make qemu` 正常启动，每秒输出一行 `100 ticks`，累计输出达到 `SHUTDOWN_AFTER`（默认定义为 `10`）次后调用 `sbi_shutdown()` 正常关机退出。

## SPECIFICATION

- **指令长度计算辅助逻辑**：
  - 在 RISC-V 架构（含 C 压缩指令扩展）中，指令长度由指令机器码最低 2 位决定：读取 `*(const uint16_t *)(tf->epc)`，若 `(inst & 0x3) != 0x3` 则为 2 字节压缩指令（如 `c.ebreak`），步进长度为 `2`；若 `(inst & 0x3) == 0x3` 则为 4 字节标准 32 位指令（如 32 位 `ebreak` 或 `.word 0x0000007b`），步进长度为 `4`。
- **`interrupt_handler(struct trapframe *tf)`**：
  - Preconditions：`tf` 非空；可能由硬件中断经 `trap_dispatch` 调用（此时 `tf->cause` 最高位为 1），也可能由单元测试直接构造 `tf.cause` 调用（最高位可能为 1 或 0）。
  - Postconditions：
    - 提取中断原因码 `intptr_t cause = (tf->cause << 1) >> 1;`（注意使用无符号移位或掩码 `uintptr_t cause = (tf->cause << 1) >> 1;` 清除最高位符号位，确保得到非负中断编号）。
    - `IRQ_S_TIMER`：
      1. 调用 `clock_set_next_event()` 设置下一次时钟中断；
      2. `ticks++`；当 `ticks % TICK_NUM == 0` 时调用 `print_ticks()`，并递增打印轮数计数 `num`；
      3. 当累计打印次数达到 `SHUTDOWN_AFTER`（`#ifndef SHUTDOWN_AFTER` 定义为 `10`）时，调用 `sbi_shutdown()` 关机。
    - 其他已知中断类型分别输出对应提示（如 `IRQ_U_SOFT` -> `"User software interrupt\n"`，`IRQ_S_SOFT` -> `"Supervisor software interrupt\n"`，`IRQ_H_SOFT` -> `"Hypervisor software interrupt\n"`，`IRQ_M_SOFT` -> `"Machine software interrupt\n"`，`IRQ_U_TIMER` -> `"User timer interrupt\n"`，`IRQ_H_TIMER` -> `"Hypervisor timer interrupt\n"`，`IRQ_M_TIMER` -> `"Machine timer interrupt\n"`，`IRQ_U_EXT` -> `"User external interrupt\n"`，`IRQ_S_EXT` -> `"Supervisor external interrupt\n"`，`IRQ_H_EXT` -> `"Hypervisor external interrupt\n"`，`IRQ_M_EXT` -> `"Machine external interrupt\n"`）。
    - `default`（无法识别的中断，如 `cause == 15`）：先输出 `cprintf("Unknown interrupt: cause %d\n", (int)cause);`，再调用 `print_trapframe(tf);`。
- **`exception_handler(struct trapframe *tf)`**：
  - Preconditions：`tf` 非空，`tf->cause` 为异常原因码。
  - Postconditions：
    - `CAUSE_ILLEGAL_INSTRUCTION`：输出 `"Exception type: Illegal instruction\n"` 及 `"Illegal instruction at 0x%08x\n", tf->epc`，然后根据 `tf->epc` 处指令编码长度将 `tf->epc` 增加 2 或 4。
    - `CAUSE_BREAKPOINT`：输出 `"Exception type: breakpoint\n"` 及 `"Breakpoint at 0x%08x\n", tf->epc`，然后根据 `tf->epc` 处指令编码长度将 `tf->epc` 增加 2 或 4。
    - 其他未要求具体处理的异常（包括 `default`）：调用 `print_trapframe(tf);` 进行兜底输出。

## Agent Result

1. 在 `code/kern/trap/trap.c` 的 `//lab3 function2 2412172` 处引入 `<sbi.h>` 与 `#ifndef SHUTDOWN_AFTER`（默认为 `10`），并实现了静态辅助函数 `riscv_inst_len(uintptr_t epc)`，通过读取 `*(const uint16_t *)epc` 的最低两位判断指令为 16 位压缩指令（`+2`）还是 32 位标准指令（`+4`）。
2. 实现了 `interrupt_handler(struct trapframe *tf)`：
   - 用 `intptr_t cause = (tf->cause << 1) >> 1;` 剥离最高位中断标志位。
   - 在 `IRQ_S_TIMER` 分支中调用 `clock_set_next_event()` 预约下一次时钟中断，递增 `ticks`，每满 `TICK_NUM (100)` 次调用 `print_ticks()` 并累加打印次数 `num`，当 `num == SHUTDOWN_AFTER (10)` 时调用 `sbi_shutdown()` 正常关机。
   - 其余已知中断打印对应描述字符串，`default` 分支输出 `"Unknown interrupt: cause %d\n"` 并调用 `print_trapframe(tf)`。
3. 实现了 `exception_handler(struct trapframe *tf)`：
   - 在 `CAUSE_ILLEGAL_INSTRUCTION` 分支输出 `"Exception type: Illegal instruction\n"` 与 `"Illegal instruction at 0x%08x\n", tf->epc`，并将 `tf->epc += riscv_inst_len(tf->epc)`。
   - 在 `CAUSE_BREAKPOINT` 分支输出 `"Exception type: breakpoint\n"` 与 `"Breakpoint at 0x%08x\n", tf->epc`，并将 `tf->epc += riscv_inst_len(tf->epc)`。
   - 其余异常分支及 `default` 均调用 `print_trapframe(tf)` 进行兜底输出。

## Verification

- 编译验证：`os2026.sh build` 零警告通过编译（见 `evidence/build_output.txt`）。
- QEMU 计时与关机验证：`os2026.sh qemu` 每 100 次时钟中断打印一次 `100 ticks`，累计打印 10 次后自动触发 `sbi_shutdown()` 退出（见 `evidence/final_qemu.txt`）。
- 评测脚本验证：`make grade` 三个测试项（Test 1 断点返回 35/35、Test 2 非法指令与未知中断/异常兜底 25/25、Test 3 持续时钟中断 40/40）全部 PASS，总分 `100/100`（见 `evidence/final_grade.txt`、`evidence/grade_context_out.txt`、`evidence/grade_illegal_out.txt`、`evidence/grade_timer_out.txt`）。
- 差异补丁：已保存至 `evidence/t3_trap.diff`。

### 给初学者的大白话原理解释

如果把 `__alltraps` 比作医院的分诊台，那么 `interrupt_handler` 和 `exception_handler` 就是急诊科和门诊科的专科医生。当外部的“闹钟”响了（`IRQ_S_TIMER` 时钟中断），急诊医生不仅要在记事本上打个勾（`ticks++`），还得赶紧定好下一次闹钟（`clock_set_next_event`），否则闹钟只响一次就再也不响了；每攒够 100 个勾就报一次时，报满 10 次就调用 `sbi_shutdown()` 下班关机。而当程序自己踩到了“断点”（`ebreak`）或念错了“咒语”（非法指令）进门诊时，硬件登记的 `sepc` 还停留在出错的那条指令上——如果医生光打印诊断书却不把 `tf->epc` 往后挪一条指令的长度（RISC-V 压缩指令 2 字节、普通指令 4 字节），CPU 出院回去又会立刻再踩同一颗雷，陷入无限死循环。
