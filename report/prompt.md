# Lab2 Prompt 汇总

本文件按任务保存实际四段式提示词。以下从对应 ai_logs 文件逐字复制 `## PROMPT` 至 `## SPECIFICATION` 末尾，保留当时的路径、操作要求和验证计划；历史计划不等于实际执行结果，后者见报告及 ai_logs 的复核记录。

<a id="task01"></a>

## Task 01：功能模块一·任务一：page_init()

发送对象：Codex CLI，Implementation/Test Agent，Herdr 面板 w1:p2；发送时间：2026-10-24（北京时间，具体发送时分 TODO）；对应 ai_logs 文件：[task_01.md](ai_logs/lab2/task_01.md)。

````markdown
## PROMPT

你是 OS Lab2 的 Implementation/Test Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，当前分支 `lab2`，实验代码在 `code/`。运行环境是 macOS + Docker（OrbStack），必须在镜像 `os2026-dbg`（RISC-V GCC 13.2.0、QEMU 4.1.1）里编译和运行，不要用 Mac 本机或系统自带的 QEMU 8.2.2（lab2 在 8.2.2 上没有任何输出）。

任务：在 `code/kern/mm/pmm.c` 第 64 行注释 `//lab2 function1 YOUR CODE` 的位置实现 `static void page_init(void);`，它被同文件后面的 `pmm_init()` 调用。功能：根据设备树提供的物理内存信息建立 `Page` 描述符数组，把排除了 OpenSBI、内核镜像和描述符数组本身之后剩余的完整空闲页面交给当前 `pmm_manager`（通过同文件里的 `init_memmap()`）。

操作要求：
1. 必须直接修改仓库里的实际文件，不要只展示代码。
2. 只允许修改 `code/kern/mm/pmm.c`，并且只在第 64 行注释位置添加 `page_init` 及其需要的静态辅助代码；把这行注释改为 `//lab2 function1 2412172`（保留 `lab2 function1`，把 `YOUR CODE` 换成学号）。
3. 禁止修改：`pmm.c` 里其它已有函数（包括 `init_pmm_manager()` 里的 `pmm_manager = &default_pmm_manager;`，切换管理器由 Leader 另行完成）、任何头文件、`kern/init/entry.S`、`Makefile`、`tools/*`、`kern/driver/*`、`default_pmm.c`、`best_fit_pmm.c`。不要 git commit / push，不要删除仓库已有文件，不要安装软件或改系统配置。
4. 另一个 Agent 正在同时修改 `code/kern/mm/best_fit_pmm.c`。仓库原样骨架里该文件引用了未声明的 `free_list`、`nr_free`，所以**仓库目录本身现在编译不过**。编译验证请在临时副本里做：
   ```bash
   rm -rf /Users/wesley/大三上课程/操作系统/tmp/lab2-task01
   mkdir -p /Users/wesley/大三上课程/操作系统/tmp
   cp -R /Users/wesley/大三上课程/操作系统/os2026-labs/code /Users/wesley/大三上课程/操作系统/tmp/lab2-task01
   rm -rf /Users/wesley/大三上课程/操作系统/tmp/lab2-task01/bin /Users/wesley/大三上课程/操作系统/tmp/lab2-task01/obj
   rm /Users/wesley/大三上课程/操作系统/tmp/lab2-task01/kern/mm/best_fit_pmm.c   # 只在副本里删，副本仍使用 default_pmm_manager
   ```
   每次改完仓库里的 `pmm.c` 后重新复制一遍再测。副本里不要做其它改动。
