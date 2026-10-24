# Lab 2 提示词汇总（PROMPT / RELY / GUARANTEE / SPECIFICATION）

本文件汇总 Lab 2 各功能模块实现、测试生成、代码审查与 QEMU 源码调试分析所使用的四段式提示词。

---

<a id="task01"></a>

## 一、功能模块一·任务一：`page_init()` 物理内存探测与初始化（初版）

对应过程记录：[task_01.md](ai_logs/lab2/task_01.md)

````markdown
## PROMPT

你是 OS Lab2 的 Implementation/Test Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，当前分支 `lab2`，实验代码在 `code/`。运行环境是 macOS + Docker（OrbStack），在镜像 `os2026-dbg`（RISC-V GCC 13.2.0、QEMU 4.1.1）里编译和运行。

任务：在 `code/kern/mm/pmm.c` 第 64 行注释 `//lab2 function1 YOUR CODE` 的位置实现 `static void page_init(void);`，它被同文件后面的 `pmm_init()` 调用。功能：根据设备树提供的物理内存信息建立 `Page` 描述符数组，把排除了 OpenSBI、内核镜像和描述符数组本身之后剩余的完整空闲页面交给当前 `pmm_manager`（通过同文件里的 `init_memmap()`）。

操作要求：
1. 必须直接修改仓库里的实际文件，不要只展示代码。
2. 只允许修改 `code/kern/mm/pmm.c`，并且只在第 64 行注释位置添加 `page_init` 及其需要的静态辅助代码；把这行注释改为 `//lab2 function1 2412172`（保留 `lab2 function1`，把 `YOUR CODE` 换成学号）。
3. 禁止修改：`pmm.c` 里其它已有函数、任何头文件、`kern/init/entry.S`、`Makefile`、`tools/*`、`kern/driver/*`、`default_pmm.c`、`best_fit_pmm.c`。

输出要求：
- QEMU 输出中出现 `physical memory map:` 和形如 `  memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff].` 的一行（大小、起止地址以实际为准，末地址是闭区间），之后自检通过并打印 `check_alloc_page() succeeded!` 和 `satp ...` 两行，无 panic。
- `make grade` 里 Test 1（page init）得满分。

## RELY

- 全局变量（`pmm.c:15-22`）：`struct Page *pages;`（描述符数组的**虚拟地址**）、`size_t npage`（可管理的物理页号上界，注意是 PPN 上界，不是数组长度）、`uint64_t va_pa_offset;`、`const size_t nbase = DRAM_BASE / PGSIZE;`（= 0x80000）。
- `pmm.c`：`static void init_memmap(struct Page *base, size_t n)` 调用 `pmm_manager->init_memmap(base, n)`；`pmm_init()` 依次调用 `init_pmm_manager()`、`page_init()`、`check_alloc_page()`。
- `kern/driver/dtb.h`：`uint64_t get_memory_base(void); uint64_t get_memory_size(void);`，由 `dtb_init()`（在 `pmm_init` 之前运行）从设备树解析；设备树无效或缺失时两者都返回 0。
- `kern/mm/memlayout.h`：`KERNBASE 0xFFFFFFFFC0200000`、`PHYSICAL_MEMORY_END 0x88000000`（内核能管理的物理地址上界）、`PHYSICAL_MEMORY_OFFSET 0xFFFFFFFF40000000`（内核虚拟地址 − 物理地址）、`KERNEL_BEGIN_PADDR 0x80200000`；`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`；`SetPageReserved/ClearPageReserved/PageReserved`（bit `PG_reserved`=0）、`SetPageProperty/ClearPageProperty/PageProperty`（bit `PG_property`=1）。
- `kern/mm/pmm.h`：`PADDR(kva)`（`kva < KERNBASE` 会 panic，结果 `kva - va_pa_offset`）；`page2ppn(page) = page - pages + nbase`；`page2pa(page) = page2ppn(page) << PGSHIFT`；`pa2page(pa)`：`PPN(pa) >= npage` 时 panic，否则返回 `&pages[PPN(pa) - nbase]`。所以 `pages[0]` 对应物理地址 `DRAM_BASE`，数组长度是 `npage - nbase`。
- `kern/mm/mmu.h`：`PGSIZE 4096`、`PGSHIFT 12`、`PPN(la)`；`libs/riscv.h`：`DRAM_BASE 0x80000000`；`libs/defs.h`：`ROUNDUP(a, n)`、`ROUNDDOWN(a, n)`。
- `tools/kernel.ld`：`PROVIDE(end = .);`，`end` 是内核镜像结尾的**虚拟地址**（在 C 里 `extern char end[];`）。
- `kern/debug/assert.h`：`panic(...)`；`cprintf` 来自 `stdio.h`（`pmm.c` 已包含）。
- 内存布局：OpenSBI 占 `[0x80000000, 0x80200000)`；内核占 `[0x80200000, KernelEnd)`；`Page` 描述符数组连续排在内核镜像之后，起点为 `end` 向上按页对齐；数组末尾用 `PADDR` 转成物理地址后再向上按页对齐，才是空闲区起点；设备树报告的内存末端（还要受 `PHYSICAL_MEMORY_END` 限制）向下按页对齐；只有两者之间非空的完整页面区间才能交给 `init_memmap()`；在空闲区建立之前，所有描述符都应先处于保留状态。`end` 是虚拟地址、`KernelEnd` 是物理地址，不能混用。
- 自动检查 `tools/test_lab2.c` Test 1 检查：`va_pa_offset == PHYSICAL_MEMORY_OFFSET`；`get_memory_base()`、`get_memory_size()` 非 0；`npage == PPN(mem_base + mem_size)` 且 `npage > nbase`；`pages` 非空、按页对齐、`>= ROUNDUP(end, PGSIZE)`；`nr_free_pages()` 非 0 且 `<= npage - nbase`；`pa2page(PADDR(end))` 和 `pages` 数组起始物理页都是 `PageReserved`；`pa2page(page2pa(q)) == q`。

## GUARANTEE

