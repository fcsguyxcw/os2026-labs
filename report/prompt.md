# Lab 3 提示词汇总（PROMPT / RELY / GUARANTEE / SPECIFICATION）

本文件汇总 Lab 3 在依赖迁移、功能模块一（Trap 上下文恢复与入口）、功能模块二（中断与异常处理）以及代码审查验证中所使用的全部四段式提示词。

---

<a id="task01"></a>

## 一、前置依赖任务：迁移 Lab 2 物理内存管理实现（`page_init` 与 `best_fit_pmm_manager`）

对应过程记录：[task_01.md](ai_logs/lab3/task_01.md)

````markdown
## PROMPT

你是 OS Lab 3 的 Implementation Agent。工作目录为 `/Users/wesley/大三上课程/操作系统/os2026-labs`（分支 `lab3`），实验源码位于 `code/`。
根据实验指导书要求：“本实验依赖实验 2。请使用你此前设计的提示词生成相应实验代码，并将生成结果填写到本实验代码中标有 `lab2` 注释的对应位置，确保代码能够正确编译。”

请在以下两个位置填入 Lab 2 的实现代码：
1. `code/kern/mm/pmm.c`：
   - 将 `init_pmm_manager()` 中的 `pmm_manager = &default_pmm_manager;` 切换为 `pmm_manager = &best_fit_pmm_manager;`；
   - 在 `//lab2 请补充你在lab2中的代码` 处（将该注释改为 `//lab2 function1 2412172`）实现 `static void page_init(void)`。
   - **严禁修改** `pmm.c` 中已有的 `alloc_pages()`、`free_pages()`、`nr_free_pages()`（它们在 Lab 3 中已包含 `local_intr_save` / `local_intr_restore` 原子保护逻辑）以及 `pmm_init()`、`check_alloc_page()`。
2. `code/kern/mm/best_fit_pmm.c`：
   - 在 `//lab2 请补充你在lab2中的代码` 处（将该注释改为 `//lab2 function2-2 2412172`）实现 Best-Fit 物理内存分配器及 `const struct pmm_manager best_fit_pmm_manager` 定义。
   - **严禁修改** `basic_check()` 和 `best_fit_check()`。

## RELY

- `code/kern/mm/pmm.c` 中已声明全局变量：`struct Page *pages;`、`size_t npage = 0;`、`uint64_t va_pa_offset;`、`const size_t nbase = DRAM_BASE / PGSIZE;`、`const struct pmm_manager *pmm_manager;`。
- `code/kern/driver/dtb.h`：`uint64_t get_memory_base(void);`、`uint64_t get_memory_size(void);`。
- `code/kern/mm/memlayout.h`：`KERNBASE`、`KMEMSIZE`、`KERNTOP`、`PHYSICAL_MEMORY_OFFSET`、`struct Page`、`SetPageReserved`、`ClearPageReserved`、`PageReserved`、`SetPageProperty`、`ClearPageProperty`、`PageProperty`、`free_area_t`、`le2page`。
- `code/kern/mm/pmm.h`：`PADDR(kva)`、`pa2page(pa)`、`page2pa(page)`、`set_page_ref`、`struct pmm_manager`。
- `code/kern/mm/mmu.h`：`PGSIZE`、`PPN`；`code/libs/defs.h`：`ROUNDUP`、`ROUNDDOWN`；`code/libs/list.h`：双向循环链表操作 `list_init`、`list_add_before`、`list_add_after`、`list_del`、`list_next`、`list_prev`、`list_empty`。

## GUARANTEE

- `code/kern/mm/pmm.c` 与 `code/kern/mm/best_fit_pmm.c` 编译零警告（`-Werror`）。
- 内核启动时 `pmm_init()` 正常输出 `memory management: best_fit_pmm_manager`、`physical memory map:` 并通过 `check_alloc_page() succeeded!` 断言自检。
- 保留 `pmm.c` 中 Lab 3 新增的 `<../sync/sync.h>` 头文件及 `local_intr_save` / `local_intr_restore` 临界区保护不变。

## SPECIFICATION

