# Task 04：修复 page_init 描述符数组越界（Review 发现）

执行者：Codex w1:p2（Debug Agent）
Leader 发出时间：2026-10-24（北京时间）

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

## Agent Result

Codex 回复（2026-10-24 21:26）：在写入描述符前计算 `descriptors_end = PADDR(&pages[npage - nbase])`，`descriptors_end > mem_end` 时 panic 并输出两个地址；数组放得下但没有完整空闲页时保留原有的 reserved 标记和原因提示；`page_init()` 以外未改动，`git diff --check` 通过。证据：`evidence/t4_pmm.diff`。

## Verification

Leader 在 Docker 中验证（2026-10-24）：
- 代码审查：检查位于 `SetPageReserved` 循环之前，`freemem` 复用 `descriptors_end`，符合新 Case 5 / 修订后的 Case 3。
- 默认 128MB：`evidence/t4_qemu.txt`，编译无 warning，输出 `best_fit_pmm_manager`、`physical memory map:`、`check_alloc_page() succeeded!`、两行 satp，无 panic。
- `make grade`：`evidence/t4_grade.txt`，`Total Score: 20/20`。
- 尝试复现越界分支：直接运行 `qemu-system-riscv64 ... -m 2080K`（不改 Makefile），输出只有 QEMU 被 timeout 终止的一行，连 OpenSBI 横幅都没有（`evidence/t4_qemu_m2080K.txt`）。原因（推测，与 10-24 日志中 `DTB Address: 0x82200000` 一致）：OpenSBI 0.4 把设备树放在 DRAM 起点 +34MB 处，内存小于约 34MB 时固件自身无法启动，而这么大的内存里描述符数组总能放下。所以 **Case 5 分支无法在 QEMU 中运行时复现，只经过静态审查（UNVERIFIED）**。
- 补充：`-m 64M` 运行（`evidence/t4_qemu_m64M.txt`），设备树报告 `0x4000000`（64MB），`page_init` 打印 `[0x0000000080000000, 0x0000000083ffffff]` 并通过 `check_alloc_page()`，说明实现按设备树结果工作，不依赖写死的 128MB。

状态：修复 **DONE**（默认路径编译、运行、评分通过）；Case 5 运行时 **UNVERIFIED**（环境无法构造）。