- 实现 `static void page_init(void)`，可以添加只在 `pmm.c` 内部使用的 `static` 辅助函数。
- 成功返回时：`va_pa_offset`、`npage`、`pages` 已正确设置；已打印 `physical memory map:` 及内存范围行；当前 `pmm_manager` 已通过 `init_memmap()` 注册了唯一一段初始空闲区。
- 内核镜像、描述符数组所在的页面，以及 `[DRAM_BASE, 0x80200000)`，都不在空闲区内且保持 `PG_reserved`。
- 不破坏任何现有接口；编译无警告（编译选项含 `-Werror`）。

## SPECIFICATION

### page_init

- Preconditions：`dtb_init()` 已运行；`init_pmm_manager()` 已运行（`pmm_manager` 已设置并 `init()`）；BSS 已清零；启动页表已把 `KERNBASE` 起的高地址映射到 `0x80200000` 起的物理内存，偏移为 `PHYSICAL_MEMORY_OFFSET`。
- Postconditions：
  - `va_pa_offset == PHYSICAL_MEMORY_OFFSET`。
  - `npage` = 可管理物理内存上界（设备树内存末端与 `PHYSICAL_MEMORY_END` 取较小者）的页号；默认 128MB 时为 `0x88000`。
  - `pages = (struct Page *)ROUNDUP((void *)end, PGSIZE)`（虚拟地址），数组覆盖 `npage - nbase` 个描述符，每个描述符 `PG_reserved` 置位。
  - 空闲区 `[freemem, mem_end_aligned)`：`freemem = ROUNDUP(PADDR(&pages[npage - nbase]), PGSIZE)`，`mem_end_aligned = ROUNDDOWN(可管理上界, PGSIZE)`；当 `freemem < mem_end_aligned` 时调用一次 `init_memmap(pa2page(freemem), (mem_end_aligned - freemem) / PGSIZE)`。
  - 打印格式与指导书示例一致：
    ```
    physical memory map:
      memory: 0x%016lx, [0x%016lx, 0x%016lx].
    ```
    分别为内存大小、起始物理地址、结束物理地址（闭区间，即 `base + size - 1`）。
- Branch / Edge cases：
  - Case 1：`get_memory_base() == 0` 或 `get_memory_size() == 0`，或 `base + size` 溢出、`base + size <= base`：`panic`，不修改 `npage`、不调用 `init_memmap()`。
  - Case 2：设备树内存末端超过 `PHYSICAL_MEMORY_END`：以 `PHYSICAL_MEMORY_END` 为上界，超出部分不管理。
  - Case 3：描述符数组之后已经没有完整页面（`freemem >= mem_end_aligned`）：不调用 `init_memmap()`；用 `cprintf` 明确输出原因，不要静默跳过。
  - Case 4：设备树内存起点不是 `DRAM_BASE`：本框架 `pages[0]` 固定对应 `DRAM_BASE`（`nbase` 是常量），此时 `panic` 并打印实际起点，不要擅自改动 `nbase` 或 `pa2page` 的语义。
- Requirements：
  - 计算过程中不要把虚拟地址和物理地址混用：`end`、`pages` 是虚拟地址，转物理地址只能用 `PADDR`；描述符下标 = 物理页号 − `nbase`。
  - 初始空闲区的 `Page` 字段（`property`、`PG_property`、`ref`、链表）由 `pmm_manager->init_memmap` 负责设置，`page_init` 只负责把整段交过去。
  - 不引入与本任务无关的功能。
````

---

<a id="task04"></a>

## 二、功能模块一·任务一：`page_init()` 描述符数组越界检查修复（迭代版）

对应过程记录：[task_04.md](ai_logs/lab2/task_04.md)

````markdown
## PROMPT

问题：`code/kern/mm/pmm.c` 的 `page_init()` 在把 `pages[0 .. npage-nbase)` 全部置 `PG_reserved` 之前，没有确认描述符数组本身落在可管理的物理内存内。若设备树报告的内存很小（例如 base=0x80000000、size=0x208000，内核仍放得下），描述符数组末端会超过内存末端，循环写入越界内存。

任务：只修改 `code/kern/mm/pmm.c` 里的 `page_init()`，在写入任何描述符之前增加容量边界检查。

操作要求：
1. 直接修改实际文件；只改 `page_init()` 函数体，其它代码（包括 `init_pmm_manager()`、学号注释行）一律不动。
2. 不要修改其它文件。

## RELY

- 当前 `page_init()` 实现（初版产物）。
- `PADDR(kva)`：`kva < KERNBASE` 时 panic，否则返回 `kva - va_pa_offset`；`va_pa_offset` 必须先赋值再用 `PADDR`。
- `pages = ROUNDUP(end, PGSIZE)`；描述符数组物理末端 = `PADDR(&pages[npage - nbase])`（只计算地址，不解引用）。
- `mem_end`：已按 `PHYSICAL_MEMORY_END` 截断并向下按页对齐的可管理上界（物理地址）。

## GUARANTEE

- 描述符数组放不下时，在写入任何描述符之前 `panic`，且不调用 `init_memmap()`。
- 原有 Case 1～4 的行为和默认 128MB 路径的输出不变。
- 编译无警告（`-Werror`）。

## SPECIFICATION

### page_init（新增 Case 5，修订 Case 3）

- Case 5（新增）：`PADDR(&pages[npage - nbase]) > mem_end`，即描述符数组超出可管理物理内存：在置位任何 `PG_reserved` 之前 `panic`，信息包含数组物理末端和 `mem_end`。
- Case 3（修订）：数组放得下，但 `ROUNDUP(数组物理末端, PGSIZE) >= mem_end`，即数组之后没有完整空闲页：照旧先把全部描述符置 `PG_reserved`，不调用 `init_memmap()`，用 `cprintf` 输出原因。
- 其余 Preconditions / Postconditions 与初版相同。
````

---

<a id="task02"></a>

## 三、功能模块二·任务二：Best-Fit 连续物理内存分配器

对应过程记录：[task_02.md](ai_logs/lab2/task_02.md)

````markdown
## PROMPT

任务：在 `code/kern/mm/best_fit_pmm.c` 第 6 行注释 `//lab2 function2-2 YOUR CODE` 的位置，实现完整的 Best-Fit 连续物理内存分配模块，并定义 `const struct pmm_manager best_fit_pmm_manager` 完成注册（`best_fit_pmm.h` 已声明 `extern const struct pmm_manager best_fit_pmm_manager;`），使它能替换内核当前使用的 First-Fit 管理器 `default_pmm_manager`。

