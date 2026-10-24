# Task 09：Buddy System 分配模块（功能模块二 任务三 · 第一份提示词：实现）

执行者：Codex w1:p2（Implementation Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Implementation Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`，代码在 `code/`。你的沙箱无法访问 Docker，不要尝试编译运行；Leader 会在 Docker 镜像 `os2026-dbg` 里编译、运行并把真实输出反馈给你。

任务：参考现有 ucore 物理内存管理框架（`struct pmm_manager`，以及 `default_pmm.c` 的 First-Fit、`best_fit_pmm.c` 的 Best-Fit 写法），从零实现一个 **Buddy System（伙伴系统）物理页分配器**，作为新的 `pmm_manager` 接入内核。

Buddy System 的含义：所有空闲块的大小都是 2 的幂（1、2、4、8… 页），一个 order 为 k 的块包含 2^k 页，且其起始页下标（相对于本管理器管理区域的起点）是 2^k 的整数倍。分配 n 页时，把 n 向上取整到 2^k，从 order k 的空闲链表取块；没有就从更大的 order 取一块，逐级对半拆分，拆出的另一半（伙伴）放回对应 order 的链表。释放时，检查同 order 的伙伴块（下标为 `idx ^ (1 << k)`）是否空闲且 order 相同，是就合并成 order k+1 的块并继续向上检查，直到伙伴不空闲或到达最大 order。

操作要求：
1. 必须直接修改实际文件。新建 `code/kern/mm/buddy_pmm.h` 和 `code/kern/mm/buddy_pmm.c`（`kern/mm/` 下的 `.c` 会被 Makefile 自动编译，不需要改 Makefile）。
2. `buddy_pmm.h` 声明 `extern const struct pmm_manager buddy_pmm_manager;` 和 `void buddy_check(void);`。
3. `buddy_pmm.c` 定义 `buddy_pmm_manager`（`name = "buddy_pmm_manager"`），`.check` 指向 `buddy_check`。**`buddy_check` 本任务不实现**——测试函数由下一份提示词（Task 10）单独生成；本任务在 `buddy_pmm.c` 末尾写一个占位实现：只调用一次 `assert(buddy_pmm_manager.nr_free_pages() > 0)` 并 `cprintf("buddy_check: placeholder\n")`，Task 10 会替换它。
4. 禁止修改任何已有文件（包括 `pmm.c`、`pmm.h`、`memlayout.h`、`default_pmm.c`、`best_fit_pmm.c`、`Makefile`、`tools/*`）。切换到 Buddy 的编译开关由 Leader 在 `pmm.c` 里加：`make DEFS=-DLAB2_PMM_BUDDY` 时 `init_pmm_manager()` 使用 `&buddy_pmm_manager`；默认编译仍用 Best-Fit。不要 git commit / push。
5. 完成后保存 `git -C <仓库> diff --no-index /dev/null code/kern/mm/buddy_pmm.c > <lab2 日志目录>/evidence/t9_buddy.diff`（命令返回 1 属正常）以及 `.h` 的同样 diff 到 `t9_buddy_h.diff`，然后回复 ≤12 行：数据结构设计、最大 order、如何处理非 2 的幂大小的初始区域、你最不确定的点。

输出要求（Leader 会验证）：
- `make DEFS=-DLAB2_PMM_BUDDY` 编译无警告（`-Werror`），内核打印 `memory management: buddy_pmm_manager`、`physical memory map:`、`buddy_check: placeholder`、`check_alloc_page() succeeded!`，无 panic。
- 默认 `make` 和 `make grade`（20/20）不受影响。

## RELY

均来自 2026-10-24 仓库代码：

- `kern/mm/pmm.h`：`struct pmm_manager { const char *name; void (*init)(void); void (*init_memmap)(struct Page *base, size_t n); struct Page *(*alloc_pages)(size_t n); void (*free_pages)(struct Page *base, size_t n); size_t (*nr_free_pages)(void); void (*check)(void); };`；`page2ppn`、`page2pa`、`pa2page`、`alloc_pages`/`free_pages`（经 `pmm_manager` 转发）。
- `kern/mm/memlayout.h`：`struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; }`；`PG_reserved`（bit 0）、`PG_property`（bit 1）及 `Set/Clear/PageReserved`、`Set/Clear/PageProperty`；`le2page`；`free_area_t { list_entry_t free_list; unsigned int nr_free; }`。
- `libs/list.h`：`list_init/list_add/list_add_before/list_add_after/list_del/list_empty/list_next/list_prev`。
- `kern/debug/assert.h`：`assert`、`panic`；`stdio.h`：`cprintf`。
- 调用约定（`pmm.c`）：`pmm_init()` → `init_pmm_manager()`（调用 `init()`）→ `page_init()` 调用**一次** `init_memmap(base, n)`，`base` 起的 `n` 页都是 `PG_reserved` 状态的连续页；默认 128MB 时 `n` 约为 0x7df9 页（不是 2 的幂，起点也不是 2 的大幂次对齐）→ `check_alloc_page()` 调用 `pmm_manager->check()`。
- 参考实现：`default_pmm.c`（First-Fit，`free_area` + 宏、块头 `property` 记长度、`PG_property` 标块头）、`best_fit_pmm.c`。
- 编译选项含 `-Werror -Wall -O2`，`-Wno-unused`。

## GUARANTEE

- 新增 `buddy_pmm.h`、`buddy_pmm.c`；定义 `const struct pmm_manager buddy_pmm_manager`，6 个函数指针全部有实现（`check` 指向 `buddy_check`）。
- 内部函数均为 `static`；只有 `buddy_pmm_manager` 和 `buddy_check` 对外可见。
- 不修改任何已有文件；编译无警告。

## SPECIFICATION

### 内部状态
- 每个 order（0..MAX_ORDER，MAX_ORDER 由你定，需覆盖 128MB 以内的块，并写成宏）一条空闲链表和该 order 的空闲块数；总空闲页数 `nr_free`。
- 空闲块只把块头页挂在链表上：块头 `PG_property` 置位，`property` 记录该块的 order（或页数，二选一，必须在注释里写清楚）；块内其它页 `PG_property` 清零。
- 记录管理区域的起始 `Page *` 和页数，用于计算“相对下标”和伙伴地址；伙伴不能落到管理区域之外。

### buddy_init
- Postconditions：全部 order 链表为空，`nr_free == 0`，管理区域未设置。

### buddy_init_memmap(base, n)
- Preconditions：`n > 0`；`base[0..n-1]` 都是 `PG_reserved`；本管理器只会被调用一次（如被调用第二次，`panic` 并说明原因——不要静默支持多区域）。
- Postconditions：每页 `flags`、`property` 清零，`ref = 0`；把 `[base, base+n)` 按“从低到高，每次取当前位置能放下的、相对下标对齐的最大 2 的幂块”切成若干块挂入对应 order 链表；`nr_free == n`。
- Edge cases：`n` 不是 2 的幂时，尾部小块按同一规则切分，不得丢页也不得越界。

### buddy_alloc_pages(n)
- Preconditions：`n > 0`。
- Case 1：`n > nr_free` 或需要的 order 超过 MAX_ORDER，或所有 ≥ k 的链表都为空：返回 `NULL`，状态不变。
- Case 2：找到 order j ≥ k 的块（取最小的非空 j；同一 order 取链表中地址最低的块）：从链表摘下；当 j > k 时逐级拆分，每次把高地址的一半作为 order j-1 的空闲块挂回；返回块头。返回块头 `ClearPageProperty`，并在块头记录本次分配的 order（释放时需要），`nr_free -= 2^k`。
- Requirements：分配出去的块页数是 2^k ≥ n，多出的 2^k − n 页属于内部碎片，**随整块一起释放**，在注释中说明。

### buddy_free_pages(base, n)
- Preconditions：`n > 0`；`base` 是之前 `buddy_alloc_pages(n')` 返回的块头，且 `n` 向上取整到 2 的幂后与分配时的 order 相同（不一致时 `assert` 失败）。
- Postconditions：整块（2^k 页）每页 `flags` 清零、`ref = 0`；然后循环：计算伙伴下标 `idx ^ (1 << k)`，若伙伴在区域内、是空闲块头（`PG_property`）且 order == k，就把伙伴从链表摘下、清除其块头标记，合并为 order k+1（块头取两者中下标较小者），k++；否则停止。最后把块头挂入 order k 链表，`nr_free += 2^(原 k)`。
- Edge cases：k == MAX_ORDER 时不再合并；伙伴超出区域时不合并。

### buddy_nr_free_pages
- 返回 `nr_free`。

## Agent Result

Codex 回复（2026-10-24 21:47）：新建 `buddy_pmm.h`、`buddy_pmm.c`，未改已有文件。每个 order 一条地址有序空闲链表及块数，另记管理区域与总空闲页数；`property` 保存 order；`MAX_ORDER = 15`（最大块 32768 页 = 128MiB）；初始区域按相对下标对齐，逐次切出能放下的最大块，非 2 的幂的尾部也切成小块保留；拆分、异或找伙伴、合并；释放时断言 order 匹配，内部碎片随整块归还；`buddy_check()` 为占位。证据 `evidence/t9_buddy.diff`、`t9_buddy_h.diff`。

## Verification

Leader（2026-10-24）：
- 集成：Leader 在 `pmm.c` 加编译开关——`LAB2_PMM_BUDDY` 时 `init_pmm_manager()` 用 `&buddy_pmm_manager`（并 include `buddy_pmm.h`）；`LAB2_SLUB_TEST` 时 `check_alloc_page()` 之后调用 `slub_check()`（供 Task 11 使用）。默认编译不变。
- 代码审查：拆分（高地址一半挂回）、伙伴下标 `idx ^ size`、伙伴越界判断、合并后块头取较小下标，均符合 spec。NOTE：order 0 的已分配块头 `property` 与普通页同为 0，`free` 无法识别“传入的不是块头”这种误用（spec 未要求）。
- `make DEFS=-DLAB2_PMM_BUDDY` + `make qemu`（`evidence/t9_buddy_qemu.txt`）：编译无 warning/error；输出 `memory management: buddy_pmm_manager`、`memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff].`、`buddy_check: placeholder`、`check_alloc_page() succeeded!`、`satp physical address: 0x0000000080204000`，无 panic。
- 默认 `make grade`（`evidence/t9_grade_default.txt`）：20/20。

状态：实现 **DONE**；正确性要等 Task 10 的测试。
