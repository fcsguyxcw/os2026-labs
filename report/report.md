# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 2：物理内存和页表 |
| **小组成员** | 2412172-许城玮；单人小组 |
| **完成日期** | 2026-10-24（进行中） |

### 小组分工

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2412172-许城玮 | 单人完成全部练习 |

---

## 一、实验目的

1. 理解页表的建立和使用方法。
2. 理解物理内存的管理方法。
3. 理解页面分配算法。

---

## 二、实验环境

| 角色 | AI 编程工具 | 底层模型 | 备注 |
|------|------------|---------|------|
| Leader | Claude Code | Claude Opus 5.5 | 任务拆分、整合与 Docker 验证 |
| 子 Agent | Codex CLI | GPT-6.1-Sol medium | Herdr 面板 w1:p2、w1:p3；实现、审查与修复 |

软件环境：macOS 上的 OrbStack Docker，实际使用镜像 `os2026-dbg`。镜像内使用 RISC-V GCC 13.2.0、QEMU 4.1.1 和 gdb-multiarch。QEMU 4.1.1 以 `--enable-debug` 编译，同一个可执行文件既用于运行内核，也用于分支任务调试。仓库分支为 `lab2`，代码位于 `code/`。工具与环境信息依据 [启动输出 t3_qemu.txt](ai_logs/lab2/evidence/t3_qemu.txt) 和 [环境配置状态.md](../../环境配置状态.md) 的 Mac 环境记录；启动输出确认 OpenSBI 与内核运行情况，工具版本和调试版构建位置由环境配置记录提供。这里不沿用模板中未经此次日志确认的 Ubuntu 和 gdb 具体版本。

固件是内核启动前运行的底层软件。实际启动输出为 `OpenSBI v0.4 (Jul  2 2019 11:53:53)`，另有 `Runtime SBI Version : 0.1`；前者是固件版本，后者是其运行时接口版本，二者不能混为一谈。原始输出见 [t3_qemu.txt](ai_logs/lab2/evidence/t3_qemu.txt)。

**环境调研结论**（来源为上述环境配置记录）：`entry.S` 的启动页表前 511 项为零，仅最后一项映射高地址，没有低地址恒等映射，即没有把低虚拟地址映射回同一物理地址。环境记录报告 QEMU 8.2.2 下在开启分页后的 `0x80200038` 发生 `exec_page_fault`；本报告没有重新复现。QEMU 4.1.1 的正常运行有 [t3_qemu.txt](ai_logs/lab2/evidence/t3_qemu.txt) 支持。分支任务第 5 问进一步结合 Task 07 实测说明：4.1.1 此时仍命中分页前留下的软件 TLB 表项；“8.2.2 因更积极刷新而失败”仍是**推测，未核对源码**。

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

本实验先解决“哪些物理页能用”，再解决“请求到来时选择哪一块”。物理页是按固定大小划分的物理内存，本实验每页为 4096 字节。`struct Page` 是记录一页状态的描述符，相当于该页的登记卡，登记卡本身也要占内存。

指导书给出的实现链是 `pmm_init() -> init_pmm_manager() -> page_init() -> init_memmap() -> alloc_pages()/free_pages()`。这是功能上的依赖顺序：先选择分配器，再登记可用页，之后才能分配和回收。源码中的 `init_memmap()` 由 `page_init()` 内部调用，分配与释放由上层接口或自检发起，并非这些函数在同一层依次调用。

设备树是一份描述硬件布局的数据。`dtb_init()` 先于 `pmm_init()` 解析其中的内存起点和大小。`page_init()` 排除 OpenSBI、内核镜像和描述符数组，把其后的完整页面交给分配器。分配器管理空闲连续块，并维护空闲页数。Buddy、SLUB 与分支任务见后文（进行中）。

### 3.2 功能的逐步实现

1. `init_pmm_manager()` 选择当前物理内存管理器，并调用其 `init()` 清空内部状态。当前源码选择的是 `best_fit_pmm_manager`。
2. `page_init()` 校验设备树信息，设置虚实地址偏移和页号上界，规划描述符存放位置；确认数组能放下后，先把全部页面标为保留。
3. `init_memmap()` 将剩余完整页面交给当前管理器。管理器把这些页改为空闲状态，建立按地址排列的空闲块链表。
4. 先阅读 First-Fit（首次适应：选择第一个够大的块），再实现 Best-Fit（最佳适应：选择够用的最小块）。两者通过同一个 `pmm_manager` 函数指针表接入；函数指针表记录了分配、回收等操作的具体实现。
5. `check_alloc_page()` 调用当前管理器的自检。源码中的测试会实际分配和释放页面，检查计数、标记和链表是否能恢复。Buddy、SLUB 与分支任务见后文（进行中）。

以上顺序依据指导书 `lab2_2_1_exercise.md` 及 `pmm.c`，具体迭代见下文和 [AI 协作总览](ai_logs/lab2/overview.md)。

---

## 四、实验内容与实现

### 功能模块一：物理内存初始化

#### 任务一：`page_init()`（`kern/mm/pmm.c`）

**功能与核心步骤。** `page_init()` 把设备树中的物理内存范围变为可供分配器管理的页面。它没有参数，也不返回地址；结果保存在 `va_pa_offset`、`npage`、`pages` 和管理器状态中。

`end` 是内核镜像结尾的虚拟地址，`pages` 也保存虚拟地址。虚拟地址是内核指令使用的地址，物理地址是内存硬件对应的地址；此处通过 `PADDR` 减去 `va_pa_offset` 转换。内存上界取设备树末端与 `PHYSICAL_MEMORY_END` 的较小值，再向下按页对齐。`npage` 是这个上界的物理页号，不是数组长度；数组长度为 `npage - nbase`，`pages[0]` 对应 `DRAM_BASE`。