Best-Fit 的含义：分配 n 页时，在所有长度 ≥ n 的空闲连续块中，选择**长度最小**的那一块；长度相同时选地址最低的那一块。块比 n 大时，从块头切出 n 页分配出去，剩余部分成为新的空闲块。释放时把页面放回空闲链表，并与地址上紧邻的前后空闲块合并。

操作要求：
1. 必须直接修改仓库里的实际文件，不要只展示代码。
2. 只允许修改 `code/kern/mm/best_fit_pmm.c`：在第 6 行注释位置写入实现（内部状态、管理函数、`best_fit_pmm_manager` 定义；`best_fit_pmm_manager` 的 `.check` 指向本文件的 `best_fit_check`，在注释位置先写前向声明 `static void best_fit_check(void);`）。把这行注释改为 `//lab2 function2-2 2412172`（保留 `lab2 function2-2`，把 `YOUR CODE` 换成学号）。
3. 禁止修改：`basic_check()`、`best_fit_check()` 两个测试函数（一个字符都不要动）；任何头文件中已有的结构体、宏和接口；`default_pmm.c`、`kern/init/entry.S`、`Makefile`、`tools/*`、`kern/driver/*`。

输出要求：
- 编译无警告（编译选项含 `-Werror`）。
- 在 `pmm.c` 的 `init_pmm_manager()` 切换为 `pmm_manager = &best_fit_pmm_manager;` 后，内核启动打印 `memory management: best_fit_pmm_manager`，`basic_check()` 和 `best_fit_check()` 全部断言通过并打印 `check_alloc_page() succeeded!`；`make grade` Test 2 得满分。

## RELY

- `kern/mm/pmm.h`：
  ```c
  struct pmm_manager {
      const char *name;
      void (*init)(void);
      void (*init_memmap)(struct Page *base, size_t n);
      struct Page *(*alloc_pages)(size_t n);
      void (*free_pages)(struct Page *base, size_t n);
      size_t (*nr_free_pages)(void);
      void (*check)(void);
  };
  ```
  以及 `page2pa`、`pa2page`、`alloc_page()`/`free_page()`（单页包装）、`npage`。
- `kern/mm/memlayout.h`：`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`；`PG_reserved`、`PG_property`；`SetPageReserved/ClearPageReserved/PageReserved`、`SetPageProperty/ClearPageProperty/PageProperty`；`set_page_ref`/`page_ref`；`le2page(le, member)`；`free_area_t { list_entry_t free_list; unsigned int nr_free; }`。
- `libs/list.h`：双向循环链表 `list_init`、`list_add`、`list_add_before`、`list_add_after`、`list_del`、`list_empty`、`list_next`、`list_prev`。
- `kern/mm/best_fit_pmm.h`：已声明 `extern const struct pmm_manager best_fit_pmm_manager;`。
- `kern/mm/best_fit_pmm.c` 已有的 `basic_check()` 和 `best_fit_check()` 直接读写文件级标识符 `free_list` 和 `nr_free`（例如 `list_empty(&free_list)`、`nr_free = 0`、`le2page(le, page_link)`、检查 `p->property` 和 `PageProperty(p)`），并且 `best_fit_check` 依赖以下约定：
  - 空闲链表按页地址**升序**排列，只把每个空闲连续块的**第一页（块头）**挂进 `free_list`；
  - 空闲块头的 `p->property` = 该连续块的页数，且 `PageProperty(p)` 为真；块内其余页及已分配页的 `PageProperty` 为假；
  - 分配时若块长 `> n`，取块的前 `n` 页返回，后 `p->property - n` 页留在原链表位置作为新块头；
  - 释放时按地址升序插入 `free_list`，若与前一个空闲块紧邻（`prev + prev->property == base`）则合并到前块，若与后一个空闲块紧邻（`base + base->property == next`）则把后块合并进来，被吸收的块头清除 `PG_property` 并从链表删除。
- 参考实现：`kern/mm/default_pmm.c`（完整的 First-Fit 实现，除了 `alloc_pages` 的选块策略是“第一个 `p->property >= n` 就 break”之外，其余数据结构和链表维护方式与上面完全一致）。

## GUARANTEE

- 在 `best_fit_pmm.c` 中定义静态 `free_area` 及 `free_list`、`nr_free` 宏，实现 `best_fit_init`、`best_fit_init_memmap`、`best_fit_alloc_pages`、`best_fit_free_pages`、`best_fit_nr_free_pages`（以及需要的内部 `static` 辅助函数），并定义 `const struct pmm_manager best_fit_pmm_manager`（`name = "best_fit_pmm_manager"`，`.check = best_fit_check`）。
- 保留 `basic_check`、`best_fit_check` 原文不变。
- 编译无警告。

## SPECIFICATION

### best_fit_init
- Preconditions：无。
- Postconditions：`free_list` 为空链表，`nr_free == 0`。

### best_fit_init_memmap(base, n)
- Preconditions：`n > 0`；`[base, base + n)` 内每一页当前都是 `PageReserved(p)`。
- Postconditions：
  - `[base, base + n)` 每页 `flags = 0`、`property = 0`、`ref = 0`；
  - `base->property = n` 且 `SetPageProperty(base)`；
  - `base` 按地址升序插入 `free_list`（链表中所有节点满足地址严格递增）；
  - `nr_free` 增加 `n`。

### best_fit_alloc_pages(n)
- Preconditions：`n > 0`；`free_list` 按地址升序排列，`nr_free` 等于链表各块 `property` 之和。
- Case 1：`n > nr_free`，或链表中没有任何 `p->property >= n` 的块：返回 `NULL`，`free_list` 和 `nr_free` 不变。
- Case 2：存在一个或多个 `p->property >= n` 的块：
  - **完整遍历** `free_list`，选出满足 `p->property >= n` 且 `p->property` **最小**的块 `page`（遇到相同最小长度时保留先遇到的，即地址较低的块）；
  - 从 `free_list` 中移除 `page`；
  - 若 `page->property > n`：令 `rest = page + n`，设置 `rest->property = page->property - n`、`SetPageProperty(rest)`，把 `rest` 插回 `page` 原来的位置（保持链表按地址升序）；
  - `ClearPageProperty(page)`（并把 `page->property` 置 0），`nr_free -= n`，返回 `page`。

