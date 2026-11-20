# Lab 4 AI 生成与调试过程中的问题记录（Bugs & Fixes）

本文件记录 Lab 4 在迁移 Lab 2/3 代码、生成本实验代码、编译及运行验证过程中发现的真实问题与解决过程。

---

## Bug 1：Lab 3 的 `exception_handler` 使用的异常号宏在 Lab 4 中改名

- **阶段**：`task_01`（迁移 Lab 3 的 `trap.c`）
- **现象**：把 Lab 3 的 `interrupt_handler` / `exception_handler` 原样填入后编译报错：
  ```text
  kern/trap/trap.c:161:10: error: 'CAUSE_FAULT_FETCH' undeclared (first use in this function)
  kern/trap/trap.c:179:10: error: 'CAUSE_FAULT_LOAD' undeclared (first use in this function)
  kern/trap/trap.c:187:10: error: 'CAUSE_FAULT_STORE' undeclared (first use in this function)
  ```
- **原因**：Lab 4 骨架的 `libs/riscv.h` 改用了 RISC-V 特权规范的命名：`CAUSE_FETCH_ACCESS`（0x1）、`CAUSE_LOAD_ACCESS`（0x5）、`CAUSE_STORE_ACCESS`（0x7），并新增了 `CAUSE_*_PAGE_FAULT`。异常号数值没有变。
- **修复**：在 `trap.c` 中把三个宏替换为新名字，分支逻辑不变。
- **验证**：重新编译通过，`make qemu` 时钟中断初始化后正常运行。

## Bug 2：`page_init()` 依赖的 `PHYSICAL_MEMORY_END` 在 Lab 4 的 `memlayout.h` 中不存在

- **阶段**：`task_01`（迁移 Lab 2 的 `page_init`）
- **现象**：
  ```text
  kern/mm/pmm.c:110:19: error: 'PHYSICAL_MEMORY_END' undeclared (first use in this function); did you mean 'PHYSICAL_MEMORY_OFFSET'?
  ```
- **原因**：与 Lab 3 遇到的情况相同，Lab 4 的 `memlayout.h` 只保留 `KERNBASE`、`KMEMSIZE`、`KERNTOP`、`PHYSICAL_MEMORY_OFFSET`。
- **修复**：不修改 `memlayout.h`，在 `page_init()` 内用已有宏换算同一个物理上界：
  ```c
  uint64_t phys_mem_end = KERNTOP - PHYSICAL_MEMORY_OFFSET;   // = 0x88000000
  ```
  `KERNTOP = 0xFFFFFFFFC0200000 + 0x7E00000 = 0xFFFFFFFFC8000000`，减去偏移 `0xFFFFFFFF40000000` 正好是 `0x88000000`，与 Lab 2 的定义一致。
- **验证**：启动输出 `memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff]`，`check_alloc_page()`、`check_pgdir()`、`check_boot_pgdir()` 全部通过。

## Bug 3：骨架 `default_pmm.c` 引用了不存在的 `best_fit_pmm.h`，且 `pmm.c` 固定绑定 `default_pmm_manager`

- **阶段**：`task_01`（迁移 Lab 2 的 Best-Fit 分配器），阅读骨架时发现，未等编译报错。
- **现象**：`kern/mm/default_pmm.c` 开头 `#include <best_fit_pmm.h>`，文件后半部分是 `basic_check` / `best_fit_check`（使用 `free_list`、`nr_free`），但 `kern/mm/` 下没有 `best_fit_pmm.h`；而 `pmm.c` 的 `init_pmm_manager()`（不在 `//lab2` 填写区内）写死 `pmm_manager = &default_pmm_manager`。
- **处理**：
  1. 从 Lab 2 分支补回 `kern/mm/best_fit_pmm.h`（原样复制）。
  2. 在 `default_pmm.c` 的 `//lab2` 处填入 Lab 2 的 Best-Fit 实现（`free_area`、`free_list`/`nr_free` 宏、`best_fit_check` 前向声明及 5 个函数），管理器结构体以 `default_pmm_manager` 为名导出，`.name` 保持 `"best_fit_pmm_manager"`。
  3. 不修改 `pmm.c` 中填写区以外的代码，也不修改 `basic_check` / `best_fit_check`。
- **验证**：启动输出 `memory management: best_fit_pmm_manager`，`check_alloc_page() succeeded!`。

---

## Spec 中预先规避的设计陷阱

以下问题没有在运行中出现，因为在写 Spec 时已从测试和指导书中识别出来并写成约束：

1. **`page_insert` 对同一地址重复插入同一页**：若先撤销旧映射再加引用，引用计数会短暂归零并释放该页。Spec 要求“先 `page_ref_inc` 再处理旧映射，同页时引用减回”。对应 `test_lab4_pmm_basic` 中“重复插入后 `nr_free_pages()` 不变”的断言。
2. **`proc_run` 不能使用 `lsatp()`**：`libs/riscv.h` 的 `lsatp()` 是 Sv32 遗留，Spec 要求按 `enable_paging()` 的写法拼 `SATP_MODE_SV39`。
3. **idleproc 不在 `proc_list` 中**：`schedule` 从 idleproc 出发时必须从链表头开始找，否则 `current->list_link` 是一个自环节点，循环会立刻结束、永远选不到 initproc。
4. **idleproc 字段必须保持空白**：测试断言 `idleproc->context.ra == 0`、`tf == NULL`、`kstack == bootstack`，所以 `proc_init` 只设置 pid、state、kstack、need_resched 和名字。
