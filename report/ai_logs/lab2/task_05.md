# Task 05：分支任务 · QEMU 4.1.1 地址翻译源码分析与调试方案

执行者：Codex w1:p3（Analysis Agent）；gdb 实际调试由 Leader 执行（子 Agent 沙箱无法访问 Docker，见 bugs.md 问题 1）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 分支任务的 Analysis Agent。目标：为“用 gdb 调试正在运行 ucore 的 QEMU，观察一次访存的虚拟地址如何被翻译成物理地址”准备**源码分析和可直接执行的调试方案**。你不能运行 Docker / QEMU / gdb，只做静态源码阅读；Leader 会按你的方案在容器里实际调试，并把真实输出反馈给你。

任务：
1. 阅读 QEMU 4.1.1 源码副本 `/Users/wesley/大三上课程/操作系统/tmp/qemu-src/`（从容器 `/opt/qemu-src/qemu-4.1.1` 原样拷出，含 `target/riscv`、`accel/tcg`、`include/exec`、`exec.c`、`memory.c`），给出 RISC-V 64 位客户机一次**数据 load**（例如 `ld`）从 TCG 生成的 helper 到得到物理地址、再到访问客户机内存的**完整调用路径**（函数名 + 文件:行号），以及路径上的关键分支（TLB 命中/未命中、victim TLB、`tlb_fill` 失败抛异常、MMU 模式判断、页表项有效性/权限/叶子判断、大页、A/D 位处理等）。
2. 详细解释 `target/riscv/cpu_helper.c` 中 `get_physical_address` 的 SV39 页表遍历：每一级如何从 VA 取索引、如何算 PTE 地址、如何读 PTE（读用的是哪个函数，读的是客户机物理地址）、什么条件下继续下一级、什么条件下是叶子、最后如何拼出物理地址。
3. 找出模拟 CPU 查 TLB 的 C 代码（`accel/tcg/cputlb.c` 中 `load_helper` / `tlb_hit` / `victim_tlb_hit` / `tlb_set_page_with_attrs` 等，以实际源码为准），说明 TLB 的组织（`CPUTLBEntry` 里存了什么、按什么索引、`mmu_idx` 的作用、`addend` 是什么）。
4. 分析 QEMU 软件 TLB 与真实 CPU TLB 的**逻辑区别**，并说明“未开启分页（satp 为 Bare 或 M 模式）时的访存”在 QEMU 里是否也走 TLB / `tlb_fill` / `get_physical_address`，哪个分支直接返回。结论必须有源码行号支撑；拿不准的写“待调试确认”。
5. 设计调试方案，Leader 将在容器里执行：
   - 终端 1：`./os2026.sh debug`（QEMU 带 `-s -S` 启动，等待 gdb）；终端 2：`./os2026.sh gdb-qemu`（gdb 附加到 QEMU 进程，已设 `handle SIGPIPE nostop noprint` 和 `set remotetimeout unlimited`）；终端 3：`./os2026.sh gdb`（riscv gdb 连 ucore）。Leader 也可能用 gdb 的批处理模式（`gdb -batch -x <script>`）自动化，所以请把终端 2 和终端 3 的命令各写成一个可直接 `source` 的 gdb 脚本。
   - 选定观察目标：(a) 一条**开启分页之后**的内核访存，建议用 ucore 中一定会被访问的地址（例如某个全局变量，或 `kern_init` 入口取指），给出如何用 `riscv64-unknown-elf-objdump`/`nm` 找到它的虚拟地址；(b) 一条**开启分页之前**（`entry.S` 里写 satp 之前，或 OpenSBI 阶段）的访存，用于第 4 问对比。
   - 条件断点：在 `get_physical_address`、`riscv_cpu_tlb_fill`、`load_helper`（或其被内联后的实际可下断点符号）上设置以地址为条件的断点；注意排除 gdb 读内存触发的调试访问（调用栈里有 `cpu_memory_rw_debug` 的那次），给出排除方法。
   - 每个断点命中后要打印的变量（例如 `addr`、`mode`、`base`、`levels`、`ptesize`、`idx`、`pte_addr`、`pte`、`ppn`、`*physical`、`tlb_entry->addr_read`、`addend`），以及 `bt` 的预期样子。
6. 列出你认为答辩时最值得演示的“主线”：从哪个断点开始、单步几次、看哪几个变量，能在 3 分钟内讲清一次翻译。

操作要求：
- 只读源码，**只允许新建**以下两个文件，不修改任何其它文件，不 git commit / push：
  - `<lab2 日志目录>/evidence/t5_qemu_analysis.md`：第 1～4、6 项的分析（中文，引用源码时给 `文件:行号` 和短代码片段）；
  - `<lab2 日志目录>/evidence/t5_gdb_qemu.gdb` 与 `t5_gdb_ucore.gdb`：第 5 项的两个 gdb 脚本，脚本里每条命令后用 `#` 注释说明用途。
