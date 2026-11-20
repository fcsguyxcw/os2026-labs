# Lab 4 提示词汇总（PROMPT / RELY / GUARANTEE / SPECIFICATION）

本文件汇总 Lab 4 在依赖迁移和三个功能模块中使用的四段式提示词。生成过程中遇到的问题见 [ai_logs/lab4/bugs.md](ai_logs/lab4/bugs.md)。

---

<a id="task01"></a>

## 一、前置依赖任务：迁移 Lab 2 / Lab 3 代码

````markdown
## PROMPT

你是 OS Lab 4 的 Implementation Agent。工作目录为 `os2026-labs`（分支 `lab4`），源码位于 `code/`。
指导书要求：“本实验依赖实验2/3。请使用你此前设计的提示词生成相应实验代码，并将生成结果填写到本实验代码中标有 lab2/lab3 注释的对应位置，确保代码能够正确编译。”
请把 `lab2`、`lab3` 分支中已经通过检查的实现填入以下位置，保留原注释，并在其后加 `lab2 functionN 2412172` / `lab3 functionN 2412172` 标记：
1. `code/kern/mm/pmm.c` 的 `//lab2 请补充你在lab2中的代码`：`page_init()`。
2. `code/kern/mm/default_pmm.c` 的 `//lab2 请补充你在lab2中的代码`：Best-Fit 分配器。
3. `code/kern/trap/trap.c` 的 `//lab3 请补充你在lab3中的代码`：`interrupt_handler()`、`exception_handler()`。
4. `code/kern/trap/trapentry.S` 两处 `#lab3`：`RESTORE_ALL` 宏体，以及 `__alltraps` 中 `move a0, sp; jal trap`。
迁移前先对比 Lab 4 与旧实验的 `memlayout.h`、`riscv.h`、`pmm.c` 中的宏名和管理器名，列出所有不再存在的符号再动手。

## RELY

- `pmm.c` 的 `init_pmm_manager()` 不在填写区内，固定使用 `&default_pmm_manager`；`default_pmm.c` 已 `#include <best_fit_pmm.h>` 并包含不可修改的 `basic_check()` / `best_fit_check()`，它们使用 `free_list`、`nr_free`。
- Lab 4 的 `pmm.c` 中 `alloc_pages` / `free_pages` / `nr_free_pages` 已带 `local_intr_save` 保护，不得修改。
- Lab 4 的 `memlayout.h` 提供 `KERNBASE`、`KMEMSIZE`、`KERNTOP`、`PHYSICAL_MEMORY_OFFSET`。
- `trapentry.S` 的 `SAVE_ALL` 在 `sp` 上按 `struct trapframe` 布局保存 `x1..x31` 与 `sstatus/sepc/badvaddr/cause`（下标 32~35）。

## GUARANTEE

- 不修改 `memlayout.h`、`riscv.h`、`basic_check`、`best_fit_check` 以及 `pmm.c` 填写区之外的代码。
- 编译通过；启动输出 `memory management: best_fit_pmm_manager`，`check_alloc_page()` 通过；时钟中断正常。

## SPECIFICATION

- `page_init()`：与 Lab 2 相同，但可管理物理内存上界改用已有宏换算：`KERNTOP - PHYSICAL_MEMORY_OFFSET`（= `0x88000000`）。
- Best-Fit：与 Lab 2 相同（按地址升序空闲链表，选满足大小的最小块，释放时与前后相邻块合并），以 `default_pmm_manager` 为名导出，`.name = "best_fit_pmm_manager"`；若 `best_fit_pmm.h` 不存在则从 Lab 2 原样补回。
- `trap.c`：与 Lab 3 相同；异常号宏使用 Lab 4 `riscv.h` 中的名字。
- `RESTORE_ALL`：先从 32/33 槽位取出 `sstatus`、`sepc` 并写回 CSR，再恢复 `x1, x3..x31`，最后恢复 `x2(sp)`。
````

---

<a id="task02"></a>

## 二、功能模块一：虚拟内存页操作（`page_insert` / `page_remove`）

````markdown
## PROMPT

在 `code/kern/mm/pmm.c` 的 `//LAB4 function1 YOUR CODE` 处（把 `YOUR CODE` 换成 `2412172`）实现：
- `int page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm);`
- `void page_remove(pde_t *pgdir, uintptr_t la);`
可以增加一个 `static inline void page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep)` 辅助函数。
实现时保持“页表项内容、物理页引用计数、TLB 状态”三者同步。

## RELY

