# Task 08：分支任务报告章节与答辩演示步骤

执行者：Codex w1:p3（Report Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Report Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`。用户是初学者，答辩时要现场演示并回答助教问题，而且不能借助大模型作答。

任务：
1. 在 `report/report.md` 的“分支任务：gdb 调试页表查询过程”一节，替换全部 TODO，写完：
   - “调试环境”：三个终端各做什么、启动顺序（`./os2026.sh debug` → `./os2026.sh gdb-qemu` 设断点后 `continue` → `./os2026.sh gdb`）；调试版 QEMU 从哪来（镜像 `os2026-dbg` 内 `--enable-debug` 编译的 QEMU 4.1.1，源码在容器 `/opt/qemu-src/qemu-4.1.1`，因此不需要指导书里“重新编译、改 Makefile 的 QEMU 路径、sudo gdb”这几步）；报告里的数据是 Leader 用批处理脚本（`ai_logs/lab2/evidence/t7_scripts/`）在同一个容器里自动跑三终端得到的，交互式命令与脚本相同。
   - 第 1～6 问逐一回答。事实只能来自 `ai_logs/lab2/task_05.md`、`task_07.md`、`evidence/t5_qemu_analysis.md`、`evidence/t7_*.txt`、`evidence/t7_leader_notes.md` 和你实际读到的源码（`/Users/wesley/大三上课程/操作系统/tmp/qemu-src/`）。每个关键结论给出 `文件:行号` 和对应的实测输出（短引用，代码块）。要求：
     - 第 1 问：给出一行版调用路径 + 每一环的作用 + 关键分支（TLB 命中/未命中/victim、Bare/M 直接返回、V 位、叶子判断、权限、A/D），并用 Task 07 第一轮 [A]～[F] 演示 VA `0xffffffffc0207008` → PA `0x80207008` → 宿主指针的全过程。
     - 第 2 问：单步讲解 Sv39 遍历，用实测数值算一遍（`satp` → 根页表 `0x80205000` → `idx=511` → `pte_addr=0x80205ff8` → `pte=0x200000cf` → 叶子 → `ppn` → 拼接）；说明本实验启动页表是 1GB 大页，所以只走一层，若是 4KB 页会走三层（按源码说明循环如何继续，不要编造三层的实测数据）。
     - 第 3 问：C 语言 TLB 查找在 `load_helper`/`store_helper`（`cputlb.c:1271`、`:1519`）、`tlb_hit`、`victim_tlb_hit`；`CPUTLBEntry` 存什么、怎么索引、`mmu_idx`、`addend`；用实测 [H]（`addr_write=0xffffffffffffffff` 空表项 → 未命中 → victim → fill）和 [G]（同页后续访问在生成代码快速路径命中、不进 C helper）说明。
     - 第 4 问：逻辑区别至少包括——QEMU 软件 TLB 缓存的是“客户机虚拟页 → 宿主内存指针（addend）”，真实 TLB 缓存“虚拟页 → 物理页 + 权限”；QEMU 在 Bare 模式下也把访存装进软件 TLB（实测 [I][J]：`case VM_1_10_MBARE` 直接 `*physical = addr`，仍 `tlb_set_page`），真实 CPU 在 Bare 模式下不做地址翻译；QEMU 的 TLB 按 `mmu_idx`（特权级）分组、大小和替换策略是软件选择；以及 [K] 写 satp 后旧表项仍然命中（`csr.c:746` 只在 ASID 变化时 flush）。真实 CPU 的部分只写规范/教材层面的说法，并标明未实测。
     - 第 5 问：有趣细节，至少写：写 satp 之后低地址取指为什么没有缺页（[K] + `t7_leader_notes.md` 第 2 节，QEMU 8.2.2 部分标“推测，未核对源码”）；`-d in_asm` 日志里 0x80200038 那个 TB 被反汇编成一串 `sw a2,96(s0)`（原因待确认，如实写）；1GB 大页在软件 TLB 里只装 4KB；`get_physical_address` 返回页基址不含页内偏移；gdb 读内存也会触发 `get_physical_address`（`cpu_memory_rw_debug`），所以断点要加条件。
     - 第 6 问：只记录真实发生的大模型交互：Leader（Claude Code）把源码分析任务交给 Codex（Task 05 原文在 `prompt.md`）、Codex 给出的哪些预测被实测证实；satp 疑问的情景、思路（先看 `-d in_asm` 的 TB 边界，再读 `write_satp`，再用 gdb 在 `get_page_addr_code` 验证）；子 Agent 无法访问 Docker 后的分工调整。不要编造用户本人与大模型的对话。
   - 同时修订两处（Leader 审读意见，见 `task_06.md` Verification）：① “二、实验环境”里“工具与环境信息依据 Task 06”这句改为引用启动输出（`evidence/t3_qemu.txt`）和 `/Users/wesley/大三上课程/操作系统/环境配置状态.md`；② 该节对 QEMU 8.2.2 的说明后面补一句指向分支任务第 5 问的解释（4.1.1 能跑是因为旧 TLB 表项，8.2.2 的部分仍为推测）。
   - “五、测试与验证”里“Buddy、SLUB 与分支任务”小节：分支任务部分写明用到的脚本和 evidence 文件；Buddy、SLUB 保留 TODO。
2. 新建 `report/ai_logs/lab2/分支任务演示步骤.md`：给用户自己答辩用的 3 分钟演示稿。内容：三个终端逐条要敲的命令（交互式，不是批处理）；在终端 2 设的条件断点（排除 `cpu_memory_rw_debug` 的办法）；每停一次要 `print` 什么、看到什么数值、该怎么讲一句话；可能遇到的问题（`Ignoring packet error` → `set remotetimeout unlimited`、容器名冲突 `docker rm -f os`、断点不命中时怎么办）；最后列 8～10 个助教可能问的问题和简短答案要点（只用本报告里有依据的内容）。
3. `report/prompt.md`：在末尾追加 Task 05、Task 07（说明 Task 07 是 Leader 自己执行、没有发给子 Agent 的提示词，只附一行说明，不要编造提示词）和 Task 08 的四段式提示词原文（从 `task_05.md`、`task_08.md` 的 `## PROMPT` 到 `## SPECIFICATION` 末尾逐字复制），格式与前面小节一致。