- 不确定的地方明确标“待调试确认”，不要把推测写成事实。
- 完成后回复 ≤15 行：调用路径一行版、TLB 查找函数位置、两个观察目标地址的获取方法、你最不确定的 3 点。

## RELY

- QEMU 版本 4.1.1，以 `--enable-debug` 编译，`-machine virt`，OpenSBI v0.4（启动输出）。容器内源码 `/opt/qemu-src/qemu-4.1.1`，本地只读副本 `/Users/wesley/大三上课程/操作系统/tmp/qemu-src/`。
- Leader 已在 2026-10-24 验证：gdb 能在 `get_physical_address`（`target/riscv/cpu_helper.c:158`）和 `riscv_cpu_tlb_fill`（同文件 `:438`）下断点命中；gdb 读客户机内存时也会走 `get_physical_address`，调用栈中有 `cpu_memory_rw_debug`。
- ucore 侧（`code/`，已编译通过）：`kern/init/entry.S` 在 `kern_entry` 里构造 `satp`（Sv39，`boot_page_table_sv39`）后 `csrw satp`、`sfence.vma`，再跳到高地址的 `kern_init`；本次运行 `kern_init` 等符号在 `0xffffffffc02000xx` 一带（以 `nm bin/kernel` 为准）；`pmm_init` 打印 `satp physical address: 0x0000000080205000`。启动页表把 `0xffffffffc0000000` 起 1GB 映射到物理 `0x80000000`（1GB 大页）。
- 指导书：`/Users/wesley/大三上课程/操作系统/实验指导书/lab2/lab2_4_qemu_mmu_debug.md`（6 个调试要求）。
- 入口脚本：`/Users/wesley/大三上课程/操作系统/os2026.sh`（`debug` / `gdb-qemu` / `gdb` 子命令，读它确认实际命令）。

## GUARANTEE

- 两个 gdb 脚本只包含 gdb 命令和注释，Leader 可直接 `source`。
- 分析中的每个结论都附源码 `文件:行号`；推测标“待调试确认”。
- 不修改任何已有文件。

## SPECIFICATION

- Preconditions：P0（page_init、Best-Fit）已完成并编译通过；QEMU 源码副本存在。
- Postconditions：上述 3 个文件存在、非空；回复包含 6 项要求的摘要。
- Edge cases：
  - 若某函数在 `--enable-debug`（-O0）下仍被宏展开或内联、无法直接下断点，给出可替代的断点位置（文件:行号）。
  - 若 4.1.1 中 TLB 查找代码与上面列的函数名不同，以实际源码为准并说明。
  - 1GB 大页（Sv39 第一级就是叶子）会让页表遍历只走一级，请说明这对观察有什么影响，并建议是否另找一个经过多级页表的地址（若 ucore 本实验中不存在多级映射，如实说明）。

## Agent Result

Codex 回复（2026-10-24 21:38）：新建 `evidence/t5_qemu_analysis.md`、`t5_gdb_qemu.gdb`、`t5_gdb_ucore.gdb`，未运行 Docker/QEMU/gdb。调用主线：TCG load → `helper_le_ldq_mmu` → 软件 TLB（`cputlb.c:1271`，`tlb_hit` 定义在 `include/exec/cpu-all.h:359`，victim `:977`）→ miss 后 `tlb_fill` → `riscv_cpu_tlb_fill` → `get_physical_address` Sv39 遍历 → `tlb_set_page_with_attrs` 装表 → `haddr = addr + addend` 读宿主 RAM。指出：`load_helper` 强制内联；同页后续访问可能在生成代码快速路径命中而不进 C helper；1GB 大页只走一层；`get_physical_address` 分页路径返回 4KB 页基址不含页内偏移；`write_satp` 只在 ASID 变化时 flush；gdb 读内存走 `cpu_memory_rw_debug` 需排除。最不确定：副本缺 TCG 后端源码、内联/TLB 冷热影响命中、双 gdb 批处理交接。

## Verification

Leader 按该分析的行号（`cpu_helper.c:197/254/334`、`cputlb.c:1030/1271/1338/1519`）写批处理脚本，在 Docker 中实测（Task 07）：调用栈、Sv39 一层遍历、`addend`、Bare 分支、快速路径不进 helper 等预测全部与实测一致；`get_physical_address` 返回页基址 `0x80207000`、完整物理地址需加页内偏移 `0x008`，也与分析一致。Codex 自己的两个 gdb 脚本没有直接使用（为交互式设计，含门控变量），Leader 改写为批处理脚本 `evidence/t7_scripts/`。

状态：**DONE**（静态分析经 Task 07 实测验证）。
