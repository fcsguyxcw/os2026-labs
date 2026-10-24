# Task 11：SLUB 任意大小内存对象分配（功能模块二 任务四 · 第一份提示词：实现）

执行者：Codex w1:p3（Implementation Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Implementation Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`，代码在 `code/`。你的沙箱无法访问 Docker，不要尝试编译运行；Leader 会在 Docker 镜像 `os2026-dbg` 中编译运行并反馈真实输出。

任务：在没有现成代码框架的情况下，从零实现一个简化的 **SLUB 两层内存分配器**并接入现有内核：
- 第一层：按页分配。直接使用内核已有的 `alloc_pages(n)` / `free_pages(base, n)`（当前系统物理内存管理器是 Best-Fit），不要自己再管理物理页。
- 第二层：在第一层之上按任意字节大小分配。对外提供 `void *kmalloc(size_t size)` 和 `void kfree(void *ptr)`。小对象按大小分级（size class），每一级是一个 cache；每个 cache 由若干 slab 组成，**一个 slab 就是第一层分出来的 1 个物理页**，页内切成等大的对象；空闲对象用“对象内嵌的空闲指针”串成该 slab 的 freelist（SLUB 的做法：空闲时对象自身的前 8 字节存下一个空闲对象地址）。大对象（超过最大 size class）直接按页向第一层申请。

操作要求：
1. 必须直接修改实际文件。新建 `code/kern/mm/slub.h`、`code/kern/mm/slub.c`（`kern/mm/` 下的 `.c` 自动编译，不改 Makefile）。
2. `slub.h` 声明：`void slub_init(void);`、`void *kmalloc(size_t size);`、`void kfree(void *ptr);`、`void slub_check(void);`。
3. **`slub_check` 本任务不实现**——测试函数由下一份提示词（Task 12）单独生成。本任务在 `slub.c` 末尾写占位实现：`kmalloc(32)` 一次、写入并读回一个字节、`kfree`，然后 `cprintf("slub_check: placeholder\n")`。
4. 接入方式：Leader 已在 `pmm.c` 加好编译开关——`make DEFS=-DLAB2_SLUB_TEST` 时，`pmm_init()` 在 `check_alloc_page()` 之后调用 `slub_init(); slub_check();`。默认编译不调用 SLUB。你不要修改 `pmm.c` 或任何已有文件；不 git commit / push。
5. 完成后保存 `git diff --no-index /dev/null code/kern/mm/slub.c > <lab2 日志目录>/evidence/t11_slub.diff`（返回 1 正常）和 `.h` 的 `t11_slub_h.diff`，回复 ≤12 行：size class 列表、slab 页内布局、如何从对象指针找回所属 slab/cache、大对象如何记录页数、最不确定的点。

输出要求（Leader 会验证）：`make DEFS=-DLAB2_SLUB_TEST` 编译无警告，内核打印 `slub_check: placeholder`，无 panic；默认 `make grade` 仍 20/20。

## RELY

均来自 2026-10-24 仓库代码：

- `kern/mm/pmm.h`：`struct Page *alloc_pages(size_t n); void free_pages(struct Page *base, size_t n); size_t nr_free_pages(void);`；`page2pa(page)`；`pa2page(pa)`（越界 panic）；`PADDR(kva)`（`kva < KERNBASE` 时 panic，结果 `kva - va_pa_offset`）；`KADDR` 宏在 `pmm.h` 中被注释掉了，**不可用**，内核虚拟地址用 `(void *)(page2pa(page) + va_pa_offset)` 计算；`extern uint64_t va_pa_offset;`。
- 内核通过启动页表把 `0xffffffffc0000000` 起 1GB 映射到物理 `0x80000000` 起，因此任意已分配物理页都可以经 `pa + va_pa_offset` 访问。
- `kern/mm/memlayout.h`：`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`。**`Page` 的 `flags`、`property`、`page_link` 由物理内存管理器使用**（Best-Fit 在页被分配出去后不再读它们，但 Buddy 会在已分配块头的 `property` 里存 order），所以 SLUB 不要在 `struct Page` 里存自己的数据；需要的元数据放在 slab 页内部（页头）。
- `kern/mm/mmu.h`：`PGSIZE 4096`；`libs/defs.h`：`ROUNDUP`、`ROUNDDOWN`、`size_t`、`uintptr_t`；`libs/list.h` 双向链表；`libs/string.h`：`memset`、`memcpy`；`kern/debug/assert.h`：`assert`、`panic`；`stdio.h`：`cprintf`。
- 编译选项 `-Werror -Wall -O2 -fno-builtin -nostdinc`（不能用标准库 `malloc` 等）。

## GUARANTEE

- 新增 `slub.h`、`slub.c`；对外只暴露 `slub_init`、`kmalloc`、`kfree`、`slub_check`，其余为 `static`。
- 所有页都经 `alloc_pages` / `free_pages` 获得和归还；空 slab 必须归还给第一层（不要无限缓存空页）。
- 不在 `struct Page` 中存储 SLUB 数据；不修改任何已有文件；编译无警告。

## SPECIFICATION

### 数据结构
- size class：至少 8、16、32、64、128、256、512、1024、2048 字节（可调整，写成数组）。每个 cache 记录对象大小、每个 slab 的对象数、slab 链表（至少区分“有空闲对象的 partial 链表”；full 的可以不挂链表）。
- slab 页内布局：页起始处放 slab 头（所属 cache 指针、空闲对象数或已用对象数、freelist 头、链表节点、一个用于校验的 magic），对象从页头之后按对象大小对齐（至少 8 字节对齐）排列。对象数 = (PGSIZE − 页头大小) / 对象大小。
- 大对象：`size` 超过最大 size class 时，向第一层申请 `ROUNDUP(size + 大对象页头, PGSIZE) / PGSIZE` 页，页头记录 magic 和页数，返回页头之后的地址。

### slub_init
- Postconditions：所有 cache 的链表为空、统计清零。可重复调用前提是没有未释放对象（不要求支持）。

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

## Agent Result

Codex 回复（2026-10-24 21:58）：新建 `slub.h`、`slub.c`，未改已有文件。size class 8～2048 共 9 级；slab 页头保存 magic、cache、空闲计数、freelist、链表节点，对象区按 class 对齐；释放时按页向下对齐找页头并校验对象边界，空 slab 立即归还；大对象页头记录页数，不借用 `Page` 字段；占位 `slub_check`。偏差：按“只新建两个文件”的理解，没有生成 PROMPT 要求的 diff 证据文件（Leader 补生成 `evidence/t11_slub.diff`、`t11_slub_h.diff`）。

## Verification

Leader（2026-10-24）：
- 代码审查：所有页经 `alloc_pages`/`free_pages`；不写 `struct Page`；`kfree` 校验地址范围、magic、对象边界，并沿 freelist 检查重复释放（spec 之外的额外检查）。NOTE：`object_offset = ROUNDUP(页头大小, object_size)`，对象大小 ≥ 页头时对象区从 `object_size` 偏移开始，2048 级每个 slab 只能放 1 个对象、1024 级 3 个，空间利用率低，属简化实现的代价；大对象需要额外 16 字节页头，`kmalloc(PGSIZE)` 要占 2 页。
- `make DEFS=-DLAB2_SLUB_TEST` + `make qemu`（`evidence/t11_slub_qemu.txt`）：编译无 warning/error；输出 `memory management: best_fit_pmm_manager`、`check_alloc_page() succeeded!`、`slub_check: placeholder`、两行 satp，无 panic（日志中 `make: warning: Clock skew detected` 来自宿主与容器文件时间戳差异，非编译警告）。
- 默认 `make grade`（`evidence/t11_grade_default.txt`）：20/20。

状态：实现 **DONE**；正确性要等 Task 12 的测试。