5. 运行命令（输出全部保存，不要编造）：
   ```bash
   cd /Users/wesley/大三上课程/操作系统/tmp/lab2-task01
   docker run --rm -v "$PWD:/code" -w /code os2026-dbg bash -c 'make 2>&1; timeout 10 make qemu' > <evidence>/t1_qemu.txt 2>&1
   docker run --rm -v "$PWD:/code" -w /code os2026-dbg bash -c 'make grade' > <evidence>/t1_grade.txt 2>&1
   ```
   `<evidence>` = `/Users/wesley/大三上课程/操作系统/os2026-labs/report/ai_logs/lab2/evidence`（不存在就创建）。`make qemu` 不会自己退出，退出码 124 是正常的。如果 docker 报连不上 daemon，停止并报告，不要自己处理。
   另外把 `git -C /Users/wesley/大三上课程/操作系统/os2026-labs diff code/kern/mm/pmm.c` 保存为 `<evidence>/t1_pmm.diff`。

输出要求：
- QEMU 输出中出现 `physical memory map:` 和形如 `  memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff].` 的一行（大小、起止地址以实际为准，末地址是闭区间），之后 `default_check` 通过并打印 `check_alloc_page() succeeded!` 和 `satp ...` 两行，无 panic。
- `make grade` 里 Test 1（page init）得满分；副本里没有 Best-Fit，Test 2 失败是预期的。
- 完成后回复 ≤15 行：做了什么、两条命令的结果、grade 分数、遇到的问题和偏差。不要编造未实际运行得到的输出。

## RELY

均来自 2026-10-24 仓库代码：

- 全局变量（`pmm.c:15-22`）：`struct Page *pages;`（描述符数组的**虚拟地址**）、`size_t npage`（可管理的物理页号上界，注意是 PPN 上界，不是数组长度）、`uint64_t va_pa_offset;`、`const size_t nbase = DRAM_BASE / PGSIZE;`（= 0x80000）。
- `pmm.c`：`static void init_memmap(struct Page *base, size_t n)` 调用 `pmm_manager->init_memmap(base, n)`；`pmm_init()` 依次调用 `init_pmm_manager()`、`page_init()`、`check_alloc_page()`。
- `kern/driver/dtb.h`：`uint64_t get_memory_base(void); uint64_t get_memory_size(void);`，由 `dtb_init()`（在 `pmm_init` 之前运行）从设备树解析；设备树无效或缺失时两者都返回 0。
- `kern/mm/memlayout.h`：`KERNBASE 0xFFFFFFFFC0200000`、`PHYSICAL_MEMORY_END 0x88000000`（内核能管理的物理地址上界）、`PHYSICAL_MEMORY_OFFSET 0xFFFFFFFF40000000`（内核虚拟地址 − 物理地址）、`KERNEL_BEGIN_PADDR 0x80200000`；`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`；`SetPageReserved/ClearPageReserved/PageReserved`（bit `PG_reserved`=0）、`SetPageProperty/ClearPageProperty/PageProperty`（bit `PG_property`=1）。
- `kern/mm/pmm.h`：`PADDR(kva)`（`kva < KERNBASE` 会 panic，结果 `kva - va_pa_offset`）；`page2ppn(page) = page - pages + nbase`；`page2pa(page) = page2ppn(page) << PGSHIFT`；`pa2page(pa)`：`PPN(pa) >= npage` 时 panic，否则返回 `&pages[PPN(pa) - nbase]`。所以 `pages[0]` 对应物理地址 `DRAM_BASE`，数组长度是 `npage - nbase`。
- `kern/mm/mmu.h`：`PGSIZE 4096`、`PGSHIFT 12`、`PPN(la)`；`libs/riscv.h`：`DRAM_BASE 0x80000000`；`libs/defs.h`：`ROUNDUP(a, n)`、`ROUNDDOWN(a, n)`。
- `tools/kernel.ld`：`PROVIDE(end = .);`，`end` 是内核镜像结尾的**虚拟地址**（在 C 里 `extern char end[];`）。
- `kern/debug/assert.h`：`panic(...)`；`cprintf` 来自 `stdio.h`（`pmm.c` 已包含）。
- 内存布局（指导书 `实验指导书/lab2/lab2_3_5_2_search_phymem_layout.md`）：OpenSBI 占 `[0x80000000, 0x80200000)`；内核占 `[0x80200000, KernelEnd)`；`Page` 描述符数组连续排在内核镜像之后，起点为 `end` 向上按页对齐；数组末尾用 `PADDR` 转成物理地址后再向上按页对齐，才是空闲区起点；设备树报告的内存末端（还要受 `PHYSICAL_MEMORY_END` 限制）向下按页对齐；只有两者之间非空的完整页面区间才能交给 `init_memmap()`；在空闲区建立之前，所有描述符都应先处于保留状态。`end` 是虚拟地址、`KernelEnd` 是物理地址，不能混用。
- 自动检查 `tools/test_lab2.c`（`make grade` 在临时副本里注入到 `pmm_init()` 末尾）Test 1 检查：`va_pa_offset == PHYSICAL_MEMORY_OFFSET`；`get_memory_base()`、`get_memory_size()` 非 0；`npage == PPN(mem_base + mem_size)` 且 `npage > nbase`；`pages` 非空、按页对齐、`>= ROUNDUP(end, PGSIZE)`；`nr_free_pages()` 非 0 且 `<= npage - nbase`；`pa2page(PADDR(end))` 和 `pages` 数组起始物理页都是 `PageReserved`；`pa2page(page2pa(q)) == q`。

