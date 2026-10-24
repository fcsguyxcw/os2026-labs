# Task 10：Buddy System 测试函数（功能模块二 任务三 · 第二份提示词：测试）

执行者：Codex w1:p2（Test Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Test Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`。你的沙箱无法访问 Docker，Leader 会编译运行并把真实输出反馈给你。

任务：针对 Task 09 生成的 Buddy System 分配器（`code/kern/mm/buddy_pmm.c`），设计并实现测试函数 `void buddy_check(void)`，替换文件末尾的占位实现。测试要能检验 Buddy 的核心行为和边界情况，失败时用 `assert` 让内核 panic（panic 信息里有文件和行号），全部通过时打印 `buddy_check() succeeded!`。

操作要求：
1. 只修改 `code/kern/mm/buddy_pmm.c` 中 `buddy_check` 的实现，可以在它前面添加只供测试使用的 `static` 辅助函数（例如不变量检查函数）。**不得修改** `buddy_pmm.c` 中 Task 09 写的分配器函数和 `buddy_pmm_manager`；如果你发现分配器有 bug，不要自己修，在回复里报告（附你的推理），由 Leader 决定。
2. 不改其它文件；不 git commit / push。
3. 测试运行方式：`make DEFS=-DLAB2_PMM_BUDDY` 编译后，内核把 Buddy 作为系统物理内存管理器，`page_init` 把约 0x7df9 页一次交给 `buddy_init_memmap`，然后 `check_alloc_page()` 调用 `buddy_check()`。测试就在这个真实初始状态上运行，**结束时必须把分配器恢复到开始时的状态**（每个 order 的空闲块数、`nr_free` 都和开始时相同）。
4. 完成后把 `buddy_check` 及辅助函数的 diff 保存为 `<lab2 日志目录>/evidence/t10_buddy_check.diff`，回复 ≤12 行：每组测试检验什么、为什么能发现对应的错误。

## RELY

- Task 09 的分配器（`buddy_pmm.c`）：`MAX_ORDER 15`；`free_areas[order].free_list`（按地址升序）与 `.nr_blocks`；`region_base`、`region_pages`、`nr_free`；空闲块头 `PG_property` 置位、`property` = order；已分配块头 `PG_property` 清零、`property` = 分配 order；块的对齐以 `region_base` 为基准（相对下标是 2^order 的倍数）；`buddy_alloc_pages(n)` 把 n 向上取整到 2^k，取最小的非空 order、同 order 取最低地址，拆分时高地址一半挂回；`buddy_free_pages(base, n)` 要求 n 向上取整后的 order 与分配时一致，按 `idx ^ (1 << k)` 找伙伴并逐级合并。
- 测试可以直接调用 `buddy_pmm_manager.alloc_pages/free_pages/nr_free_pages`，也可以访问本文件内的静态变量做白盒检查。
- `assert`（`kern/debug/assert.h`）、`cprintf`、`page2pa`、`le2page`、`list_next`。

## GUARANTEE

- `buddy_check()` 覆盖下面 SPECIFICATION 列出的全部行为，通过时打印 `buddy_check() succeeded!`。
- 测试结束后分配器状态与开始时完全一致。
- 不修改分配器本身；编译无警告（`-Werror`）。

## SPECIFICATION

测试至少包含以下各组（顺序可调整），每组前用 `cprintf` 打印一行组名，便于从输出定位：

1. **不变量检查函数**（每组之后都调用）：遍历每个 order 的链表——每个节点 `PageProperty` 为真、`property == order`、相对下标按 2^order 对齐、块不越出区域、链表按地址严格升序；各 order 链表长度等于 `nr_blocks`；Σ(nr_blocks × 2^order) == `nr_free`；不同空闲块互不重叠（可利用“地址升序 + 对齐”在同一 order 内检查，跨 order 检查可用简单的 O(块数²) 或标记法，自行选择，注意 128MB 下块数不多）。
2. **初始状态**：`nr_free == region_pages`（刚初始化、未分配时），并调用不变量检查。
3. **向上取整与内部碎片**：`alloc(3)` 使 `nr_free` 减少 4；`alloc(1)`、`alloc(5)`、`alloc(8)` 分别减少 1、8、8；返回的块头相对下标按对应 order 对齐；几个块互不重叠；释放后 `nr_free` 恢复。
4. **拆分**：找一个“比目标 order 大、且所有更小 order 链表都为空”的情形，或者先把较小 order 的空闲块全部借走（分配出来暂存，之后归还），然后分配一个小块，检查从大块拆出的每一级都恰好多出一个空闲块、且多出的块正是返回块的伙伴（下标 = 返回块下标 ^ 2^j）。
5. **伙伴合并**：分配两块互为伙伴的 order k 块（可由第 4 组得到），先释放一块，检查它以 order k 留在链表上、没有合并；再释放另一块，检查二者合并成 order k+1（或更高）的块，链表里不再有这两个 order k 块头。
6. **非伙伴不合并**：两个相邻但不是伙伴的同 order 块（例如相对下标 2^k 和 2^(k+1)）都空闲时不得合并。
7. **失败路径**：`alloc(nr_free + 1)` 返回 `NULL` 且状态不变；`alloc((1 << MAX_ORDER) + 1)` 返回 `NULL`；借走所有空闲块后 `alloc(1)` 返回 `NULL`。
8. **恢复**：归还所有借走的块后，各 order 的 `nr_blocks` 与 `nr_free` 和测试开始时的快照完全一致。
- 不要测试会触发 `assert` 的非法调用（例如 `n == 0`、释放时 order 不匹配），因为那会让内核 panic、测试无法继续；这些在回复中列为“未覆盖”。

## Agent Result

Codex 回复（2026-10-24 21:51）：只替换 `buddy_check` 并添加测试用 static 辅助函数；8 组测试——不变量检查（每组之后调用）、初始状态、向上取整与内部碎片、借走初始块、逐级拆分、伙伴合并、相邻非伙伴不合并、失败路径、恢复初始状态（核对初始块地址与各阶计数）；未覆盖会触发 panic 的非法调用（n=0、释放 order 不匹配）；未发现确定的分配器 bug。证据 `evidence/t10_buddy_check.diff`。

## Verification

Leader（2026-10-24）：
1. `make DEFS=-DLAB2_PMM_BUDDY` + `make qemu`（`evidence/t10_buddy_qemu.txt`）：编译无 warning；8 组依次打印，`buddy_check() succeeded!`、`check_alloc_page() succeeded!`，无 panic。
2. **变异测试**（检验测试本身是否有效，`evidence/t10_mutation.txt`）：在临时副本 `tmp/mut-20261024/` 中分别向分配器注入一个错误，测试都在对应组 panic：
   - M1 关闭伙伴合并（`while (0 && ...)`）→ “borrowing initial blocks” 后 `buddy_pmm.c:304 assertion failed: free_areas[k].nr_blocks == initial_counts[k]`；
   - M2 拆分时把低地址一半挂回、返回高地址一半 → “splitting at every level” 中 `buddy_pmm.c:325 assertion failed: first == isolated`；
   - M3 分配时 `nr_free -= n`（不按 2^k 扣减）→ “rounding and internal fragmentation” 中 `buddy_pmm.c:229 assertion failed: before >= size && nr_free == before - size`。
   说明：M2 本身仍是一种可行的伙伴算法，只是违反了 spec 约定的“高地址一半挂回”，测试按 spec 检出。
3. 默认构建不受影响（grade 见 Task 09 Verification；Task 11 后会再跑一次）。

状态：**DONE**。