### best_fit_free_pages(base, n)
- Preconditions：`n > 0`；`[base, base + n)` 内每一页都不是 `PageReserved` 且都不是 `PageProperty`。
- Postconditions：
  - `[base, base + n)` 每页 `flags = 0`、`property = 0`、`ref = 0`；
  - `base->property = n`，`SetPageProperty(base)`，`nr_free += n`；
  - 把 `base` 按地址升序插入 `free_list`；
  - 若前驱节点 `prev` 满足 `prev + prev->property == base`：`prev->property += base->property`，`ClearPageProperty(base)`，从链表删除 `base`，并令 `base = prev`；
  - 若后继节点 `next` 满足 `base + base->property == next`：`base->property += next->property`，`ClearPageProperty(next)`，从链表删除 `next`。

### best_fit_nr_free_pages
- Postconditions：返回 `nr_free`。
````

---

<a id="task03"></a>

## 四、`page_init` 与 Best-Fit 交叉代码审查提示词

对应过程记录：[task_03.md](ai_logs/lab2/task_03.md)

````markdown
## PROMPT

任务：对照 `page_init` 与 `Best-Fit` 的 GUARANTEE 和 SPECIFICATION，逐条审查 `code/kern/mm/pmm.c` 与 `code/kern/mm/best_fit_pmm.c` 的实现。重点：
- 是否有违反 SPECIFICATION 的分支（含 edge cases）；
- 虚拟地址 / 物理地址 / 页号 / 描述符下标是否混用；
- 链表、`nr_free`、`PG_property`、`PG_reserved` 的不变量是否在每个分支都保持；
- 测试没有覆盖、但 spec 要求的行为。

输出：按 `[MUST_FIX|SHOULD_FIX|NOTE] 文件:行号 — 问题 — 依据（spec 的哪一条）` 列出审查发现，最后给出 `结论：PASS` 或 `结论：NEEDS_FIX`。

## RELY

- 被审规格：`page_init` 与 `Best-Fit` 的四段式规格。
- 运行证据：`make qemu` 与 `make grade`（20/20）输出日志。
- 原始骨架：课程代码包 `lab2.zip`。

## GUARANTEE

- 只读审查，不修改源码。
- 每条发现都给出文件行号和 spec 依据。

## SPECIFICATION

- Preconditions：工作区包含 `page_init`、`Best-Fit` 的实现和 `init_pmm_manager()` 的切换改动。
- Postconditions：输出完整审查报告，最后一行为 PASS 或 NEEDS_FIX。
- Edge cases：发现现有测试未覆盖的边界漏洞时照样报告；没有发现问题时写明“未发现违反 spec 的问题”并 PASS。
````

---

<a id="task09"></a>

## 五、功能模块二·任务三：Buddy System 实现提示词（第一份）

对应过程记录：[task_09.md](ai_logs/lab2/task_09.md)

````markdown
## PROMPT

任务：参考现有 ucore 物理内存管理框架（`struct pmm_manager`，以及 `default_pmm.c` 的 First-Fit、`best_fit_pmm.c` 的 Best-Fit 写法），从零实现一个 **Buddy System（伙伴系统）物理页分配器**，作为新的 `pmm_manager` 接入内核。

Buddy System 的含义：所有空闲块的大小都是 2 的幂（1、2、4、8… 页），一个 order 为 k 的块包含 2^k 页，且其起始页下标（相对于本管理器管理区域的起点）是 2^k 的整数倍。分配 n 页时，把 n 向上取整到 2^k，从 order k 的空闲链表取块；没有就从更大的 order 取一块，逐级对半拆分，拆出的另一半（伙伴）放回对应 order 的链表。释放时，检查同 order 的伙伴块（下标为 `idx ^ (1 << k)`）是否空闲且 order 相同，是就合并成 order k+1 的块并继续向上检查，直到伙伴不空闲或到达最大 order。

操作要求：
1. 必须直接修改实际文件。新建 `code/kern/mm/buddy_pmm.h` 和 `code/kern/mm/buddy_pmm.c`（`kern/mm/` 下的 `.c` 会被 Makefile 自动编译，不需要改 Makefile）。
2. `buddy_pmm.h` 声明 `extern const struct pmm_manager buddy_pmm_manager;` 和 `void buddy_check(void);`。
3. `buddy_pmm.c` 定义 `buddy_pmm_manager`（`name = "buddy_pmm_manager"`），`.check` 指向 `buddy_check`。`buddy_check` 测试函数由第二份提示词单独生成，本阶段先提供最小占位实现。
4. 在 `pmm.c` 中通过编译宏 `LAB2_PMM_BUDDY` 接入：`make DEFS=-DLAB2_PMM_BUDDY` 时 `init_pmm_manager()` 使用 `&buddy_pmm_manager`；默认编译仍用 Best-Fit。

输出要求：
- `make DEFS=-DLAB2_PMM_BUDDY` 编译无警告（`-Werror`），内核打印 `memory management: buddy_pmm_manager`、`physical memory map:`、`check_alloc_page() succeeded!`，无 panic。
- 默认 `make` 和 `make grade`（20/20）不受影响。

## RELY

- `kern/mm/pmm.h`：`struct pmm_manager { const char *name; void (*init)(void); void (*init_memmap)(struct Page *base, size_t n); struct Page *(*alloc_pages)(size_t n); void (*free_pages)(struct Page *base, size_t n); size_t (*nr_free_pages)(void); void (*check)(void); };`；`page2ppn`、`page2pa`、`pa2page`、`alloc_pages`/`free_pages`（经 `pmm_manager` 转发）。
- `kern/mm/memlayout.h`：`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`；`PG_reserved`（bit 0）、`PG_property`（bit 1）及 `Set/Clear/PageReserved`、`Set/Clear/PageProperty`；`le2page`；`free_area_t { list_entry_t free_list; unsigned int nr_free; }`。
- `libs/list.h`：`list_init/list_add/list_add_before/list_add_after/list_del/list_empty/list_next/list_prev`。
- `kern/debug/assert.h`：`assert`、`panic`；`stdio.h`：`cprintf`。
- 调用约定（`pmm.c`）：`pmm_init()` → `init_pmm_manager()`（调用 `init()`）→ `page_init()` 调用**一次** `init_memmap(base, n)`，`base` 起的 `n` 页都是 `PG_reserved` 状态的连续页；默认 128MB 时 `n` 约为 0x7df9 页（不是 2 的幂，起点也不是 2 的大幂次对齐）→ `check_alloc_page()` 调用 `pmm_manager->check()`。
- 参考实现：`default_pmm.c`、`best_fit_pmm.c`。
- 编译选项含 `-Werror -Wall -O2`，`-Wno-unused`。