描述符数组起点取 `ROUNDUP(end, PGSIZE)`。当前修复后的实现先计算数组物理末端 `PADDR(&pages[npage - nbase])`，确认没有超过可管理上界，再把所有描述符置 `PG_reserved`。`PG_reserved` 表示该页暂不能由分配器使用。数组末端向上按页对齐得到 `freemem`。若 `freemem` 小于内存上界，就只调用一次 `init_memmap()`，传入该连续空闲区；否则输出没有完整空闲页的原因。这样 OpenSBI、内核和描述符自身占用的页面都不会被分出去。

**四段式规格与提示词。** spec 是可逐项核对的行为约定。Task 01 的 `PROMPT` 说明目标、唯一允许的修改位置和输出证据；`RELY` 列出可依赖的源码接口、常量与地址语义；`GUARANTEE` 列出实现必须保证的初始化状态、保留区与测试要求；`SPECIFICATION` 用前置条件、后置条件、边界分支细化算法。这样分开可以区分“框架已提供什么”和“本次必须实现什么”，避免把页号上界当成描述符个数。

最终发送的原文见 [prompt.md：Task 01](prompt.md#task01)，修复提示词见 [Task 04](prompt.md#task04)，交叉审查提示词见 [Task 03](prompt.md#task03)。正文只解释设计，不重复提示词全文。Task 04 是在原规格上新增 Case 5，并修订 Case 3，不回写 Task 01 的历史提示词。

**实现迭代。** Task 01 在指定位置实现初始化和原有四类边界：零值或溢出时终止、超出管理上限时截断、无完整空闲页时提示、内存起点不是 `DRAM_BASE` 时终止。子 Agent 的 Docker 命令因 socket 权限失败，编译并未开始；随后由 Leader 整合 Best-Fit 并运行，Task 03 前的默认路径获得 20/20。

Task 03 的交叉审查仍判定 page_init 为 `NEEDS_FIX`：代码先写描述符，再判断空闲页是否足够，遗漏了“连描述符数组本身都放不下”的情况。Task 04 增加写入前的严格大于检查；越界时 `panic`，即明确终止内核，并输出数组物理末端和内存上界。数组恰好放得下但没有完整空闲页时，仍先保留全部描述符，再提示且不调用 `init_memmap()`。修复后默认运行、64MB 补测和 grade 证据见第五节；越界分支运行时仍未验证。

过程依据：[Task 01](ai_logs/lab2/task_01.md)、[Task 03](ai_logs/lab2/task_03.md)、[page_init 审查](ai_logs/lab2/evidence/t3_review_page_init.md)、[Task 04](ai_logs/lab2/task_04.md)、[bugs.md](ai_logs/lab2/bugs.md)。另据 Task 01 的复核记录，数组在 BSS（启动时清零的静态数据区）之外，目前只置保留位，并未主动清零所有保留描述符字段；空闲页字段由管理器重置。若扩展到非零初始 RAM 环境，应再明确保留描述符其余字段的初始化约定，不能由当前通过结果推断所有环境都相同。

#### Challenge：硬件可用物理内存范围的获取方法

一种方法是读取固件或引导程序提供的内存描述，而不是把整个物理地址空间都当作 RAM。x86 的 BIOS 可通过 `int 15h` 的 E820 服务提供内存区间及类型；UEFI 可提供 memory map（内存映射表），说明哪些区间可用、哪些仍被固件占用。设备树可描述 RAM 区间和保留区；ACPI 是固件提供的硬件配置表体系，也可提供内存拓扑等信息，通常需与平台内存映射及保留信息结合使用。这些接口的共同点是传递区间和用途，OS 还需扣除固件、内核及管理数据占用，才能确定真正可分配的页。这里是在回答获取方法，没有在本实验中调用 BIOS、UEFI 或 ACPI。

另一种方法是 OS 自己探测。在已确认适合探测的候选 RAM 中，保存原值，逐页选择位置，交替写入 `0x55`、`0xAA` 等特征值并读回比较，最后恢复原值；还可检查不同地址是否相互覆盖。这只是探测思路，不能直接对未知地址任意写入。MMIO（映射到内存地址空间的设备寄存器）写入可能触发设备操作；内存空洞可能引发访问异常；缓存可能让读回只反映缓存内容；地址别名可能让两个地址实际指向同一存储位置；探测还可能破坏固件、内核或已有数据。即使恢复原值，也不能撤销设备寄存器的副作用，所以该方法需要平台知识和异常处理。

本实验使用第一种方法中的设备树。在 RISC-V + OpenSBI 启动链中，`a1` 传入 DTB（设备树的二进制形式）的物理地址，`entry.S` 把它保存到 `boot_dtb`。`dtb_init()` 转换地址、校验格式并解析内存信息，随后 `page_init()` 通过 `get_memory_base()`、`get_memory_size()` 读取结果。默认日志报告起点 `0x80000000`、大小 128MB，64MB 补测则报告 64MB；没有通过写特征值来发现内存。依据为 `entry.S`、`kern/driver/dtb.c`、指导书“物理内存探测”及第五节运行证据。

### 功能模块二：物理内存分配

#### 任务一：理解 First-Fit

`default_pmm.c` 用 `free_area` 保存双向空闲链表和 `nr_free` 空闲页数。空闲块只把第一页作为链表节点；块头的 `property` 记录连续页数，`PG_property` 标记它是空闲块头。因此链表节点数与空闲页数不同。

`default_init()` 建立空链表，并把计数设为零。`default_init_memmap(base, n)` 接收一段此前保留的连续页，逐页检查 `PageReserved`，清除 `flags`、`property`，把引用计数 `ref` 设为零；再把第一页设为长度为 n 的块头，按地址升序插入链表，增加空闲页数。

`default_alloc_pages(n)` 先要求 n 大于零；总页数不足时返回 `NULL`。之后从链表头开始，遇到第一个长度至少为 n 的块就停止。选中后摘下块头；块较大时，把前 n 页交给调用者，将余下部分的头页设为新的空闲块头，放回原位置。最后减少 `nr_free` 并清除已分配块头的 `PG_property`。若总页数够但没有足够大的连续块，也返回 `NULL`。

`default_free_pages(base, n)` 检查这些页不是保留页或空闲块头，清除状态及引用计数，建立新块头并增加空闲页数，再按地址插入。先看前一个块的结尾是否正好等于 base，相邻就合并；若已与前块合并，base 更新为前块的头。再检查更新后的块尾是否紧挨后块，若相邻继续合并。被吸收的头页清除 `PG_property` 并摘链。地址有序让它只需检查直接邻居，不必查找所有块。

**改进空间。** 设空闲块数为 k，First-Fit 最坏需要扫描 O(k) 个节点；反复从低地址开始也容易让小碎片聚集在那里。外部碎片指空闲页总数足够，却因分散而没有所需连续块。Next-Fit（下次适应）可从上次停止处继续寻找，代价是维护游标且不保证碎片更少。按大小分级链表可缩小搜索范围，但需维护多个类别；平衡树可组织块大小或地址索引，但插入、删除和相邻合并更复杂。Buddy 按二的幂组织块，便于拆分合并，但请求向上取整会产生内部浪费。这些是可考虑的方案，本次没有据此改动原 First-Fit。

#### 任务二：Best-Fit（`kern/mm/best_fit_pmm.c`）

**spec 与提示词设计。** Task 02 的 `PROMPT` 明确“在够用的块中选最小，同长度选最低地址”，并限制只能在 Best-Fit 文件的指定位置实现，两个测试函数不可改；`RELY` 列出 `pmm_manager`、链表接口、页标志及测试直接读写的 `free_list`、`nr_free` 约定；`GUARANTEE` 要求六个回调注册完整、测试原样保留并通过验证；`SPECIFICATION` 分别写初始化、导入空闲区、分配、释放和计数查询的前后条件，要求分配失败不改变状态、分配前完整遍历、释放时前后合并。这种拆法把选择策略和链表维护分开，能逐项检查失败分支是否也保持计数一致。最终原文见 [prompt.md：Task 02](prompt.md#task02)。

**实现。** `best_fit_insert()` 按地址插入，`best_fit_alloc_pages()` 完整遍历链表，只在发现更小的可用块时替换候选；同长度保留先遇到的低地址块。拆分后余块放回原位置，已分配头的 `property` 清零、`PG_property` 清除。释放后先合并前块再合并后块，被吸收头的 `property` 和 `PG_property` 都清除。`nr_free` 只按分配或释放的页数变化，合并不再次增加计数。管理器通过前向声明把 `.check` 指向原有 `best_fit_check()`。

**迭代与切换。** Task 02 完成实现和静态自查，但子 Agent 首次 Docker 编译因权限失败，没有实际开始。随后 Leader 在 `init_pmm_manager()` 将 `pmm_manager = &default_pmm_manager;` 切换为 `pmm_manager = &best_fit_pmm_manager;`，再在 Docker 中整合运行。只实现文件而不切换，内核仍会调用 First-Fit。Task 03 的 Best-Fit 交叉审查结论为 `PASS`，两个原有测试函数与压缩包骨架逐字节一致；整合日志为 20/20。依据：[Task 02](ai_logs/lab2/task_02.md)、[Task 03 审查](ai_logs/lab2/evidence/t3_review_best_fit.md)。

**改进空间。** 当前每次分配都完整扫描 k 个空闲块，时间为 O(k)。挑最小的够用块仍可能留下很小的余块，不能消除外部碎片。可以按“块长度、地址”建立有序树，直接定位够用的最小块；代价是要同时维护地址邻接关系以便合并，并增加索引维护成本。按大小分级也能减少搜索，但同类内部仍需选择，可能牺牲严格最佳适应。

Task 03 的 NOTE 还指出测试缺口：两个原有自检未显式独立验证同长度取低地址、三个入口的 n=0 断言、总页数足够但最大块不足，以及前后两侧同时合并。静态检查这些分支符合 spec，但 20/20 不等于它们都被动态覆盖。后续可逐项构造边界测试，检查失败后状态不变、合并后的标志清理与总计数；本次未新增这些测试。

#### 任务三：Buddy System

TODO：实现提示词与测试提示词（原文见 `prompt.md`）；测试覆盖了哪些核心行为和边界情况，为什么能有效检验实现。

#### 任务四：SLUB

TODO：实现提示词与测试提示词（原文见 `prompt.md`）；测试覆盖了哪些核心行为和边界情况，为什么能有效检验实现。

### 分支任务：gdb 调试页表查询过程

#### 调试环境

客户机是运行中的 ucore；宿主是运行 QEMU 的容器。TCG 是 QEMU 把客户机指令转换为宿主代码的翻译器。TB（翻译块）是一段已翻译的客户机指令对应的宿主代码。TLB 是缓存地址映射和权限的快表；PTE 是页表项。load/store 分别表示读/写内存，fill 表示补入缓存，tag 是用于匹配地址的标记。

三个终端分别控制 QEMU 启动、QEMU 源码调试、ucore 调试。宿主课程目录下先运行 `./os2026.sh debug`，QEMU 用 `-s -S` 等待连接；再运行 `./os2026.sh gdb-qemu`，附加 QEMU 进程、设断点并 `continue`；最后运行 `./os2026.sh gdb`，通过 `localhost:1234` 控制 ucore。这两个 gdb 调试的对象不同，源码单步在终端 2，客户机寄存器与指令在终端 3。入口依据 `../os2026.sh:21-28`，remote 命令依据 `code/Makefile:179-182`。QEMU 宿主停住时，终端 3 暂时等待，应先操作终端 2。

使用镜像 `os2026-dbg` 中已用 `--enable-debug` 编译的 QEMU 4.1.1，源码在容器 `/opt/qemu-src/qemu-4.1.1`。本环境不需要再做指导书中的重新编译、改 Makefile 的 QEMU 路径和 `sudo gdb`；入口脚本已给容器 `SYS_PTRACE` 能力（`os2026.sh:13`）。这些环境说明见 [Task 07](ai_logs/lab2/task_07.md) 和环境配置记录。

以下实测数据由 Leader 在同一容器中运行 [t7_scripts/](ai_logs/lab2/evidence/t7_scripts/) 的批处理脚本取得。`run.sh` 自动启动 QEMU、host gdb 和 RISC-V gdb；交互式演示使用相同断点、单步与打印命令。首次链路见 [t7_tracer_qemu_gdb.txt](ai_logs/lab2/evidence/t7_tracer_qemu_gdb.txt)，完整两轮见 [paged 日志](ai_logs/lab2/evidence/t7_paged_qemu_gdb.txt)、[Bare 日志](ai_logs/lab2/evidence/t7_bare_qemu_gdb.txt)。本报告整理时没有重新运行调试。可照做的交互稿见 [分支任务演示步骤.md](ai_logs/lab2/分支任务演示步骤.md)。下文 QEMU 源码行号均相对 `/opt/qemu-src/qemu-4.1.1`，已与本地只读副本核对。ucore 的 `pmm.c` 行号按 Task 07 取证时版本引用；后续 Buddy/SLUB 扩展可能使行号移动，可按 `SetPageReserved` 等语句定位。

#### 第 1 问：关键调用路径和分支

本次数据读取的慢路径一行版：`code_gen_buffer → helper_le_ldq_mmu → load_helper → tlb_fill → riscv_cpu_tlb_fill → get_physical_address → tlb_set_page_with_attrs → 返回 load_helper → ldq_le_p(haddr)`。前半段是调用栈；装表、返回和读取是随后发生的步骤，并非所有函数同一时刻嵌套。

`helper_le_ldq_mmu` 接收一次小端 8 字节读取（`accel/tcg/cputlb.c:1446-1450`）。`load_helper` 检查软件 TLB；主表未命中则查 victim TLB，即保存被主表挤出项的小表；两者都未命中才 fill（`:1271-1278`）。`tlb_fill` 分发到 CPU 的架构回调（`:868-879`），RISC-V 回调计算物理地址并检查物理内存保护 PMP（`target/riscv/cpu_helper.c:451-467`）。成功后装表（`:471-474`），返回 helper 用 `addr+addend` 得到宿主指针并读取 RAM（`cputlb.c:1338,1361`）。装表时通过物理地址空间找到承载该页的内存区域，取得宿主 RAM 指针（`cputlb.c:725-748,804`；`exec.c:722-770`）。`addend` 是宿主地址偏移，不是客户机 PA 减 VA。

第一轮 [A]～[F] 的短引用如下，省略标签解释和非关键行，各行来自同一轮日志：

```text
[A] addr=0xffffffffc0207008 satp=0x8000000000080205 priv=1
[B] base(根页表物理地址)=0x80205000 idx=511 pte_addr=0x80205ff8 pte=0x200000cf
[C] *physical(4KB 物理页基址)=0x80207000  完整物理地址=0x80207008
[D] Value returned is $1 = 0
[E] vaddr=0xffffffffc0207000 paddr=0x80207000 prot=7 mmu_idx=1 size=0x1000
[F] addr_read(tag)=0xffffffffc0207000 addend=0xffffafe00000
haddr=0xffff70007008  *(uint64_t*)haddr=0x0
```

这里读的是描述符数组第一个元素的 flags，即 `pages[0].flags`。`code/kern/mm/pmm.c:97-98` 调用 `SetPageReserved`，宏以读后改写的方式置位（`code/kern/mm/memlayout.h:45`）。Task 07 选取的是第一次读，`access_type=0`、`size=8`，不是取指。数组起点是 `0xffffffffc0207000`；不要把它和保存数组指针的全局变量 `pages` 自身地址混淆。

最后的加法按64位地址回绕：`0xffffffffc0207008 + 0xffffafe00000 = 0xffff70007008`。前者是客户机 VA，中间算出的 `0x80207008` 是客户机 PA，后者才是 QEMU 进程中的宿主指针。宿主指针和 addend 会随运行变化，不能当作固定地址背诵。[D] 日志同时显示 `ret=1`，但当时停在 `cpu_helper.c:451` 的赋值前；函数返回值已经是0，应以返回值为准，再单步后看 ret。

关键分支还包括以下几类。这些分支位置来自源码；除本次正常路径和第二轮 Bare 路径外，没有相应异常的动态样本，不能说都已实测覆盖。

- 有效 M 模式或无 MMU 功能直接返回原地址；MPRV 可先改变数据访问的有效模式。Bare 也直接返回原地址（`cpu_helper.c:163-174,196-199`）。
- Sv39 先检查虚拟地址高位符号扩展（`:223-229`）。V=0 失败；R/W/X 全0继续下级；W=1而R=0的保留组合失败（`:256-267`）。叶子还要检查 U/S 权限、SUM/MXR、访问类型和大页对齐（`:268-288`）。
- A 表示页被访问，D 表示页被写脏。需要时置 A，store 还置 D；RAM 内用原子比较交换更新，冲突重新尝试，不能更新则失败（`:291-327`）。本次 A/D 已为1，没有观察到更新。
- 普通 fill 失败时设置异常并退出当前 CPU 执行循环；probe 探测模式失败只返回 false（`:475-479`；`target/riscv/op_helper.c:28-34`）。TLB 命中会绕过遍历；victim 命中交换缓存项（`cputlb.c:994-1007`）。跨页、未对齐与设备内存另有分支（`:1264-1268,1282-1334`），本次选择的是普通对齐 RAM load。

#### 第 2 问：用实测数值单步解释 Sv39

Sv39 使用39位虚拟地址，并以三个9位索引查页表。这里按源码的循环编号解释：i=0 是根层，对应通常所说的 L2；不能把 i=0 误认为4KB叶子所在的最底层。

1. [A] 的 `satp=0x8000000000080205`：MODE=8 为 Sv39，根物理页号 PPN=`0x80205`，左移12位得到根表物理地址 `0x80205000`。源码为 `cpu_helper.c:184-191`，设置 `levels=3, ptidxbits=9, ptesize=8`。
2. 初始 `ptshift=18`。对 VA `0xffffffffc0207008`，`idx=(VA>>(12+18))&0x1ff=511`。PTE 物理地址是 `base+idx*8=0x80205000+0xff8=0x80205ff8`（`:231-242`）。
3. RV64 用 `ldq_phys(cs->as,pte_addr)` 读取8字节 PTE（`:252`）。它读的是客户机物理内存，不是拿 pte_addr 再查一次客户机虚拟页表；包装函数见 `include/exec/memory_ldst_phys.inc.h:29-32`。本地副本缺少其最终实例化的 `memory_ldst.inc.c`，内部实现不作进一步无依据展开。
4. [B] 的 `pte=0x200000cf`，右移10位得 `ppn=0x80000`；V/R/W/X/A/D=1，U=0。R/X 已置位，属于叶子，且 ppn 低18位为0，满足1GB映射对齐（`:254-278`）。权限检查允许本次 S 态 load。
5. [C] 中 `vpn=VA>>12=0xffffffffc0207`。源码 `:333-334` 的计算为 `((0x80000 | (vpn & 0x3ffff)) << 12)`；低18位 VPN=`0x207`，于是物理页基址=`0x80207000`。加页内偏移 `VA&0xfff=8`，完整 PA=`0x80207008`。

对应的遍历现场：

```text
levels=3 ptidxbits=9 ptesize=8 i=0 ptshift=18
base(根页表物理地址)=0x80205000 idx=511 pte_addr=0x80205ff8 pte=0x200000cf
ppn=0x80000  V=1 R=1 W=1 X=1 U=0 A=1 D=1
```

本实验启动页表只有最后一个根项有映射（`code/kern/init/entry.S:59-63`），该项是1GB大页，根层已经是叶子，因此实测只读了一层。如果采用4KB页，前两层应是 R/W/X 全0的有效非叶子项，源码 `cpu_helper.c:259-261` 把 `base` 改成下一层表物理地址，循环 `:237` 把 ptshift 从18减到9、再到0，第三层才得到4KB叶子。这个三层过程是源码解释，没有三层实测数据。

#### 第 3 问：TLB 的 C 实现、组织与查找

C 查找入口分别是 `load_helper` 的 `accel/tcg/cputlb.c:1271` 和 `store_helper` 的 `:1519`。`tlb_hit` 定义在 `include/exec/cpu-all.h:359-361`，实际比较页地址并保留 invalid 位（`:348-350`）。`victim_tlb_hit` 在 `cputlb.c:977-1010`。两个 helper 都强制内联（`:1247,1500`），所以源码行断点可能展开成多个位置，日志里 load 行有14个位置、store行有7个位置；需加地址条件。

`CPUTLBEntry` 存读、写、取指三种 tag：`addr_read/addr_write/addr_code`，再存 `addend`（`include/exec/cpu-defs.h:110-129`）。tag 是虚拟页标记及低位状态；没有相应访问权限时可设为全1（`cputlb.c:805-817`），不能把它解释为客户机物理页号。`mmu_idx` 选择缓存组，RISC-V 这里返回 `env->priv`，本次为1，即 S 态（`cpu_helper.c:27-33`）；它不是 satp MODE，也不是地址空间标识 ASID。主表索引为 `(addr>>12)&(mask>>CPU_TLB_ENTRY_BITS)`（`include/exec/cpu_ldst.h:163-180`）。表大小可动态变化；victim 每组8项，全相联扫描；冲突淘汰使用轮转位置（`cpu-defs.h:83-107,170-188`；`cputlb.c:777-786`）。

第二轮 [H] 实测的是分页前的 **store**，不可当作 load 的输出：

```text
addr=0x80206000 mmu_idx=1 index=6 tlb_addr(addr_write)=0xffffffffffffffff 页=0x80206000 -> 未命中
satp=0x0 priv=1
1520        if (!victim_tlb_hit(env, mmu_idx, index, tlb_off,
```

下一行检查 victim；随后 [I] 进入 `get_physical_address`，说明本次 victim 也未命中，最终走 fill。日志没有展示 victim 命中的交换现场，交换行为仅由源码说明。

[G] 则是缓存命中的对比。[F] 后设定的同页 `cputlb.c:1271` 和 `get_physical_address` 断点没有再命中，日志以 detach 结束，**没有一段现场打印“TLB 命中”的 [G] 输出**。Task 07 据此结合生成代码的快速表结构（`cpu-defs.h:133-136,179-188`）解释：后续同页访问直接在生成的宿主代码中命中快表，没有进入 C helper；不能把“C 断点没停”误认为没有访存。该结论依据为 Task 07 记录与源码，未记录快路径逐条宿主汇编单步。

#### 第 4 问：软件 TLB 与真实 CPU 的逻辑区别

教材/规范层面，真实 CPU 的地址翻译 TLB 缓存“虚拟页到物理页及权限”，Bare 模式不执行分页地址翻译。这里没有对真实 CPU 的微架构做实测，也不推断其内部一定不存在任何其他缓存。

QEMU 软件 TLB 则为模拟器执行服务：RAM 快路径用虚拟页 tag 和 addend 直接得到宿主内存指针，同时保留访问类型及设备内存分发信息（`cpu-defs.h:110-150`；`cputlb.c:804,1338`）。即使客户机不做分页，它仍需把客户机物理内存定位到宿主内存，所以 Bare 也可以 miss、fill、装表。不是说每次 Bare 访问都查页表。

第二轮 [I][J] 是直接证据：

```text
addr=0x80206000 access_type=1 mmu_idx=1 satp=0x0 priv_ver=0x11100
mode=1 vm=0
*physical=0x80206000（等于 addr，没有查页表）
vaddr=0x80206000 paddr=0x80206000 prot=7 mmu_idx=1
```

实测停在 `case VM_1_10_MBARE`（`cpu_helper.c:196-199`），直接 `*physical=addr`，之后仍调用 `tlb_set_page`（`:471-473`）。有效 M 模式直接返回是另一源码分支（`:171-174`），本次 `priv=1` 不能写成 M 模式实测。

QEMU 选择按 mmu_idx 分组、动态主表与8项 victim，这是软件实现策略，不是真实芯片的结构承诺。它还能保留 Bare 下装入的旧项。第二轮 [K]：

```text
addr=0x80200038 satp=0x8000000000080205
$1 = 0x80200000
$2 = 0x0
```

这里 `$1` 为 `entry->addr_code`，`$2` 为 index；新 satp 已生效，但取指页仍命中旧 tag。`get_page_addr_code` 在 `cputlb.c:1030` 比较 tag，未进入 get_physical_address。`target/riscv/csr.c:739-749` 在这一版本的新规范分支只在 ASID 改变时显式 flush；本次 MODE/PPN 变化、ASID 没变，旧项没有随之刷掉。不能从此推导所有版本或真实 CPU 都有同样行为。

#### 第 5 问：调试中的细节

**写 satp 后为何低地址还能取指。** 源码 `code/kern/init/entry.S:29-42` 在写 satp 后才执行 sfence 并跳到高地址。页表无低地址映射，但 [K] 证明 `0x80200038` 的取指在 QEMU 4.1.1 命中了旧 Bare 代码项。`t7_inasm_kern_entry.txt` 的 TB 边界摘录：

```text
0x0000000080200034:  18029073          csrrw           zero,satp,t0
0x0000000080200038:  d030              sw              a2,96(s0)
0x000000008020003a:  d030              sw              a2,96(s0)
0xffffffffc02000d6:  00006517          auipc           a0,24576
```

该文件的 Trace 行还记录执行了从 `0x80200038` 开始的 TB，再进入高地址。结合 [K]、`csr.c:746-749` 和 [Leader notes 第2节](ai_logs/lab2/evidence/t7_leader_notes.md)，解释是：旧代码 TLB 项使第二个 TB 能被翻译；sfence 在执行时刷新 TLB（`target/riscv/insn_trans/trans_privileged.inc.c:90-95`、`op_helper.c:144-153`），但后续跳高地址指令已在同一 TB 内生成，不需逐条重新取指。第二轮 [K] 自身在第二次停点后 detach，并未跟踪到 kern_init；完整跳转证据来自独立的 in_asm/Trace 日志，不能混称同一轮。

环境记录中 QEMU 8.2.2 的 `epc=0x80200038` 与此差异相符，但“8.2.2 在 satp 改变时总刷新，因而暴露缺页”是**推测，未核对源码**。不把它写成已验证实现。

**反汇编显示与执行路径不同。** 第二个 TB 的日志把 `0x80200038` 起的指令显示为一串 `sw a2,96(s0)`，与 entry.S 应有的 sfence/跳转序列不符。原因**待确认**；Leader 推测是日志反汇编使用调试读接口，按新 satp 无法读到低地址，显示了不可靠字节。现有证据不足以证明这个原因，也不能说内核实际执行了一串相同 sw。

**1GB 页表映射，软件 TLB 仍只装4KB。** [B] 的根层叶子和 [E] 的 `size=0x1000` 同时成立；`cpu_helper.c:472-473` 固定按 TARGET_PAGE_SIZE 装表。映射的页大小与软件缓存项粒度不是一回事。

**返回的是页基址。** [C] 的 `*physical=0x80207000` 不含偏移8，真实数据地址为 `0x80207008`（`cpu_helper.c:333-334`）。Bare 分支直接返回原 addr，不能把两条返回路径混为一谈。

**gdb 读内存也能走翻译。** `exec.c:3964-3990` 的 `cpu_memory_rw_debug` 调试读会调用架构 debug 翻译，后者也调 get_physical_address（`cpu_helper.c:382-389`）。因此先看 bt 是否经真实 `riscv_cpu_tlb_fill/load_helper`。Task 07 [A] 栈里是实际 load；为更稳妥排除调试读取，演示稿先在 fill 的451行按数据地址与 access_type 下断点，再 step 进入翻译函数，避免只凭地址认定是客户机执行。

#### 第 6 问：真实的大模型协作过程

Leader（Claude Code）把源码分析与调试方案交给 Codex，形成 [Task 05](ai_logs/lab2/task_05.md)，四段式提示词原文保存在 [prompt.md](prompt.md#task05)。Codex 只读源码，输出分析与两个 gdb 脚本。子 Agent 之前因 Docker socket 权限无法执行，后续明确由子 Agent 分析、Leader 容器实测。Task 07 是 Leader 自己完成的调试记录，没有发给子 Agent 的提示词；不能补写一段虚构对话。

[Task 05 分析](ai_logs/lab2/evidence/t5_qemu_analysis.md) 预测的 helper→fill→页表路径被 [A] 证实；根层1GB叶子、physical 不含页内偏移、4KB软件项被 [B][C][E] 证实；Bare 仍装软件 TLB 被 [I][J] 证实。方案最初建议选择 npage 的读取并提到固件 load，Leader 实测改选自然 miss 的 `pages[0].flags` load 和入口的 `sd a0,0(t0)` store，报告采用实际目标，保留原方案不回写历史记录。

另一个真实问题是“写 satp 后为什么没在低地址缺页”。Task 07 与 Leader notes 记录的思路是：先用 `-d in_asm,exec,nochain` 找 TB 边界；发现 csrw 在前一个 TB 末尾，排除“csrw 与后续全部在旧 TB”这一解释；再读 `write_satp`，发现只在 ASID 改变时 flush；最后用 gdb 在 `get_page_addr_code:1030` 看 `addr_code`，用 [K] 确认旧项命中。异常的 sw 反汇编和8.2.2行为没有充分证据，继续标待确认。这里记录的是 Agent 的任务交接、源码分析和 Leader 验证，没有编造用户与模型逐句对话。

---

## 五、测试与验证

### P0：page_init 与 Best-Fit

以下引用 Leader 已保存的运行证据，本次报告整理没有重新运行命令。Task 04 的复核记录说明：修复后在 `os2026-dbg` 中编译无 warning，默认 128MB 运行正常。`make qemu` 的关键输出摘自 [t4_qemu.txt](ai_logs/lab2/evidence/t4_qemu.txt)，保留日志中的地址与格式：

```text
memory management: best_fit_pmm_manager
physical memory map:
  memory: 0x0000000008000000, [0x0000000080000000, 0x0000000087ffffff].
check_alloc_page() succeeded!
satp virtual address: 0xffffffffc0205000
satp physical address: 0x0000000080205000
```

日志未见 panic。`check_alloc_page()` 调用当前 Best-Fit 的 `check`，其中执行 `basic_check()` 与 `best_fit_check()` 的检查内容。QEMU 持续运行，由超时终止；不能将超时退出本身当作内核错误或测试通过的依据，判断依据是终止前的输出。`satp` 是保存地址转换模式与页表根位置的寄存器，这里打印的是启动页表的虚拟、物理地址，不是完整页表调试结论。

`make grade` 原始结果见 [t4_grade.txt](ai_logs/lab2/evidence/t4_grade.txt)：

```text
  -check page init:                          OK
  -check best fit:                           OK
Total Score: 20/20
```

Leader 还用 `-m 64M` 补测，关键输出摘自 [t4_qemu_m64M.txt](ai_logs/lab2/evidence/t4_qemu_m64M.txt)：

```text
physical memory map:
  memory: 0x0000000004000000, [0x0000000080000000, 0x0000000083ffffff].
check_alloc_page() succeeded!
```

这说明初始化会采用设备树给出的较小内存范围，没有把可用内存固定为 128MB。这不是单独一次 64MB grade 结果。

**越界分支的验证限制。** Task 03 用源码与布局推导出：在所记录的 `end=0xffffffffc0206078` 下，若内存大小为 `0x208000`，520 个描述符占用 `[0x80207000, 0x8020c140)`，超过内存末端 `0x80208000`。Task 04 已把检查放到描述符写入循环前。

Leader 尝试 `-m 2080K` 时，[t4_qemu_m2080K.txt](ai_logs/lab2/evidence/t4_qemu_m2080K.txt) 只有 QEMU 被 timeout 终止的一行，连 OpenSBI 横幅都没有，因此没有执行到 Case 5。Task 04 将原因标为推测：默认日志中的 DTB 地址为 `0x82200000`，距 RAM 起点约 34MB，固件启动可能受此布局限制。`bugs.md` 把它简写为“至少约 34MB 才能启动”，报告保留 Task 04 更谨慎的表述；现有证据不足以确认精确启动阈值。结论是当前 QEMU 启动配置下未能复现，Case 5 仅经静态审查，运行时状态仍为 **UNVERIFIED**。

### Buddy、SLUB 与分支任务

Buddy：TODO，补测试命令、覆盖范围和实际输出。

SLUB：TODO，补测试命令、覆盖范围和实际输出。

分支任务已由 Leader 在同一容器以双 gdb 批处理验证，本次仅整理证据。脚本为 [run.sh](ai_logs/lab2/evidence/t7_scripts/run.sh)、[qemu_tracer.gdb](ai_logs/lab2/evidence/t7_scripts/qemu_tracer.gdb)、[ucore_tracer.gdb](ai_logs/lab2/evidence/t7_scripts/ucore_tracer.gdb)、[paged.gdb](ai_logs/lab2/evidence/t7_scripts/paged.gdb)、[bare.gdb](ai_logs/lab2/evidence/t7_scripts/bare.gdb)。`run.sh <QEMU侧脚本> <ucore侧脚本> <输出前缀>` 自动运行对应一轮；不是在报告整理阶段再次执行。

原始输出为 [tracer](ai_logs/lab2/evidence/t7_tracer_qemu_gdb.txt)、[分页后 A～F](ai_logs/lab2/evidence/t7_paged_qemu_gdb.txt)、[Bare H～K](ai_logs/lab2/evidence/t7_bare_qemu_gdb.txt) 及 [TB 日志](ai_logs/lab2/evidence/t7_inasm_kern_entry.txt)，过程解释见 [Leader notes](ai_logs/lab2/evidence/t7_leader_notes.md)。已展示一次实际 load 的 VA→PA→宿主指针、根层大页、Bare store 的 miss/fill 和写 satp 后的旧代码项命中。[G] 未再停住，不存在“打印命中”的现场段；同页快路径的解释结合 Task 07 与源码。未验证真实 CPU、QEMU 8.2.2 刷新实现、victim 命中、A/D更新和三层4KB遍历。交互式三终端演示尚需用户按演示稿练习。

---

## 六、实验总结与收获

### 使用提示词让 AI 生成代码时出现的 bug 及解决

**问题 1：子 Agent 无权访问 Docker，属于环境问题。** Task 01 和 Task 02 的命令均返回退出码 1，提示 OrbStack `docker.sock` 的 `permission denied`，编译并未开始。记录见 [t1_qemu.txt](ai_logs/lab2/evidence/t1_qemu.txt) 与 [t2_compile.txt](ai_logs/lab2/evidence/t2_compile.txt)。处理方式是保持沙箱权限，由子 Agent 实现和静态自查，Leader 在 Docker 中编译、运行与评分。后续提示词明确了这项分工。不能把“命令已发出”写成“编译已通过”。

**Bug 2：规格遗漏了描述符数组本身放不下的情况。** Task 01 只规定数组之后没有完整空闲页时要提示，却没有要求在写数组前校验容量。默认 128MB 路径测试通过，Task 03 仍通过代码审查发现潜在越界。Task 04 补充 Case 5：数组物理末端超过内存上界时先 panic，且不得写任何描述符或调用 `init_memmap()`；原来的无空闲页分支只适用于数组已放得下的情况。修复后默认路径与 64MB 路径通过，grade 为 20/20；越界分支仍未动态验证，限制见第五节。根因和处理依据 [bugs.md](ai_logs/lab2/bugs.md) 及 [Task 04](ai_logs/lab2/task_04.md)。这说明除了检查函数最后能否得到空闲区，还要在每一步写入前检查该步骤本身是否安全。

### 本实验的重要知识点与 OS 原理知识点

| 知识点 | 实验中的含义 | OS 原理中的含义 | 关系与差异 |
|--------|--------------|----------------|------------|
| 物理页与 Page 描述符 | 每 4096 字节对应一个 `Page`，描述符数组也占用 RAM，长度为 `npage - nbase` | 用元数据追踪物理页的占用、引用等状态 | 描述符代表一页，但不是那页数据本身；本实验只管理一个受上限限制的连续 RAM 范围 |
| 虚拟地址、物理地址与 `va_pa_offset` | `end`、`pages` 是虚拟地址，减固定偏移得到物理地址，物理页号减 `nbase` 才是数组下标 | 页表把程序使用的虚拟地址翻译为物理地址 | 固定偏移仅适用于本实验的内核直接映射区域；不能把这种减法推广到任意进程地址 |
| 连续内存分配算法 | First-Fit 取第一个够用块，Best-Fit 完整搜索后取够用的最小块，分配从块头切出 | 在空闲区间中按策略选择、拆分和回收连续资源 | 本实验要求物理页连续；实际虚拟连续区域可以映射到不连续物理页，二者需求不同 |
| 机制与策略分离 | `pmm_manager` 提供统一调用接口，具体管理器决定选块方式 | 将“能执行哪些操作”和“如何选择资源”分离 | 替换管理器无需改上层分配接口；本实验通过 C 的函数指针表实现，不依赖对象语言 |
| 外部碎片与合并 | 空闲页散在多块时，大请求可能失败；释放时按物理相邻关系合并 | 资源总量充足但缺少所需连续区间 | 合并只能连接已空闲且相邻的块；不能跨过仍占用的页。Best-Fit 也不能保证消除碎片 |

TODO：补充 Buddy、SLUB 与分支任务完成后对应的其余知识点。

### OS 原理中重要但本实验没有对应的知识点

- 换页与页面置换：内存不足时选择页面换出到外存，再在需要时调入。本次 P0 只分配和释放物理页，没有外存交换或置换策略。
- 多进程地址空间与页表切换：不同进程可使用相同虚拟地址并映射到不同物理页。本次 P0 只使用启动内核映射，没有建立和切换多个进程的页表。
- 内存保护与权限检查：页表的读、写、执行及用户权限用于隔离访问。`entry.S` 有启动映射权限位，但本次 P0 没有实现用户与内核隔离，也没有专门验证权限异常。
- 按需分配与缺页处理：访问尚未建立映射的合法地址时再分配页面。本次初始化提前登记物理页，分配失败直接返回 `NULL`，没有形成进程缺页处理流程。

TODO：其余任务完成后再核对上述范围，并补充未对应的知识点。

### AI 协作开发的经验

P0 中，四段式提示词明确了接口、修改范围和证据要求，交叉审查补出了默认测试无法覆盖的布局边界。实现和运行分工也必须与真实权限一致：子 Agent 无权访问 Docker 后，运行结果改由 Leader 提供，报告保留原始失败和后续验证两条记录。对没有覆盖到的分支，应写明静态检查与运行验证的区别。

TODO：Buddy、SLUB 与分支任务完成后补充其余协作经验。