- `pte_t *get_pte(pde_t *pgdir, uintptr_t la, bool create)`：返回 `la` 对应的末级页表项地址；`create = 1` 时按需分配中间级页表页，分配失败返回 `NULL`。
- `pte2page(pte)`、`page2ppn(page)`、`pte_create(ppn, perm)`、`page_ref_inc/dec(page)`（返回新值）、`free_page(page)`、`tlb_invalidate(pgdir, la)`（`sfence.vma la`），`PTE_V/R/W/U`。
- 测试 `check_pgdir`、`check_boot_pgdir`、`test_lab4_pmm_basic`：同一地址先后插入不同页、同一地址重复插入同一页、对未映射地址重复 `page_remove`。

## GUARANTEE

- `page_insert` 成功返回 0；`get_pte` 失败返回 `-E_NO_MEM`，且此时不修改任何引用计数或页表项。
- `page_remove` 幂等：未映射时安静返回，不分配中间级页表。
- 所有页表项修改后都对该地址执行 `tlb_invalidate`。

## SPECIFICATION

- `page_remove_pte`：Pre：`ptep` 非空。若 `*ptep & PTE_V`：取 `page = pte2page(*ptep)`，`page_ref_dec(page) == 0` 时 `free_page(page)`；`*ptep = 0`；`tlb_invalidate`。否则什么也不做。
- `page_remove`：`ptep = get_pte(pgdir, la, 0)`，非空时调用 `page_remove_pte`。
- `page_insert`：
  1. `ptep = get_pte(pgdir, la, 1)`，为空返回 `-E_NO_MEM`。
  2. 先 `page_ref_inc(page)`。
  3. 若 `*ptep & PTE_V`：旧页等于 `page` 则 `page_ref_dec(page)`（只更新权限，不得释放）；否则 `page_remove_pte` 撤销旧映射。
  4. `*ptep = pte_create(page2ppn(page), PTE_V | perm)`；`tlb_invalidate`；返回 0。
- Postcondition：`get_pte(pgdir, la, 0)` 指向 `page`，权限位恰为 `PTE_V | perm`；被替换的旧页引用计数减一，减到 0 时已归还分配器。
````

---

<a id="task03"></a>

## 三、功能模块二：进程创建及初始化

````markdown
## PROMPT

在 `code/kern/process/proc.c` 的 `//LAB4 function2 YOUR CODE` 处（换成 `2412172`）实现：
`alloc_proc`、`copy_mm`、`copy_thread`、`do_fork`、`kernel_thread`、`proc_init`。
目标：`proc_init` 结束后存在 idleproc（pid 0）和 initproc（pid 1），initproc 能在后续调度中第一次运行并执行 `init_main("Hello world!!")`。

## RELY

- `struct proc_struct` 字段：`state, pid, runs, kstack, need_resched, parent, mm, context, tf, pgdir, flags, name, list_link, hash_link`；`struct context` 为 `ra, sp, s0..s11`。
- 已有：`kmalloc/kfree`、`setup_kstack/put_kstack`（`KSTACKPAGE = 2` 页）、`get_pid()`、`hash_proc()`、`find_proc()`、`set_proc_name()`、`wakeup_proc()`（断言原状态非 RUNNABLE/ZOMBIE 并置 RUNNABLE）、`list_init/list_add`、全局 `proc_list`、`hash_list[HASH_LIST_SIZE]`、`nr_process`、`MAX_PROCESS`、`boot_pgdir_pa`、`bootstack`、`CLONE_VM`、`SSTATUS_SPP/SPIE/SIE`、`read_csr`。
- 首次运行链路已存在：`forkret()` 调用 `forkrets(current->tf)`；`forkrets` 执行 `move sp, a0; j __trapret`；`kernel_thread_entry` 执行 `move a0, s1; jalr s0; jal do_exit`。
- 测试 `test_lab4_process_create` 在 `proc_init` 末尾断言：`current == idleproc`；idleproc 的 `pid == 0`、`state == RUNNABLE`、`kstack == bootstack`、`parent == mm == tf == NULL`、`runs == 0`、`flags == 0`、`context.ra == 0`；initproc 的 `pid == 1`、`state == RUNNABLE`、`parent == idleproc`、`mm == NULL`、`context.ra != 0`、`context.sp != 0`。

## GUARANTEE

- `do_fork` 失败时释放已分配资源（内核栈、PCB），返回负错误码：进程数满 `-E_NO_FREE_PROC`，内存不足 `-E_NO_MEM`。
- pid 分配与挂链表在关中断区内完成；`wakeup_proc` 在所有资源就位后最后调用。

## SPECIFICATION