## GUARANTEE

- 新增 `buddy_pmm.h`、`buddy_pmm.c`；定义 `const struct pmm_manager buddy_pmm_manager`，6 个函数指针全部有实现（`check` 指向 `buddy_check`）。
- 内部函数均为 `static`；只有 `buddy_pmm_manager` 和 `buddy_check` 对外可见。
- 编译无警告。

## SPECIFICATION

### 内部状态
- 每个 order（0..MAX_ORDER，MAX_ORDER 需覆盖 128MB 以内的块，写成宏）一条空闲链表和该 order 的空闲块数；总空闲页数 `nr_free`。
- 空闲块只把块头页挂在链表上：块头 `PG_property` 置位，`property` 记录该块的 order；块内其它页 `PG_property` 清零。
- 记录管理区域的起始 `Page *` 和页数，用于计算“相对下标”和伙伴地址；伙伴不能落到管理区域之外。

### buddy_init
- Postconditions：全部 order 链表为空，`nr_free == 0`，管理区域未设置。

### buddy_init_memmap(base, n)
- Preconditions：`n > 0`；`base[0..n-1]` 都是 `PG_reserved`；本管理器只会被调用一次（如被调用第二次，`panic` 并说明原因）。
- Postconditions：每页 `flags`、`property` 清零，`ref = 0`；把 `[base, base+n)` 按“从低到高，每次取当前位置能放下的、相对下标对齐的最大 2 的幂块”切成若干块挂入对应 order 链表；`nr_free == n`。
- Edge cases：`n` 不是 2 的幂时，尾部小块按同一规则切分，不得丢页也不得越界。

### buddy_alloc_pages(n)
- Preconditions：`n > 0`。
- Case 1：`n > nr_free` 或需要的 order 超过 MAX_ORDER，或所有 ≥ k 的链表都为空：返回 `NULL`，状态不变。
- Case 2：找到 order j ≥ k 的块（取最小的非空 j；同一 order 取链表中地址最低的块）：从链表摘下；当 j > k 时逐级拆分，每次把高地址的一半作为 order j-1 的空闲块挂回；返回块头。返回块头 `ClearPageProperty`，并在块头记录本次分配的 order（释放时需要），`nr_free -= 2^k`。
- Requirements：分配出去的块页数是 2^k ≥ n，多出的 2^k − n 页属于内部碎片，**随整块一起释放**。

### buddy_free_pages(base, n)
- Preconditions：`n > 0`；`base` 是之前 `buddy_alloc_pages(n')` 返回的块头，且 `n` 向上取整到 2 的幂后与分配时的 order 相同（不一致时 `assert` 失败）。
- Postconditions：整块（2^k 页）每页 `flags` 清零、`ref = 0`；然后循环：计算伙伴下标 `idx ^ (1 << k)`，若伙伴在区域内、是空闲块头（`PG_property`）且 order == k，就把伙伴从链表摘下、清除其块头标记，合并为 order k+1（块头取两者中下标较小者），k++；否则停止。最后把块头挂入 order k 链表，`nr_free += 2^(原 k)`。
- Edge cases：k == MAX_ORDER 时不再合并；伙伴超出区域时不合并。

### buddy_nr_free_pages
- 返回 `nr_free`。
````

---

<a id="task10"></a>

## 六、功能模块二·任务三：Buddy System 测试提示词（第二份）

对应过程记录：[task_10.md](ai_logs/lab2/task_10.md)

````markdown
## PROMPT

任务：针对已实现的 Buddy System 分配器（`code/kern/mm/buddy_pmm.c`），设计并实现测试函数 `void buddy_check(void)`。测试要能检验 Buddy 的核心行为和边界情况，失败时用 `assert` 让内核 panic（panic 信息里有文件和行号），全部通过时打印 `buddy_check() succeeded!`。

操作要求：
1. 只修改 `code/kern/mm/buddy_pmm.c` 中 `buddy_check` 的实现，可以在它前面添加只供测试使用的 `static` 辅助函数（例如不变量检查函数）。**不得修改**已有的分配器函数和 `buddy_pmm_manager`。
2. 测试运行方式：`make DEFS=-DLAB2_PMM_BUDDY` 编译后，内核把 Buddy 作为系统物理内存管理器，`page_init` 把初始空闲页一次交给 `buddy_init_memmap`，然后 `check_alloc_page()` 调用 `buddy_check()`。测试就在这个真实初始状态上运行，**结束时必须把分配器恢复到开始时的状态**（每个 order 的空闲块数、`nr_free` 都和开始时相同）。

## RELY

- Buddy 分配器实现（`buddy_pmm.c`）：`MAX_ORDER 15`；`free_areas[order].free_list`（按地址升序）与 `.nr_blocks`；`region_base`、`region_pages`、`nr_free`；空闲块头 `PG_property` 置位、`property` = order；已分配块头 `PG_property` 清零、`property` = 分配 order；块的对齐以 `region_base` 为基准（相对下标是 2^order 的倍数）；`buddy_alloc_pages(n)` 把 n 向上取整到 2^k，取最小的非空 order、同 order 取最低地址，拆分时高地址一半挂回；`buddy_free_pages(base, n)` 要求 n 向上取整后的 order 与分配时一致，按 `idx ^ (1 << k)` 找伙伴并逐级合并。
- 测试可以直接调用 `buddy_pmm_manager.alloc_pages/free_pages/nr_free_pages`，也可以访问本文件内的静态变量做白盒检查。
- `assert`（`kern/debug/assert.h`）、`cprintf`、`page2pa`、`le2page`、`list_next`。

## GUARANTEE

