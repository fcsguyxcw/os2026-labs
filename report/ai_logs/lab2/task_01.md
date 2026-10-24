# Task 01：实现 page_init（功能模块一 任务一）

执行者：Codex（Implementation + Test Agent，Herdr 面板 w1:p2）
Leader 发出时间：2026-10-24（北京时间）

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

## Agent Result

Codex 回复（2026-10-24 21:17）摘要：
- 在 `pmm.c` 第 64 行实现 `page_init()`，注释改为 `//lab2 function1 2412172`，未改其它源码。
- 实现：设备树范围校验（base/size 为 0 或溢出时 panic，base ≠ DRAM_BASE 时 panic）→ 打印 `physical memory map:` → `va_pa_offset = PHYSICAL_MEMORY_OFFSET` → 上界取 `min(设备树末端, PHYSICAL_MEMORY_END)` 并向下按页对齐 → `npage = PPN(上界)` → `pages = ROUNDUP(end, PGSIZE)`，全部描述符置 `PG_reserved` → `freemem = ROUNDUP(PADDR(&pages[npage - nbase]), PGSIZE)` → 有完整页时 `init_memmap`，否则 `cprintf` 说明原因。
- **偏差**：第一条 Docker 命令退出码 1，Codex 沙箱访问 OrbStack 的 docker.sock 时 permission denied，未编译、未运行 QEMU、未跑 grade。按 spec 停止并报告。证据：`evidence/t1_qemu.txt`（只有错误）、`evidence/t1_pmm.diff`。

## Verification

Leader 复核：
- 代码审查：逐条对照 SPECIFICATION，Case 1～4 与 Postconditions 均有对应代码；虚拟地址（`end`、`pages`）只经 `PADDR` 转物理地址，下标 = 页号 − `nbase`。观察：描述符数组位于 BSS 之外，`page_init` 只对其置位 `PG_reserved` 而没有先清零其余字段；QEMU 启动时内存为零，且空闲页的字段由 `init_memmap` 全部重置，因此不影响本实验结果（记为 NOTE，写进报告的改进空间）。
- 由于子 Agent 无法使用 Docker，编译和运行由 Leader 在整合 Best-Fit 之后完成（见 Task 02 的 Verification 和 `evidence/t3_qemu.txt`、`evidence/t3_grade.txt`）：输出 `physical memory map:` / `memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff].`，`make grade` 中 `check page init: OK`。

状态：**DONE**（实现 + 编译 + QEMU 运行 + grade Test 1 通过；交叉 review 见 Task 03）。

## 大白话小结

`page_init` 是内核“清点家底”的一步：先问设备树“一共有多少内存、从哪开始”，再给每一页物理内存配一张“登记卡”（`struct Page`），这些卡片连续放在内核代码后面。OpenSBI、内核自己、还有这叠登记卡占用的页都要标成“保留”，不能分出去；剩下的完整页面才一次性交给分配器（`init_memmap`）。最容易错的是地址：`end`、`pages` 是内核看到的虚拟地址，要用 `PADDR` 减去偏移才是物理地址，页号减去 `nbase` 才是卡片下标。Review 还发现，如果内存小到连登记卡都放不下，必须在写卡片之前就停下（Bug 2）。