- `page_init()`：
  - Preconditions：`dtb_init()` 与 `init_pmm_manager()` 已完成。
  - Postconditions：校验 `mem_base == DRAM_BASE` 且 `mem_size > 0`；打印物理内存映射范围；设置 `va_pa_offset = PHYSICAL_MEMORY_OFFSET`；将 `mem_end` 按可管理物理内存上界截断并向下页对齐，设置 `npage = PPN(mem_end)`；将 `pages` 放置在 `ROUNDUP((uintptr_t)end, PGSIZE)`，校验描述符数组物理末端 `descriptors_end <= mem_end`；将 `[0, npage - nbase)` 全部页描述符标记为 `PG_reserved`；将 `[ROUNDUP(descriptors_end, PGSIZE), mem_end)` 区间通过 `init_memmap()` 注册到 `pmm_manager`。
- `best_fit_pmm.c`：
  - 维护按地址升序排列的空闲块链表 `free_list` 与空闲页总数 `nr_free`。
  - `best_fit_alloc_pages(n)`：遍历 `free_list` 选取满足 `p->property >= n` 且 `p->property` 最小的空闲块（相等时取低地址块），若剩余页数 `> 0` 则拆分写回原位置。
  - `best_fit_free_pages(base, n)`：重置释放页状态、按地址升序插入 `free_list`，并与前后物理相邻的空闲块合并。
````

---

<a id="task02"></a>

## 二、功能模块一：Trap 上下文恢复（`RESTORE_ALL`）与 Trap 入口（`__alltraps`）

对应过程记录：[task_02.md](ai_logs/lab3/task_02.md)

````markdown
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

- `__alltraps` 在 `SAVE_ALL` 后将指向当前栈顶 `trapframe` 的指针放入 `a0`（`move a0, sp`），并调用 `jal trap`；`trap()` 返回时 `sp` 仍指向该 `trapframe` 起始地址，随后顺序落入 `__trapret`。
- `RESTORE_ALL` 正确恢复 `sstatus` 和 `sepc` CSR（包括 `trap()` 中对 `tf->epc` 的修改），正确恢复除 `x0` 外的全部通用寄存器 `x1..x31`，且最后一步恢复 `x2(sp)`，使 `sp` 精确回到 Trap 发生前的值。
- 通过 `make grade` 中的 Test 1（`[Test 1] Trap entry and breakpoint return: PASS (35/35)`）。

## SPECIFICATION

- **`__alltraps` 调用约定规格**：
  - Preconditions：`SAVE_ALL` 已执行完毕，`sp` 指向栈上刚构建好的 `struct trapframe` 首地址。
  - Postconditions：第一参数寄存器 `a0 == sp`，调用 `jal trap` 进入 C 函数 `trap(struct trapframe *tf)`；由于 `sp` 是被调用者保存（Callee-saved）寄存器，`trap` 返回后 `sp` 不变，紧接着进入 `__trapret`。
- **`RESTORE_ALL` 恢复顺序与寄存器约束规格**：
  - Preconditions：`sp` 指向当前 `struct trapframe` 首地址；C 处理函数可能已经更新了 `33*REGBYTES(sp)` 处的 `tf->epc` 或 `32*REGBYTES(sp)` 处的 `tf->status`。
  - Postconditions：
    1. **先恢复 CSR，再恢复借用的通用寄存器**：由于 RISC-V 不能直接从内存写 CSR，需先用通用寄存器（`s1`、`s2`）从 `32*REGBYTES(sp)` 和 `33*REGBYTES(sp)` 加载 `status` 和 `epc`，执行 `csrw sstatus, s1` 和 `csrw sepc, s2`（`sbadaddr` 与 `scause` 是硬件进入 Trap 时写入的只读现场报告，`sret` 返回时无需写回）。
    2. **恢复通用寄存器 `x1`、`x3..x31`**：依次使用 `LOAD` 从 `1*REGBYTES(sp)`、`3*REGBYTES(sp)` .. `31*REGBYTES(sp)` 恢复各通用寄存器（这会覆盖第 1 步临时借用的 `s1`、`s2`）。`x0` 恒为 0 无需恢复。
    3. **最后恢复 `x2(sp)`**：必须把 `LOAD x2, 2*REGBYTES(sp)` 放在整个宏的最后一条指令执行，因为一旦修改 `x2(sp)`，后续基于 `(sp)` 的偏移寻址基准就会改变；而 `2*REGBYTES(sp)` 中保存的正是 `SAVE_ALL` 分配 36 个寄存器空间之前的原始 `sp`，直接加载即可一步完成栈指针恢复与 TrapFrame 栈空间释放。
````

---

<a id="task03"></a>

## 三、功能模块二：中断与异常处理功能（`interrupt_handler` 与 `exception_handler`）