- `buddy_check()` 覆盖下面 SPECIFICATION 列出的全部行为，通过时打印 `buddy_check() succeeded!`。
- 测试结束后分配器状态与开始时完全一致。
- 不修改分配器本身；编译无警告（`-Werror`）。

## SPECIFICATION

测试包含以下 8 组，每组前用 `cprintf` 打印一行组名，便于从输出定位：

1. **不变量检查函数**（每组之后都调用）：遍历每个 order 的链表——每个节点 `PageProperty` 为真、`property == order`、相对下标按 2^order 对齐、块不越出区域、链表按地址严格升序；各 order 链表长度等于 `nr_blocks`；Σ(nr_blocks × 2^order) == `nr_free`；不同空闲块互不重叠。
2. **初始状态**：`nr_free == region_pages`（刚初始化、未分配时），并调用不变量检查。
3. **向上取整与内部碎片**：`alloc(3)` 使 `nr_free` 减少 4；`alloc(1)`、`alloc(5)`、`alloc(8)` 分别减少 1、8、8；返回的块头相对下标按对应 order 对齐；几个块互不重叠；释放后 `nr_free` 恢复。
4. **借块隔离与逐级拆分**：先把较小 order 的空闲块借走（分配出来暂存，之后归还），仅保留最大块作为隔离区；然后分配 1 页小块，检查从大块拆出的每一级都恰好多出一个空闲块、且多出的块正是返回块的伙伴（下标 = 返回块下标 ^ 2^j）。
5. **伙伴合并**：分配两块互为伙伴的 order k 块，先释放一块，检查它以 order k 留在链表上、没有合并；再释放另一块，检查二者合并成更高阶块，链表里不再有这两个 order k 块头。
6. **非伙伴不合并**：两个相邻但不是伙伴的同 order 块（例如相对下标 1 和 2）都空闲时不得合并。
7. **失败路径**：`alloc(nr_free + 1)` 返回 `NULL` 且状态不变；`alloc((1 << MAX_ORDER) + 1)` 返回 `NULL`；借走所有空闲块后 `alloc(1)` 返回 `NULL`。
8. **恢复初态**：归还所有借走的块后，各 order 的 `nr_blocks` 与 `nr_free` 和测试开始时的快照完全一致。
````

---

<a id="task11"></a>

## 七、功能模块二·任务四：SLUB 分配器实现提示词（第一份）

对应过程记录：[task_11.md](ai_logs/lab2/task_11.md)

````markdown
## PROMPT

任务：在没有现成代码框架的情况下，从零实现一个简化的 **SLUB 两层内存分配器**并接入现有内核：
- 第一层：按页分配。直接使用内核已有的 `alloc_pages(n)` / `free_pages(base, n)`（当前系统物理内存管理器是 Best-Fit），不要自己再管理物理页。
- 第二层：在第一层之上按任意字节大小分配。对外提供 `void *kmalloc(size_t size)` 和 `void kfree(void *ptr)`。小对象按大小分级（size class），每一级是一个 cache；每个 cache 由若干 slab 组成，**一个 slab 就是第一层分出来的 1 个物理页**，页内切成等大的对象；空闲对象用“对象内嵌的空闲指针”串成该 slab 的 freelist（SLUB 的做法：空闲时对象自身的前 8 字节存下一个空闲对象地址）。大对象（超过最大 size class）直接按页向第一层申请。

操作要求：
1. 必须直接修改实际文件。新建 `code/kern/mm/slub.h`、`code/kern/mm/slub.c`（`kern/mm/` 下的 `.c` 自动编译，不改 Makefile）。
2. `slub.h` 声明：`void slub_init(void);`、`void *kmalloc(size_t size);`、`void kfree(void *ptr);`、`void slub_check(void);`。
3. `slub_check` 测试函数由第二份提示词单独生成，本阶段先写最小占位实现。
4. 接入方式：在 `pmm.c` 通过编译开关 `LAB2_SLUB_TEST` 接入——`make DEFS=-DLAB2_SLUB_TEST` 时，`pmm_init()` 在 `check_alloc_page()` 之后调用 `slub_init(); slub_check();`。默认编译不调用 SLUB。

输出要求：`make DEFS=-DLAB2_SLUB_TEST` 编译无警告，运行无 panic；默认 `make grade` 仍 20/20。

## RELY

- `kern/mm/pmm.h`：`struct Page *alloc_pages(size_t n); void free_pages(struct Page *base, size_t n); size_t nr_free_pages(void);`；`page2pa(page)`；`pa2page(pa)`（越界 panic）；`PADDR(kva)`（`kva < KERNBASE` 时 panic，结果 `kva - va_pa_offset`）；`KADDR` 宏在 `pmm.h` 中被注释掉了，**不可用**，内核虚拟地址用 `(void *)(page2pa(page) + va_pa_offset)` 计算；`extern uint64_t va_pa_offset;`。
- 内核通过启动页表把 `0xffffffffc0000000` 起 1GB 映射到物理 `0x80000000` 起，因此任意已分配物理页都可以经 `pa + va_pa_offset` 访问。
- `kern/mm/memlayout.h`：`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`。**`Page` 的 `flags`、`property`、`page_link` 由物理内存管理器使用**（Buddy 会在已分配块头的 `property` 里存 order），所以 SLUB 不要在 `struct Page` 里存自己的数据；需要的元数据放在 slab 页内部（页头）。
- `kern/mm/mmu.h`：`PGSIZE 4096`；`libs/defs.h`：`ROUNDUP`、`ROUNDDOWN`、`size_t`、`uintptr_t`；`libs/list.h` 双向链表；`libs/string.h`：`memset`、`memcpy`；`kern/debug/assert.h`：`assert`、`panic`；`stdio.h`：`cprintf`。
- 编译选项 `-Werror -Wall -O2 -fno-builtin -nostdinc`（不能用标准库 `malloc` 等）。

## GUARANTEE

- 新增 `slub.h`、`slub.c`；对外只暴露 `slub_init`、`kmalloc`、`kfree`、`slub_check`，其余为 `static`。
- 所有页都经 `alloc_pages` / `free_pages` 获得和归还；空 slab 必须归还给第一层（不要无限缓存空页）。
- 不在 `struct Page` 中存储 SLUB 数据；编译无警告。

