# Task 01：迁移 Lab 2 物理内存管理实现（`page_init` 与 `best_fit_pmm_manager`）

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
- `code/kern/mm/memlayout.h`：`KERNBASE`、`PHYSICAL_MEMORY_END`、`PHYSICAL_MEMORY_OFFSET`、`struct Page`、`SetPageReserved`、`ClearPageReserved`、`PageReserved`、`SetPageProperty`、`ClearPageProperty`、`PageProperty`、`free_area_t`、`le2page`。
- `code/kern/mm/pmm.h`：`PADDR(kva)`、`pa2page(pa)`、`page2pa(page)`、`set_page_ref`、`struct pmm_manager`。
- `code/kern/mm/mmu.h`：`PGSIZE`、`PPN`；`code/libs/defs.h`：`ROUNDUP`、`ROUNDDOWN`；`code/libs/list.h`：双向循环链表操作 `list_init`、`list_add_before`、`list_add_after`、`list_del`、`list_next`、`list_prev`、`list_empty`。

## GUARANTEE

- `code/kern/mm/pmm.c` 与 `code/kern/mm/best_fit_pmm.c` 编译零警告（`-Werror`）。
- 内核启动时 `pmm_init()` 正常输出 `memory management: best_fit_pmm_manager`、`physical memory map:` 并通过 `check_alloc_page() succeeded!` 断言自检。
- 保留 `pmm.c` 中 Lab 3 新增的 `<../sync/sync.h>` 头文件及 `local_intr_save` / `local_intr_restore` 临界区保护不变。

## SPECIFICATION

- `page_init()`：
  - Preconditions：`dtb_init()` 与 `init_pmm_manager()` 已完成。
  - Postconditions：校验 `mem_base == DRAM_BASE` 且 `mem_size > 0`；打印物理内存映射范围；设置 `va_pa_offset = PHYSICAL_MEMORY_OFFSET`；将 `mem_end` 按 `PHYSICAL_MEMORY_END` 截断并向下页对齐，设置 `npage = PPN(mem_end)`；将 `pages` 放置在 `ROUNDUP((uintptr_t)end, PGSIZE)`，校验描述符数组物理末端 `descriptors_end <= mem_end`；将 `[0, npage - nbase)` 全部页描述符标记为 `PG_reserved`；将 `[ROUNDUP(descriptors_end, PGSIZE), mem_end)` 区间通过 `init_memmap()` 注册到 `pmm_manager`。
- `best_fit_pmm.c`：
  - 维护按地址升序排列的空闲块链表 `free_list` 与空闲页总数 `nr_free`。
  - `best_fit_alloc_pages(n)`：遍历 `free_list` 选取满足 `p->property >= n` 且 `p->property` 最小的空闲块（相等时取低地址块），若剩余页数 `> 0` 则拆分写回原位置。
  - `best_fit_free_pages(base, n)`：重置释放页状态、按地址升序插入 `free_list`，并与前后物理相邻的空闲块合并。

## Agent Result

1. 在 `code/kern/mm/pmm.c` 中：
   - 将 `init_pmm_manager()` 的分配器指针切换为 `&best_fit_pmm_manager`。
   - 在 `//lab2 function1 2412172` 处补充 `#ifndef PHYSICAL_MEMORY_END` 兜底宏定义（`0x88000000`）并实现 `static void page_init(void)`，完成 DTB 物理内存探测、页描述符数组 `pages` 布局、已用内存 `PG_reserved` 标记以及空闲物理页 `init_memmap()` 注册。
   - 完整保留了 Lab 3 `pmm.c` 中原有的 `<../sync/sync.h>` 与 `local_intr_save` / `local_intr_restore` 中断保护逻辑。
2. 在 `code/kern/mm/best_fit_pmm.c` 中：
   - 在 `//lab2 function2-2 2412172` 处实现了 `best_fit_init`、`best_fit_insert`、`best_fit_init_memmap`、`best_fit_alloc_pages`、`best_fit_free_pages`、`best_fit_nr_free_pages` 及 `best_fit_pmm_manager` 结构体。
   - 未修改 `basic_check()` 与 `best_fit_check()`。

## Verification

- 编译验证：`os2026.sh build` 零警告、零错误通过编译（见 `evidence/build_output.txt`）。
- 运行时自检：`os2026.sh qemu` 启动日志输出 `memory management: best_fit_pmm_manager`、`physical memory map: memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff].` 以及 `check_alloc_page() succeeded!`（见 `evidence/final_qemu.txt`）。
- 差异补丁：已保存至 `evidence/t1_mm.diff`。

### 给初学者的大白话原理解释

可以把物理内存想象成一整排带编号的储物柜（每个柜子 4KB）：开机时，操作系统先向硬件打听清楚一共有多少个柜子（DTB 探测），再把内核自己已经占用的柜子贴上“已征用”封条（`PG_reserved`），把剩下的空柜子登记到空闲账本上。Best-Fit（最佳适应）管理员每次收到借柜子申请时，都会把账本从头到尾看一遍，挑出“刚好够用、多余空位最少”的那一排连号空柜子借出去，剩下的零头继续留在账本里。归还柜子时，管理员还会检查左右相邻的柜子是不是也空着，如果空着就把它们拼成一个大号连排空位，防止柜子越用越零碎。而在 Lab 3 开启中断后，借还柜子前都要先“挂上免打扰牌”（关中断 `local_intr_save`），改完账本再恢复中断，防止查账一半被定时器中断打断导致账本错乱。