- `alloc_proc`：`kmalloc` 成功后：`state = PROC_UNINIT`、`pid = -1`、`runs = 0`、`kstack = 0`、`need_resched = 0`、`parent = mm = tf = NULL`、`context` 清零、`pgdir = boot_pgdir_pa`、`flags = 0`、`name` 清零、两个链表节点 `list_init`。
- `copy_mm`：`assert(current->mm == NULL)`，返回 0。
- `copy_thread(proc, esp, tf)`：`proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1`；`*(proc->tf) = *tf`；`proc->tf->gpr.a0 = 0`；`proc->tf->gpr.sp = esp ? esp : (uintptr_t)proc->tf`；`context.ra = forkret`；`context.sp = proc->tf`。
- `do_fork`：`nr_process >= MAX_PROCESS` → `-E_NO_FREE_PROC`；`alloc_proc` → `parent = current` → `setup_kstack` → `copy_mm` → `copy_thread`；关中断：`pid = get_pid()`、`hash_proc`、`list_add(&proc_list, &list_link)`、`nr_process++`；开中断；`wakeup_proc`；返回 pid。失败标签 `bad_fork_cleanup_kstack: put_kstack` → `bad_fork_cleanup_proc: kfree`。
- `kernel_thread(fn, arg, flags)`：栈上 `struct trapframe tf` 清零；`gpr.s0 = fn`、`gpr.s1 = arg`；`status = (read_csr(sstatus) | SSTATUS_SPP | SSTATUS_SPIE) & ~SSTATUS_SIE`；`epc = kernel_thread_entry`；返回 `do_fork(flags | CLONE_VM, 0, &tf)`。
- `proc_init`：初始化 `proc_list` 与全部 `hash_list`；`idleproc = alloc_proc()`（失败 panic），只设 `pid = 0`、`state = RUNNABLE`、`kstack = bootstack`、`need_resched = 1`、名字 `idle`，`nr_process++`；`current = idleproc`；`pid = kernel_thread(init_main, "Hello world!!", 0)`（`<= 0` panic）；`initproc = find_proc(pid)`，命名 `init`；断言两者 pid 为 0 和 1。
````

---

<a id="task04"></a>

## 四、功能模块三：进程调度

````markdown
## PROMPT

实现：`code/kern/process/proc.c` 的 `//LAB4 function3 YOUR CODE` 处 `proc_run`、`cpu_idle`；`code/kern/schedule/sched.c` 的 `schedule`；`code/kern/process/switch.S` 的 `switch_to`。注释中的 `YOUR CODE` 换成 `2412172`。

## RELY

- `struct context` 字段顺序：`ra, sp, s0, s1, ..., s11`（14 个 `uintptr_t`）；`riscv.h` 提供汇编宏 `STORE`、`LOAD`、`REGBYTES`。
- idleproc 不在 `proc_list` 中；`list_next`、`le2proc(le, list_link)`、`local_intr_save/restore`、`write_csr`、`flush_tlb()`、`SATP_MODE_SV39`（= 8）、`RISCV_PGSHIFT`。
- 注意：`libs/riscv.h` 的 `lsatp()` 是 Sv32 遗留，无法表达 Sv39 的 `satp`，不可用；`pmm.c` 的 `enable_paging()` 是正确写法参照。
- 测试 `test_lab4_schedule` 在 initproc 中再创建两个线程，各自循环 `need_resched = 1; schedule();`，initproc 调度 8 次后要求两个线程都运行过。

## GUARANTEE

- `schedule` 与 `proc_run` 的关键区域关中断。
- 找不到可运行线程时退回 idleproc。
- `switch_to` 只做寄存器保存与恢复，不包含调度策略。

## SPECIFICATION

- `cpu_idle`：`while (1) if (current->need_resched) schedule();`
- `schedule`：关中断；`current->need_resched = 0`；`last = (current == idleproc) ? &proc_list : &current->list_link`；从 `last` 开始沿 `list_next` 绕一圈，跳过表头，找到第一个 `state == PROC_RUNNABLE` 即停止；没找到则 `next = idleproc`；`next->runs++`；`next != current` 时 `proc_run(next)`；开中断。
- `proc_run(proc)`：`proc == current` 时什么也不做；否则关中断：`current = proc`；`write_csr(satp, ((uintptr_t)SATP_MODE_SV39 << 60) | (proc->pgdir >> RISCV_PGSHIFT))`；`flush_tlb()`；`switch_to(&prev->context, &proc->context)`；开中断。
- `switch_to(a0 = from, a1 = to)`：按字段顺序把 `ra, sp, s0..s11` 存到 `0..13 * REGBYTES(a0)`，再从 `a1` 的同样偏移加载，`ret`。只保存被调用者保存寄存器，因为它是普通函数调用。
````