## SPECIFICATION

### 数据结构
- size class：8、16、32、64、128、256、512、1024、2048 字节（写成数组）。每个 cache 记录对象大小、每个 slab 的对象数、slab 链表（区分有空闲对象的 partial 链表；full 的不挂链表）。
- slab 页内布局：页起始处放 slab 头（所属 cache 指针、空闲对象数、freelist 头、链表节点、用于校验的 magic），对象从页头之后按对象大小对齐（至少 8 字节对齐）排列。对象数 = `(PGSIZE − object_offset) / object_size`。
- 大对象：`size` 超过最大 size class 时，向第一层申请 `ROUNDUP(size + 大对象页头, PGSIZE) / PGSIZE` 页，页头记录 magic 和页数，返回页头之后的地址。

### slub_init
- Postconditions：所有 cache 的链表为空、统计清零。

### kmalloc(size)
- Case 1：`size == 0`：返回 `NULL`。
- Case 2：小对象：选最小的 ≥ size 的 size class；该 cache 的 partial 链表有 slab 就从其 freelist 取一个对象；没有就 `alloc_pages(1)` 新建 slab（初始化页头和 freelist，所有对象入 freelist）；slab 用完时移出 partial 链表。返回对象地址（内核虚拟地址，8 字节对齐）。
- Case 3：大对象：按上面的大对象规则 `alloc_pages(npages)`。
- Case 4：第一层分配失败（`alloc_pages` 返回 `NULL`）：返回 `NULL`，不改变已有状态。

### kfree(ptr)
- Case 1：`ptr == NULL`：直接返回。
- Case 2：由 `ROUNDDOWN(ptr, PGSIZE)` 找到页头，校验 magic（不对就 `panic`，打印地址）；是 slab 页：把对象放回该 slab 的 freelist（检查 ptr 正好是某个对象的起始地址，否则 `panic`）；若 slab 之前是 full，重新挂回 partial；若 slab 全部空闲，从链表摘下并 `free_pages` 归还这 1 页。
- Case 3：是大对象页：`free_pages(该页对应的 Page, 记录的页数)`。
- Requirements：从内核虚拟地址求 `struct Page *` 用 `pa2page(PADDR(kva))`。
````

---

<a id="task12"></a>

## 八、功能模块二·任务四：SLUB 分配器测试提示词（第二份）

对应过程记录：[task_12.md](ai_logs/lab2/task_12.md)

````markdown
## PROMPT

任务：针对已实现的 SLUB 两层分配器（`code/kern/mm/slub.c`，对外接口 `slub_init`、`kmalloc`、`kfree`），设计并实现测试函数 `void slub_check(void)`，检验不同大小内存对象的分配和释放是否正确。失败时用 `assert` 让内核 panic；全部通过时打印 `slub_check() succeeded!`。

操作要求：
1. 只修改 `code/kern/mm/slub.c` 中 `slub_check` 的实现，可在它前面添加只供测试使用的 `static` 辅助函数。**不得修改**已有的分配器代码。
2. 运行方式：`make DEFS=-DLAB2_SLUB_TEST` 时，`pmm_init()` 在 `check_alloc_page()` 之后调用 `slub_init(); slub_check();`，第一层是 Best-Fit。测试结束时必须把所有对象和页归还：`nr_free_pages()` 回到测试开始时的值，各 cache 的统计回到 0。

## RELY

- SLUB 实现（`slub.c`）：size class `8,16,32,64,128,256,512,1024,2048`（`size_classes[]`、`NR_CACHES`）；`struct slub_cache { object_size, object_offset, objects_per_slab, partial, nr_slabs, nr_inuse }`；`struct slab_header { magic(SLAB_MAGIC), cache, nr_free, freelist, link }` 位于 slab 页起始处，对象从 `object_offset`（= 页头大小按对象大小向上取整）开始；空闲对象前 8 字节存下一个空闲对象地址；`kmalloc` 选最小的 ≥ size 的 class，从 partial 链表第一个 slab 的 freelist 头部取对象（LIFO），slab 用满时摘出 partial；`kfree` 由 `ROUNDDOWN(ptr, PGSIZE)` 找页头并校验 magic，slab 全空时立刻 `free_pages` 归还 1 页；大对象（> 2048）按 `ROUNDUP(size + sizeof(struct large_header), PGSIZE) / PGSIZE` 页向第一层申请，页头 `{magic(LARGE_MAGIC), npages}`，返回页头之后的地址；`live_objects` 计数全部未释放对象；`kmalloc(0)` 返回 NULL，`kfree(NULL)` 直接返回。
- 第一层接口：`nr_free_pages()`、`alloc_pages`、`free_pages`（`pmm.h`）；`PGSIZE`；`memset`（`string.h`）；`assert`、`cprintf`。

## GUARANTEE

- `slub_check()` 覆盖下面列出的全部 9 组行为，通过时打印 `slub_check() succeeded!`。
- 测试结束后所有对象、页都已归还，`nr_free_pages()` 与开始时相同；不修改分配器本身；编译无警告。

## SPECIFICATION

每组前用 `cprintf` 打印组名，包含以下 9 组测试：

