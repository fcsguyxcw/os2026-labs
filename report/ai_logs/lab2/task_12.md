# Task 12：SLUB 测试函数（功能模块二 任务四 · 第二份提示词：测试）

执行者：Codex w1:p2（Test Agent；与写实现的 w1:p3 不是同一个 Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Test Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`。你的沙箱无法访问 Docker，Leader 会编译运行并把真实输出反馈给你。

任务：针对 Task 11 生成的 SLUB 两层分配器（`code/kern/mm/slub.c`，对外接口 `slub_init`、`kmalloc`、`kfree`），设计并实现测试函数 `void slub_check(void)`，替换文件末尾的占位实现，检验不同大小内存对象的分配和释放是否正确。失败时用 `assert` 让内核 panic；全部通过时打印 `slub_check() succeeded!`。

操作要求：
1. 只修改 `code/kern/mm/slub.c` 中 `slub_check` 的实现，可在它前面添加只供测试使用的 `static` 辅助函数。**不得修改** Task 11 写的分配器代码；发现分配器 bug 只在回复里报告（附推理），不要自己修。
2. 不改其它文件；不 git commit / push。
3. 运行方式：`make DEFS=-DLAB2_SLUB_TEST` 时，`pmm_init()` 在 `check_alloc_page()` 之后调用 `slub_init(); slub_check();`，第一层是 Best-Fit（全部空闲内存约 0x7df9 页）。测试结束时必须把所有对象和页归还：`nr_free_pages()` 回到测试开始时的值，各 cache 的统计回到 0。
4. 完成后把 diff 保存为 `<lab2 日志目录>/evidence/t12_slub_check.diff`（`git diff code/kern/mm/slub.c`），回复 ≤12 行：每组测试检验什么、为什么能发现对应的错误。

## RELY

- Task 11 实现（`slub.c`，你可以直接读它的静态变量做白盒检查）：size class `8,16,32,64,128,256,512,1024,2048`（`size_classes[]`、`NR_CACHES`）；`struct slub_cache { object_size, object_offset, objects_per_slab, partial（有空闲对象的 slab 链表）, nr_slabs, nr_inuse }`；`struct slab_header { magic(SLAB_MAGIC), cache, nr_free, freelist, link }` 位于 slab 页起始处，对象从 `object_offset`（= 页头大小按对象大小向上取整）开始；空闲对象前 8 字节存下一个空闲对象地址；`kmalloc` 选最小的 ≥ size 的 class，从 partial 链表第一个 slab 的 freelist 头部取对象（LIFO），slab 用满时摘出 partial；`kfree` 由 `ROUNDDOWN(ptr, PGSIZE)` 找页头并校验 magic，slab 全空时立刻 `free_pages` 归还 1 页；大对象（> 2048）按 `ROUNDUP(size + sizeof(struct large_header), PGSIZE) / PGSIZE` 页向第一层申请，页头 `{magic(LARGE_MAGIC), npages}`，返回页头之后的地址；`live_objects` 计数全部未释放对象；`kmalloc(0)` 返回 NULL，`kfree(NULL)` 直接返回。
- 第一层接口：`nr_free_pages()`、`alloc_pages`、`free_pages`（`pmm.h`）；`PGSIZE`；`memset`（`string.h`）；`assert`、`cprintf`。

## GUARANTEE

- `slub_check()` 覆盖下面列出的全部组，通过时打印 `slub_check() succeeded!`。
- 测试结束后所有对象、页都已归还，`nr_free_pages()` 与开始时相同；不修改分配器本身；编译无警告。

## SPECIFICATION

每组前用 `cprintf` 打印组名。至少包含：

1. **边界输入**：`kmalloc(0) == NULL`；`kfree(NULL)` 不改变任何状态。
2. **size class 选择与对齐**：对 1、8、9、16、17、31、32、33、64、100、128、200、256、300、512、700、1024、1500、2048 字节各分配一次：返回非空、8 字节对齐、所在页页头 magic 正确、`cache->object_size` 恰好是“≥ size 的最小 class”、对象地址减去页起始位于 `[object_offset, PGSIZE)` 且 `(addr - 页起始 - object_offset) % object_size == 0`。
3. **数据不重叠**：保持上一组全部对象不释放，按各自申请的字节数填入互不相同的字节模式，然后逐个读回校验（能发现对象重叠、freelist 指针写进了在用对象、对象越界）。
4. **slab 扩展**：对某一 class 连续分配 `objects_per_slab + 1` 个对象：`nr_slabs` 增加 2，且最后一个对象和第一个对象不在同一页；第一个 slab 用满后不在 partial 链表上。
5. **复用（LIFO）**：释放一个对象后立刻再分配同一 class，应得到同一地址。
6. **空 slab 归还**：记录 `nr_free_pages()`，把第 4 组的对象全部释放后，该 class 的 `nr_slabs`、`nr_inuse` 回到原值，`nr_free_pages()` 恢复（证明空 slab 的页已还给第一层）。
7. **大对象**：`kmalloc(2049)`、`kmalloc(PGSIZE - 16)`、`kmalloc(PGSIZE)`、`kmalloc(3 * PGSIZE + 1)`：`nr_free_pages()` 的减少量分别等于按 RELY 公式算出的页数；整块写满模式再读回；释放后 `nr_free_pages()` 恢复。
8. **交错压力**：用一个确定性的伪随机序列（例如线性同余，固定种子）在数组里做几百次随机大小（1～3000 字节）的分配/释放交错，每个在用对象写入与其编号相关的模式，每次释放前校验模式；结束时全部释放，`live_objects == 0`，每个 cache `nr_inuse == 0 && nr_slabs == 0`，`nr_free_pages()` 回到测试开始时的值。
9. **失败路径**：`kmalloc((size_t)-1)` 返回 NULL；`kmalloc((nr_free_pages() + 1) * PGSIZE)` 返回 NULL；两者都不改变 `nr_free_pages()` 和 `live_objects`。
- 不要测试会 panic 的非法调用（重复释放、非对象地址），在回复里列为“未覆盖”。

## Agent Result

Codex 回复（2026-10-24 22:01）：只替换 `slub_check` 并添加测试用 static 辅助函数；9 组——边界输入、size class 选择与对齐、对象数据互不重叠、slab 扩展与用满摘链、LIFO 复用、空 slab 归还页、大对象页数核算与全量读写、固定种子 512 次交错压力（释放前校验数据，结束后统计和页数全部恢复）、失败路径（整数溢出、内存不足返回 NULL 且计数不变）；未覆盖会 panic 的重复释放、非对象地址；未发现确定的分配器 bug。证据 `evidence/t12_slub_check.diff`（文件未跟踪，保存的是本次修改前后差异）。

## Verification

Leader（2026-10-24）：
1. `make DEFS=-DLAB2_SLUB_TEST` + `make qemu`（`evidence/t12_slub_qemu.txt`）：编译无警告；9 组依次打印，`slub_check() succeeded!`，无 panic。
2. **变异测试**（`evidence/t12_mutation.txt`，副本在 `tmp/mut-20261024/S1～S3`）：
   - S1 空 slab 不归还页（注释掉 `free_pages`）→ 第 3 组之后 `slub.c:223 assertion failed: nr_free_pages() == expected_pages`；
   - S2 size class 选择错一级（`size > object_size` 改为 `>=`，恰好等于某级时落到下一级）→ 第 2 组 `slub.c:265 assertion failed: slab->cache == cache && cache->object_size == size_classes[class_idx]`；
   - S3 slab 内对象步长减半（对象重叠）→ 第 2 组 `slub.c:255 assertion failed: ptr != NULL && ((uintptr_t)ptr & 7) == 0`。说明：S3 是被 8 字节对齐断言先检出的（8 字节级的步长变成 4），并非由第 3 组“数据不重叠”检出；对 ≥16 字节的级别，重叠是否会被第 3 组检出未单独验证。

状态：**DONE**。
