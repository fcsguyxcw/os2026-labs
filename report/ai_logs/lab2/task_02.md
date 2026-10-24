# Task 02：实现 Best-Fit 物理内存管理器（功能模块二 任务二）

执行者：Codex（Implementation + Test Agent，Herdr 面板 w1:p3）
Leader 发出时间：2026-10-24（北京时间）

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

## Agent Result

Codex 回复（2026-10-24 21:17）摘要：
- 只改 `best_fit_pmm.c`，注释改为 `//lab2 function2-2 2412172`。
- 实现：`free_area` + `free_list` / `nr_free` 两个宏；`best_fit_insert` 按地址升序插入块头；`alloc` 完整遍历链表、取 `property >= n` 中最小的块（同长度保留先遇到的低地址块），切分后剩余块插回原位置；`free` 插入后分别与前、后相邻块合并；`best_fit_pmm_manager` 定义在注释位置，通过前向声明 `static void best_fit_check(void);` 指向测试函数。
- 两个测试函数经字节比较未改动；`git diff --check` 通过。
- **偏差**：与 Task 01 相同，Docker 返回退出码 1（无权访问 docker.sock），第一步编译未执行；第二步未执行。证据：`evidence/t2_compile.txt`、`evidence/t2_best_fit.diff`、`evidence/t2_self_check.txt`。

## Verification

Leader 复核与整合（2026-10-24）：
1. 代码审查：`alloc` 完整遍历后才选块（符合 Best-Fit，不是 First-Fit）；分配失败时链表和 `nr_free` 不变；切分后剩余块头置 `PG_property`、插回原位置保持地址升序；`free` 合并前后块时清除被吸收头页的 `PG_property` 和 `property`。未发现违反 spec 的问题。
2. Leader 在仓库 `pmm.c` 的 `init_pmm_manager()` 把 `pmm_manager = &default_pmm_manager;` 改为 `&best_fit_pmm_manager;`（唯一一行 Leader 直接改动的代码）。
3. 在 Docker 镜像 `os2026-dbg` 中运行（Leader 执行）：
   - `make && timeout 10 make qemu` → `evidence/t3_qemu.txt`：
     ```
     memory management: best_fit_pmm_manager
     physical memory map:
       memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff].
     check_alloc_page() succeeded!
     satp virtual address: 0xffffffffc0205000
     satp physical address: 0x0000000080205000
     ```
     无 panic；退出码 124 为 timeout 正常终止。
   - `make grade` → `evidence/t3_grade.txt`：`check page init: OK`、`check best fit: OK`、`Total Score: 20/20`。
4. 与 `lab2.zip` 原版逐文件比对（`diff -rq`，排除 `bin/`、`obj/`）：只有 `kern/mm/pmm.c`、`kern/mm/best_fit_pmm.c` 有差异；`basic_check`、`best_fit_check` 函数体与原版逐字一致。

状态：**DONE**（交叉 review 见 Task 03）。

## 大白话小结

物理内存分配器管着一张“空闲块清单”（`free_list`，按地址排序），每个空闲块只在第一页上记“这块有几页”（`property`）并打上 `PG_property` 标记。First-Fit 是从头找、找到第一个够大的就用；Best-Fit 要把清单从头看到尾，挑“够用里面最小的”那一块，这样大块能留给以后的大请求。分配时如果块比需要的大，就把前面切下来给出去，剩下的部分还留在清单里原来的位置；释放时把页放回清单，再看前后邻居是不是紧挨着，紧挨着就合并成一块，避免内存被切成越来越碎的小块。内核通过 `pmm_manager` 这个“函数指针表”调用分配器，所以只要把指针从 `default_pmm_manager` 换成 `best_fit_pmm_manager`，上层代码一行都不用改——这就是“机制与策略分离”。