## GUARANTEE

- 实现 `static void page_init(void)`，可以添加只在 `pmm.c` 内部使用的 `static` 辅助函数。
- 成功返回时：`va_pa_offset`、`npage`、`pages` 已正确设置；已打印 `physical memory map:` 及内存范围行；当前 `pmm_manager` 已通过 `init_memmap()` 注册了唯一一段初始空闲区。
- 内核镜像、描述符数组所在的页面，以及 `[DRAM_BASE, 0x80200000)`，都不在空闲区内且保持 `PG_reserved`。
- 不破坏任何现有接口；编译无警告（编译选项含 `-Werror`）。
- 副本中 `make qemu` 无 panic 并打印 `check_alloc_page() succeeded!`；`make grade` Test 1 满分。

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

<a id="task02"></a>

## Task 02：功能模块二·任务二：Best-Fit

发送对象：Codex CLI，Implementation/Test Agent，Herdr 面板 w1:p3；发送时间：2026-10-24（北京时间，具体发送时分 TODO）；对应 ai_logs 文件：[task_02.md](ai_logs/lab2/task_02.md)。

````markdown
## PROMPT

你是 OS Lab2 的 Implementation/Test Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，当前分支 `lab2`，实验代码在 `code/`。运行环境是 macOS + Docker（OrbStack），必须在镜像 `os2026-dbg`（RISC-V GCC 13.2.0、QEMU 4.1.1）里编译和运行，不要用 Mac 本机或系统自带的 QEMU 8.2.2。

任务：在 `code/kern/mm/best_fit_pmm.c` 第 6 行注释 `//lab2 function2-2 YOUR CODE` 的位置，实现完整的 Best-Fit 连续物理内存分配模块，并定义 `const struct pmm_manager best_fit_pmm_manager` 完成注册（`best_fit_pmm.h` 已声明 `extern const struct pmm_manager best_fit_pmm_manager;`），使它能替换内核当前使用的 First-Fit 管理器 `default_pmm_manager`。

Best-Fit 的含义：分配 n 页时，在所有长度 ≥ n 的空闲连续块中，选择**长度最小**的那一块；长度相同时选地址最低的那一块。块比 n 大时，从块头切出 n 页分配出去，剩余部分成为新的空闲块。释放时把页面放回空闲链表，并与地址上紧邻的前后空闲块合并。