操作要求：
- 只修改 `report/report.md`、`report/prompt.md`，新建 `report/ai_logs/lab2/分支任务演示步骤.md`；不改代码、不改其它日志文件，不 git commit / push。
- 不运行 Docker / QEMU / gdb（你的沙箱也用不了）。
- 中文、短句、术语第一次出现时一句话解释；推测必须标“推测”或“待确认”。
- 完成后回复 ≤10 行。

## RELY

- 实测证据：`evidence/t7_paged_qemu_gdb.txt`（第一轮 [A]～[G]）、`evidence/t7_bare_qemu_gdb.txt`（第二轮 [H]～[K]）、`evidence/t7_tracer_qemu_gdb.txt`、`evidence/t7_inasm_kern_entry.txt`、`evidence/t7_leader_notes.md`、`task_07.md`。
- 源码分析：`evidence/t5_qemu_analysis.md`；源码副本 `/Users/wesley/大三上课程/操作系统/tmp/qemu-src/`。
- 指导书：`/Users/wesley/大三上课程/操作系统/实验指导书/lab2/lab2_4_qemu_mmu_debug.md`。
- 助教 2026-10-23 01:33 通知：答辩时只需简单演示讲解关键部分，更多调试细节放在报告中；遇到 `Ignoring packet error` 用 `set remotetimeout unlimited`。
- 入口脚本：`/Users/wesley/大三上课程/操作系统/os2026.sh`。

## GUARANTEE

- 分支任务一节 6 问全部回答，没有 TODO；每个实测数值都能在 evidence 里找到。
- 演示步骤文件可照着逐条执行。
- `prompt.md` 新增内容与 `task_05.md`、`task_08.md` 逐字一致。

## SPECIFICATION

- Preconditions：Task 05、07 已完成，evidence 齐全；`report.md` 已有 P0 部分（Task 06）。
- Postconditions：`git diff --stat` 只显示 `report/report.md`、`report/prompt.md` 和新文件。
- Edge cases：evidence 与分析文件说法冲突时以 evidence 为准并在回复中指出；没有依据的内容写“待确认”，不要补全。

## Agent Result

Codex 回复（2026-10-24 21:50）：`report.md` 补齐分支任务 6 问、实验环境两处修订与验证证据；新建 `分支任务演示步骤.md`（三终端命令、三分钟讲稿、故障处理、10 个问答）；`prompt.md` 逐字追加 Task 05、08，Task 07 只记执行说明。区分了实测、源码推导与待确认内容；未运行 Docker/QEMU/gdb。

## Verification

Leader（2026-10-24）：
- 审读分支任务一节：6 问的实测数值均可在 `evidence/t7_*.txt` 中找到；推测（8.2.2 的刷新行为、`sw a2,96(s0)` 反汇编原因）均已标注；第 6 问只记录真实的 Agent 交互。
- **验证演示稿**：把演示稿主线 A～F 在终端 2、终端 3 要敲的命令原样写成 `evidence/t7_scripts/demo_main.gdb`、`demo_ucore.gdb`，在容器中按三终端顺序批处理运行（`evidence/t8_demo_validation_qemu_gdb.txt`、`t8_demo_validation_ucore_gdb.txt`）：全部命令无报错；`riscv_cpu_tlb_fill:451` 条件断点命中，`step` 进入 `get_physical_address`，`$1`～`$33` 的值与演示稿所列历史值一致（`address=0xffffffffc0207008`、`base=0x80205000`、`idx=511`、`pte=0x200000cf`、`*physical=0x80207000`、返回值 0、`prot=7`、`size=0x1000`、`addend=0xffffafe00000`、`haddr=0xffff70007008`）；终端 3 的 `p/x` 计算出 `0xffffffffc0207008`、`&kern_init = 0xffffffffc02000d6`。可选补充段（Bare 与 [K]）未按演示稿单独重跑，其命令与 Task 07 `bare.gdb` 相同。
- 交互式（真正三个终端手敲）尚未由用户本人练习。

状态：**DONE**。