1. **边界输入**：`kmalloc(0) == NULL`；`kfree(NULL)` 不改变任何状态。
2. **size class 选择与对齐**：对 1、8、9、16、17、31、32、33、64、100、128、200、256、300、512、700、1024、1500、2048 字节各分配一次：返回非空、8 字节对齐、所在页页头 magic 正确、`cache->object_size` 恰好是“≥ size 的最小 class”、对象地址减去页起始位于 `[object_offset, PGSIZE)` 且 `(addr - 页起始 - object_offset) % object_size == 0`。
3. **数据不重叠**：保持上一组全部对象不释放，按各自申请的字节数填入互不相同的字节模式，然后逐个读回校验（能发现对象重叠、freelist 指针写进了在用对象、对象越界）。
4. **slab 扩展**：对某一 class 连续分配 `objects_per_slab + 1` 个对象：`nr_slabs` 增加 2，且最后一个对象和第一个对象不在同一页；第一个 slab 用满后不在 partial 链表上。
5. **复用（LIFO）**：释放一个对象后立刻再分配同一 class，应得到同一地址。
6. **空 slab 归还**：记录 `nr_free_pages()`，把第 4 组的对象全部释放后，该 class 的 `nr_slabs`、`nr_inuse` 回到原值，`nr_free_pages()` 恢复（证明空 slab 的页已还给第一层）。
7. **大对象**：`kmalloc(2049)`、`kmalloc(PGSIZE - 16)`、`kmalloc(PGSIZE)`、`kmalloc(3 * PGSIZE + 1)`：`nr_free_pages()` 的减少量分别等于按公式算出的页数；整块写满模式再读回；释放后 `nr_free_pages()` 恢复。
8. **交错压力**：用确定性的伪随机序列（线性同余，固定种子）在数组里做 512 次随机大小（1～3000 字节）的分配/释放交错，每个在用对象写入与其编号相关的模式，每次释放前校验模式；结束时全部释放，`live_objects == 0`，每个 cache `nr_inuse == 0 && nr_slabs == 0`，`nr_free_pages()` 回到测试开始时的值。
9. **失败路径**：`kmalloc((size_t)-1)` 返回 NULL；`kmalloc((nr_free_pages() + 1) * PGSIZE)` 返回 NULL；两者都不改变 `nr_free_pages()` 和 `live_objects`。
````

---

<a id="task05"></a>

## 九、分支任务：QEMU 页表翻译源码分析与 GDB 调试方案提示词

对应过程记录：[task_05.md](ai_logs/lab2/task_05.md)、[task_07.md](ai_logs/lab2/task_07.md)

````markdown
## PROMPT

目标：为“用 gdb 调试正在运行 ucore 的 QEMU，观察一次访存的虚拟地址如何被翻译成物理地址”完成 QEMU 4.1.1 源码分析与双 GDB 调试方案设计。

任务：
1. 阅读 QEMU 4.1.1 源码（含 `target/riscv`、`accel/tcg`、`include/exec`、`exec.c`、`memory.c`），给出 RISC-V 64 位客户机一次**数据 load**（例如 `ld`）从 TCG 生成的 helper 到得到物理地址、再到访问客户机内存的**完整调用路径**（函数名 + 文件:行号），以及路径上的关键分支（TLB 命中/未命中、victim TLB、`tlb_fill` 失败抛异常、MMU 模式判断、页表项有效性/权限/叶子判断、大页、A/D 位处理等）。
2. 详细解释 `target/riscv/cpu_helper.c` 中 `get_physical_address` 的 SV39 页表遍历：每一级如何从 VA 取索引、如何算 PTE 地址、如何读 PTE（读用的是哪个函数，读的是客户机物理地址）、什么条件下继续下一级、什么条件下是叶子、最后如何拼出物理地址。
3. 找出模拟 CPU 查 TLB 的 C 代码（`accel/tcg/cputlb.c` 中 `load_helper` / `tlb_hit` / `victim_tlb_hit` / `tlb_set_page_with_attrs` 等），说明 TLB 的组织（`CPUTLBEntry` 里存了什么、按什么索引、`mmu_idx` 的作用、`addend` 是什么）。
4. 分析 QEMU 软件 TLB 与真实 CPU TLB 的**逻辑区别**，并说明“未开启分页（satp 为 Bare 或 M 模式）时的访存”在 QEMU 里是否也走 TLB / `tlb_fill` / `get_physical_address`，哪个分支直接返回。
5. 设计三终端调试方案：
   - 终端 1：`./os2026.sh debug`（QEMU 带 `-s -S` 启动，等待 gdb）；终端 2：`./os2026.sh gdb-qemu`（gdb 附加到 QEMU 进程，设置 `handle SIGPIPE nostop noprint` 和 `set remotetimeout unlimited`）；终端 3：`./os2026.sh gdb`（riscv gdb 连 ucore）。
   - 选定观察目标：(a) 一条**开启分页之后**的内核访存；(b) 一条**开启分页之前**（`entry.S` 里写 satp 之前）的访存，用于第 4 问对比。
   - 条件断点：在 `get_physical_address`、`riscv_cpu_tlb_fill`、`load_helper` 上设置以地址为条件的断点；注意排除 gdb 读内存触发的调试访问（调用栈里有 `cpu_memory_rw_debug`）。
   - 每个断点命中后要打印的变量（`addr`、`mode`、`base`、`levels`、`ptesize`、`idx`、`pte_addr`、`pte`、`ppn`、`*physical`、`tlb_entry->addr_read`、`addend`）以及 `bt`。
6. 列出答辩时最值得演示的 3 分钟主线：从哪个断点开始、单步几次、看哪几个变量。

## RELY

- QEMU 版本 4.1.1，以 `--enable-debug` 编译，`-machine virt`，OpenSBI v0.4。容器内源码位于 `/opt/qemu-src/qemu-4.1.1`。
- `get_physical_address`（`target/riscv/cpu_helper.c:158`）和 `riscv_cpu_tlb_fill`（同文件 `:438`）可直接下断点；gdb 读客户机内存时也会走 `get_physical_address`，调用栈中有 `cpu_memory_rw_debug`。
- ucore 侧（`code/`）：`kern/init/entry.S` 在 `kern_entry` 里构造 `satp`（Sv39，`boot_page_table_sv39`）后 `csrw satp`、`sfence.vma`，再跳到高地址的 `kern_init`；启动页表把 `0xffffffffc0000000` 起 1GB 映射到物理 `0x80000000`（1GB 大页）。
- 指导书：`实验指导书/lab2/lab2_4_qemu_mmu_debug.md`（6 个调试要求）。

## GUARANTEE

- 生成可直接 `source` 的宿主侧与客户机侧 GDB 调试脚本。
- 分析中的每个结论均附 QEMU 源码 `文件:行号`。

## SPECIFICATION

- Preconditions：P0（`page_init`、Best-Fit）已完成并编译通过；QEMU 4.1.1 调试版与源码就绪。
- Postconditions：输出源码分析文档与 GDB 脚本，覆盖指导书全部 6 个调试问题。
- Edge cases：
  - 若某函数被内联无法直接按函数名下断点，给出对应的源码行号断点位置。
  - 说明 1GB 大页（Sv39 根页表项即为叶子）对页表遍历层数的影响，并对比 4KB 标准三层页表的遍历分支。
````