操作要求：
1. 必须直接修改仓库里的实际文件，不要只展示代码。
2. 只允许修改 `code/kern/mm/best_fit_pmm.c`：在第 6 行注释位置写入实现（内部状态、管理函数、`best_fit_pmm_manager` 定义；`best_fit_pmm_manager` 的 `.check` 要指向本文件的 `best_fit_check`，因为它定义在注释位置之后，需要在注释位置先写前向声明 `static void best_fit_check(void);`，或者把 `best_fit_pmm_manager` 的定义放到文件末尾，二选一）。把这行注释改为 `//lab2 function2-2 2412172`（保留 `lab2 function2-2`，把 `YOUR CODE` 换成学号）。
3. 禁止修改：`basic_check()`、`best_fit_check()` 两个测试函数（一个字符都不要动）；任何头文件中已有的结构体、宏和接口；`pmm.c`（另一个 Agent 正在改它，切换管理器由 Leader 完成）、`default_pmm.c`、`kern/init/entry.S`、`Makefile`、`tools/*`、`kern/driver/*`。不要 git commit / push，不要删除仓库已有文件，不要安装软件或改系统配置。
4. 编译验证分两步：
   - 第一步（现在就做）：仓库的 `pmm.c` 里 `page_init()` 还没有实现，内核暂时链接不过。先只检查本文件能否单独编译通过：
     ```bash
     cd /Users/wesley/大三上课程/操作系统/os2026-labs/code
     docker run --rm -v "$PWD:/code" -w /code os2026-dbg bash -c 'make obj/kern/mm/best_fit_pmm.o 2>&1 || make 2>&1 | grep -n best_fit' > <evidence>/t2_compile.txt 2>&1
     ```
     （如果 Makefile 没有这个单文件目标，就跑 `make` 并确认报错里**没有**任何来自 `best_fit_pmm.c` 的错误或警告，剩下的只应是 `page_init` 未定义。）跑完删掉仓库里生成的 `code/bin`、`code/obj`。然后**对照下方 SPECIFICATION 和两个测试函数逐条自查**，回复 Leader，并等待通知。
   - 第二步（等 Leader 通知 `page_init` 已完成后再做）：复制副本，在副本里把 `init_pmm_manager()` 中的 `&default_pmm_manager` 改成 `&best_fit_pmm_manager`，然后运行：
     ```bash
     rm -rf /Users/wesley/大三上课程/操作系统/tmp/lab2-task02 && mkdir -p /Users/wesley/大三上课程/操作系统/tmp
     cp -R /Users/wesley/大三上课程/操作系统/os2026-labs/code /Users/wesley/大三上课程/操作系统/tmp/lab2-task02
     rm -rf /Users/wesley/大三上课程/操作系统/tmp/lab2-task02/bin /Users/wesley/大三上课程/操作系统/tmp/lab2-task02/obj
     # 副本里改 init_pmm_manager 这一行
     cd /Users/wesley/大三上课程/操作系统/tmp/lab2-task02
     docker run --rm -v "$PWD:/code" -w /code os2026-dbg bash -c 'make 2>&1; timeout 10 make qemu' > <evidence>/t2_qemu.txt 2>&1
     docker run --rm -v "$PWD:/code" -w /code os2026-dbg bash -c 'make grade' > <evidence>/t2_grade.txt 2>&1
     ```
   `<evidence>` = `/Users/wesley/大三上课程/操作系统/os2026-labs/report/ai_logs/lab2/evidence`（不存在就创建）。`make qemu` 不会自己退出，退出码 124 正常。docker 报连不上 daemon 时停止并报告。
   最后把 `git -C /Users/wesley/大三上课程/操作系统/os2026-labs diff code/kern/mm/best_fit_pmm.c` 保存为 `<evidence>/t2_best_fit.diff`。

输出要求：
- 必须通过本文件的 `basic_check()` 和 `best_fit_check()`（由 `pmm_manager->check()` 调用）；内核输出 `memory management: best_fit_pmm_manager`、`check_alloc_page() succeeded!`，无 panic。
- `make grade` 总分 20/20（Test 2 检查 `pmm_manager == &best_fit_pmm_manager`）。
- 每一步完成后回复 ≤15 行：做了什么、命令结果、遇到的问题和偏差。不要编造未实际运行得到的输出。

## RELY

均来自 2026-10-24 仓库代码：

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
  `alloc_page()` = `alloc_pages(1)`，`free_page(p)` = `free_pages(p, 1)`；`page_ref`、`set_page_ref`、`page2pa`。