对应过程记录：[task_03.md](ai_logs/lab3/task_03.md)

````markdown
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

- `interrupt_handler` 和 `exception_handler` 为非 `static` 函数（因为 `test_lab3.c` 通过 `extern` 直接调用它们）。
- 编译无任何警告（`-Werror`）。
- `make grade` 三个测试全部 PASS，总分 `100/100`。
- `make qemu` 正常启动，每秒输出一行 `100 ticks`，累计输出达到 `SHUTDOWN_AFTER`（默认定义为 `10`）次后调用 `sbi_shutdown()` 正常关机退出。

## SPECIFICATION

- **指令长度计算辅助逻辑**：
  - 在 RISC-V 架构（含 C 压缩指令扩展）中，指令长度由指令机器码最低 2 位决定：读取 `*(const uint16_t *)(tf->epc)`，若 `(inst & 0x3) != 0x3` 则为 2 字节压缩指令（如 `c.ebreak`），步进长度为 `2`；若 `(inst & 0x3) == 0x3` 则为 4 字节标准 32 位指令（如 32 位 `ebreak` 或 `.word 0x0000007b`），步进长度为 `4`。
- **`interrupt_handler(struct trapframe *tf)`**：
  - Preconditions：`tf` 非空；可能由硬件中断经 `trap_dispatch` 调用（此时 `tf->cause` 最高位为 1），也可能由单元测试直接构造 `tf.cause` 调用（最高位可能为 1 或 0）。
  - Postconditions：
    - 提取中断原因码 `intptr_t cause = (tf->cause << 1) >> 1;` 清除最高位符号位。
    - `IRQ_S_TIMER`：
      1. 调用 `clock_set_next_event()` 设置下一次时钟中断；
      2. `ticks++`；当 `ticks % TICK_NUM == 0` 时调用 `print_ticks()`，并递增打印轮数计数 `num`；
      3. 当累计打印次数达到 `SHUTDOWN_AFTER`（`#ifndef SHUTDOWN_AFTER` 定义为 `10`）时，调用 `sbi_shutdown()` 关机。
    - 其他已知中断类型分别输出对应提示（如 `IRQ_S_SOFT` -> `"Supervisor software interrupt\n"` 等）。
    - `default`（无法识别的中断，如 `cause == 15`）：先输出 `cprintf("Unknown interrupt: cause %d\n", (int)cause);`，再调用 `print_trapframe(tf);`。
- **`exception_handler(struct trapframe *tf)`**：
  - Preconditions：`tf` 非空，`tf->cause` 为异常原因码。
  - Postconditions：
    - `CAUSE_ILLEGAL_INSTRUCTION`：输出 `"Exception type: Illegal instruction\n"` 及 `"Illegal instruction at 0x%08x\n", tf->epc`，然后根据 `tf->epc` 处指令编码长度将 `tf->epc` 增加 2 或 4。
    - `CAUSE_BREAKPOINT`：输出 `"Exception type: breakpoint\n"` 及 `"Breakpoint at 0x%08x\n", tf->epc`，然后根据 `tf->epc` 处指令编码长度将 `tf->epc` 增加 2 或 4。
    - 其他未要求具体处理的异常（包括 `default`）：调用 `print_trapframe(tf);` 进行兜底输出。
````

---

<a id="task04"></a>

## 四、代码审查与综合评测验证

对应过程记录：[task_04.md](ai_logs/lab3/task_04.md)

````markdown
## PROMPT

你是 OS Lab 3 的 Review & Verification Agent。请对照 `task_01.md`、`task_02.md`、`task_03.md` 中的 `GUARANTEE` 与 `SPECIFICATION`，以及实验指导书 `lab3_2_1_exercise.md`、`lab3_4_lab_requirement.md` 的约束，对 `code/` 下的全部改动执行静态代码审查与运行时评测核验。

## RELY

- `evidence/t1_mm.diff`、`evidence/t2_trapentry.diff`、`evidence/t3_trap.diff`
- `evidence/build_output.txt`、`evidence/final_qemu.txt`、`evidence/final_grade.txt`

## GUARANTEE

- 确认仅修改规定的 4 个源文件、保留 `lab2`/`lab3` 注释并替换学号 `2412172`；
- 确认 `make grade` 得分 `100/100`，`make qemu` 正常触发 10 轮时钟中断输出并自动关机。

## SPECIFICATION

- 逐项核对 `git status`、`git diff` 与容器内实测输出日志，形成审查报告。
````