- `kern/mm/memlayout.h`：`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`；`PG_reserved`（bit 0）、`PG_property`（bit 1，置位表示该页是一个空闲块的头页）及 `SetPageReserved/ClearPageReserved/PageReserved/SetPageProperty/ClearPageProperty/PageProperty`；`le2page(le, member)`；`typedef struct { list_entry_t free_list; unsigned int nr_free; } free_area_t;`。
- `libs/list.h`：`list_init`、`list_add`（= `list_add_after`）、`list_add_before`、`list_add_after`、`list_del`、`list_empty`、`list_next`、`list_prev`。
- `kern/debug/assert.h`：`assert`；`libs/string.h` 未包含时需要自行 include。
- 参考实现 `kern/mm/default_pmm.c`（First-Fit）：`static free_area_t free_area;`、`#define free_list (free_area.free_list)`、`#define nr_free (free_area.nr_free)`；空闲链表按地址升序；空闲块头页 `property` = 块长度且置 `PG_property`，块内其它页 `property = 0`；`init_memmap` 初始化每页（`assert(PageReserved(p))`、`flags = property = 0`、`set_page_ref(p, 0)`），按地址插入；`free_pages` 后与前后相邻块合并。
- **测试函数直接使用的名字和约定**（`best_fit_pmm.c` 里 `basic_check`、`best_fit_check`）：
  - 直接读写 `free_list`（`list_entry_t` 左值，被整体保存/恢复：`free_list_store = free_list; list_init(&free_list); ... free_list = free_list_store;`）和 `nr_free`（`unsigned int` 左值）。所以本文件里必须有这两个同名对象（建议沿用 `free_area` + 两个宏）。
  - 遍历 `free_list` 时，每个节点 `le2page(le, page_link)` 都必须 `PageProperty(p)` 为真，所有节点 `p->property` 之和 == `nr_free_pages()`；空闲链表上只放块的头页。
  - `alloc_pages(5)` 返回的 `p0` 满足 `!PageProperty(p0)`。
  - 在清空的链表上依次 `free_pages(p0 + 1, 2)`、`free_pages(p0 + 4, 1)` 后：`alloc_pages(4) == NULL`；`PageProperty(p0 + 1) && p0[1].property == 2`；随后 `alloc_pages(1)` 必须返回 `p0 + 4`（长度 1 的块比长度 2 的块更合适），`alloc_pages(2)` 非空。
  - `free_pages(p0, 5)` 时，`p0` 的 5 页里有已经在空闲链表上的头页之外的页（测试先把 `p0+1`、`p0+4` 分配掉了），之后 `alloc_pages(5)` 必须能拿到整块 5 页——释放和合并必须正确。
  - 测试结束时 `nr_free`、链表恢复原状，`count == 0 && total == 0`。

## GUARANTEE

- 定义 `const struct pmm_manager best_fit_pmm_manager`，`name` 为 `"best_fit_pmm_manager"`，6 个函数指针全部指向本文件实现的 `static` 函数，`.check` 指向 `best_fit_check`。
- 对外接口只通过 `best_fit_pmm_manager`；可以添加只在本文件内部使用的 `static` 辅助函数。
- `basic_check()`、`best_fit_check()` 与原文件逐字一致。
- 编译无警告（`-Werror`）。
- 在已实现 `page_init` 的副本中：内核运行通过两个测试函数，`make grade` 20/20。

## SPECIFICATION

### best_fit_init
- Preconditions：内核启动时由 `init_pmm_manager()` 调用一次。
- Postconditions：`free_list` 为空链表，`nr_free == 0`。

### best_fit_init_memmap(base, n)
- Preconditions：`n > 0`；`base[0..n-1]` 都是 `PG_reserved` 状态的描述符，物理上连续，且与已有空闲块不重叠。
- Postconditions：每页 `flags` 清零（包括 `PG_reserved`）、`property = 0`、`ref = 0`；`base->property = n` 并置 `PG_property`；`base` 按地址升序插入 `free_list`；`nr_free += n`。
- Edge cases：`n == 0` 时 `assert` 失败。

### best_fit_alloc_pages(n)
- Preconditions：`n > 0`；`free_list` 与 `nr_free` 正确维护。
- Case 1（`n > nr_free`，或没有长度 ≥ n 的块）：返回 `NULL`；`free_list`、`nr_free`、所有页状态都不变。
- Case 2（找到块）：遍历整个 `free_list`，选 `property >= n` 中 `property` 最小的块（相同时取地址最低，即遍历中先遇到的那个）。把该块从链表摘下；若 `property > n`，`base + n` 成为新空闲块头：`property = 原长度 − n`、置 `PG_property`，插回原块在链表中的位置（保持地址升序）。被分配块头页 `ClearPageProperty`；`nr_free -= n`；返回块头页。
- Case 3（`n == 0`）：`assert` 失败。
- Requirements：必须完整遍历后再决定，不能遇到第一个够用的块就返回（那是 First-Fit）。

### best_fit_free_pages(base, n)
- Preconditions：`n > 0`；`base[0..n-1]` 是此前分配出去的页，不是 `PG_reserved`，也不是空闲块头。
- Postconditions：每页 `flags` 清零、`ref = 0`；`base->property = n`，置 `PG_property`；按地址升序插入 `free_list`；`nr_free += n`；若前一个空闲块的结尾正好是 `base`，与之合并；若 `base + base->property` 正好是后一个空闲块的头，与之合并；合并后被吸收的头页 `ClearPageProperty` 并移出链表，`property` 清 0。
- Edge cases：`n == 0` 时 `assert` 失败；`free_list` 为空时直接插入。

### best_fit_nr_free_pages
- Postconditions：返回 `nr_free`。

````

<a id="task03"></a>

## Task 03：Review 提示词：page_init 与 Best-Fit 交叉审查

发送对象：Codex CLI，Review Agent，Herdr 面板 w1:p2、w1:p3；发送时间：2026-10-24（北京时间，具体发送时分 TODO）；对应 ai_logs 文件：[task_03.md](ai_logs/lab2/task_03.md)。

````markdown
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

````

<a id="task04"></a>

## Task 04：page_init 描述符越界修复

发送对象：Codex CLI，Debug Agent，Herdr 面板 w1:p2；发送时间：2026-10-24（北京时间，具体发送时分 TODO）；对应 ai_logs 文件：[task_04.md](ai_logs/lab2/task_04.md)。

````markdown
## PROMPT

你是 OS Lab2 的 Debug Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`。你的沙箱无法访问 Docker，不要尝试编译运行；Leader 会在 Docker 中验证。

问题（来自 `evidence/t3_review_page_init.md` 的 MUST_FIX）：`code/kern/mm/pmm.c` 的 `page_init()` 在把 `pages[0 .. npage-nbase)` 全部置 `PG_reserved` 之前，没有确认描述符数组本身落在可管理的物理内存内。若设备树报告的内存很小（例如 base=0x80000000、size=0x208000，内核仍放得下），描述符数组末端会超过内存末端，循环写入越界内存。

任务：只修改 `code/kern/mm/pmm.c` 里的 `page_init()`，在写入任何描述符之前增加检查。

操作要求：
1. 直接修改实际文件；只改 `page_init()` 函数体，其它代码（包括 Leader 改过的 `init_pmm_manager()`、学号注释行）一律不动。
2. 不要 git commit / push，不要改其它文件。
3. 完成后把 `git diff code/kern/mm/pmm.c` 保存为 `<lab2 日志目录>/evidence/t4_pmm.diff`，回复 ≤8 行。

## RELY

- 当前 `page_init()` 实现见 `git diff code/kern/mm/pmm.c`（Task 01 产物）。
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
- 其余 Preconditions / Postconditions 与 Task 01 相同。

````

## 功能模块二·任务三：Buddy System

### 实现提示词

TODO

### 测试提示词

TODO

## 功能模块二·任务四：SLUB

### 实现提示词

TODO

### 测试提示词

TODO
