# Task 06：First-Fit 思考题、Challenge 与报告 P0 部分

执行者：Codex w1:p2（Report Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Report Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`。用户是初学者，报告要能让他读懂、答辩时讲得出来。

任务：在 `report/report.md` 和 `report/prompt.md` 中写完与 P0 有关的部分：
1. `report.md`：
   - “实验基本信息”：完成日期写 `2026-10-24（进行中）`；小组分工写“单人完成全部练习”。
   - “二、实验环境”：AI 工具表填 Leader = Claude Code（Claude Opus 5.5），子 Agent = Codex CLI（GPT-6.1-Sol medium，Herdr 面板 w1:p2、w1:p3）；**改正软件环境段落**：本实验实际使用 Docker 镜像 `os2026-dbg`，内含 RISC-V GCC 13.2.0、**QEMU 4.1.1（`--enable-debug` 编译，同一个可执行文件既用于运行也用于分支任务调试）**、gdb-multiarch，OpenSBI 版本取自 `report/ai_logs/lab2/evidence/t3_qemu.txt` 的实际启动输出；说明不用系统 QEMU 8.2.2 的原因（`entry.S` 的启动页表没有低地址恒等映射，8.2.2 下 `csrw satp` 后下一次取指缺页、内核无输出；4.1.1 正常）——这条原因写成“环境调研结论”，不要写成本仓库测出来的结果。
   - “三、实验整体逻辑分析”：3.1、3.2 按指导书 `实验指导书/lab2/lab2_2_1_exercise.md` 开头的实现链 `pmm_init() -> init_pmm_manager() -> page_init() -> init_memmap() -> alloc_pages()/free_pages()` 写，Buddy/SLUB/分支任务部分先写一句“见后文（进行中）”。
   - “功能模块一 任务一 page_init”：模块功能描述、spec 怎么写（四段式各写了什么、为什么这样分）、最终提示词（引用 `prompt.md` 对应小节，不在正文重复全文）、实现迭代过程（Task 01 → Task 03 review 发现 → Task 04 修复，事实以 `report/ai_logs/lab2/task_01.md`、`task_03.md`、`task_04.md`、`bugs.md` 为准）。
   - “Challenge”：回答“OS 无法提前知道可用物理内存范围时如何获取”，至少覆盖：固件/引导程序提供（BIOS e820 / UEFI memory map / 设备树 / ACPI）、OS 自己探测（逐页写入特征值再读回，并说明它的风险：MMIO 区域、内存空洞、缓存、别名、破坏已有数据），以及在本实验 RISC-V + OpenSBI 环境里实际用的是哪一种。可参考 10-13 实验课老师讲的 x86 `int 15h` 与交替写 0x55/0xAA 的方法。
   - “功能模块二 任务一 First-Fit”：结合 `code/kern/mm/default_pmm.c` 的 `default_init`、`default_init_memmap`、`default_alloc_pages`、`default_free_pages` 用自己的话说明初始化、分配、回收与合并（给出函数名和关键步骤，不要大段贴代码）；回答“是否有改进空间”（如：线性扫描 O(n)、外部碎片、低地址小碎片聚集、可用 next-fit / 按大小分级链表 / 平衡树 / buddy 等，各说一句代价）。
   - “功能模块二 任务二 Best-Fit”：spec、提示词引用、迭代过程（Task 02 → Leader 切换管理器 → Task 03 PASS）、在 `init_pmm_manager()` 的切换；回答“是否有改进空间”（含 Task 03 review 的 NOTE：现有测试未覆盖的分支；Best-Fit 每次全表扫描、容易留下很小的碎片；可用按大小排序的结构等）。
   - “五、测试与验证”：P0 部分——`make qemu` 关键输出（摘自 `evidence/t4_qemu.txt`，按原样）、`make grade` 20/20（`evidence/t4_grade.txt`）、`-m 64M` 补测（`evidence/t4_qemu_m64M.txt`）、越界分支无法在 QEMU 中复现（为什么）。
   - “六、实验总结与收获”中“bug 及解决”写 bugs.md 的问题 1、Bug 2；“重要知识点与 OS 原理知识点”先写 P0 相关的（物理页与 Page 描述符、虚拟/物理地址与 va_pa_offset、连续内存分配算法、机制与策略分离、外部碎片），每项写“实验中的含义 / 原理中的含义 / 关系与差异”；“OS 原理中重要但本实验没对应的”写几条（如换页与页面置换、多进程地址空间与页表切换、内存保护与权限位在本实验中只用了内核映射等）。
   - 尚未完成的部分（Buddy、SLUB、分支任务、其余知识点）保留原有 TODO 标题，不要编造内容。
2. `prompt.md`：按任务分节，原样收录 `task_01.md`、`task_02.md`、`task_04.md` 中 `## PROMPT` 到 `## SPECIFICATION` 末尾的四段式提示词全文（逐字复制，不要改写），每节开头一行注明“发送对象、发送时间、对应 ai_logs 文件”。Task 03（review）也收录，标注为“Review 提示词”。

操作要求：
- 直接修改 `report/report.md`、`report/prompt.md` 两个文件；不改代码、不改 `report/ai_logs/` 下已有文件、不 git commit / push。
- 所有事实（命令输出、地址、分数、时间、bug 经过）必须来自 `report/ai_logs/lab2/` 下的日志和 evidence，或来自你实际读到的源码；查不到的写 TODO，不要编造。
- 文风：中文、以文字说明为主、短句；术语第一次出现时用一句话解释。不要使用夸张形容词。
- 完成后回复 ≤10 行：改了哪些小节、有哪些 TODO 留着、你拿不准的事实。

## RELY

- 指导书：`/Users/wesley/大三上课程/操作系统/实验指导书/lab2/`（`lab2_2_1_exercise.md`、`lab2_3_5_2_search_phymem_layout.md`、`lab2_5_labs_requirement.md`）。
- 代码：`code/kern/mm/pmm.c`、`best_fit_pmm.c`、`default_pmm.c`。
- 过程记录：`report/ai_logs/lab2/overview.md`、`task_01.md`～`task_04.md`、`bugs.md`、`evidence/`。
- lab1 报告可参考结构：`git show lab1:report/report.md`。
- 报告要求（`lab2_5_labs_requirement.md`）：说明整体逻辑、核心函数理解、每个练习的 spec 和 prompt 怎么写、AI 生成代码时的 bug 与解决、重要知识点与 OS 原理对应、原理中重要但实验没对应的知识点。

## GUARANTEE

- `report.md` 中 P0 相关小节、Challenge、实验环境不再有 TODO；其余未完成小节保留 TODO。
- `prompt.md` 中提示词与 `task_0X.md` 逐字一致。
- 不修改其它文件。

## SPECIFICATION

- Preconditions：P0 已提交（commit `9c65e68`），日志和 evidence 完整。
- Postconditions：两个文件更新；`git diff --stat` 只显示 `report/report.md`、`report/prompt.md`。
- Edge cases：日志与源码有出入时以源码和 evidence 为准，并在回复中指出；某个事实在日志里找不到时写 TODO 而不是推断。

## Agent Result

Codex 回复（2026-10-24 21:33）：完成 P0 的实验环境、整体逻辑、page_init、Challenge、First-Fit、Best-Fit、测试与总结；Task 01～04 四段式提示词逐字核对后收入 `prompt.md`；Buddy、SLUB、分支任务及后续总结保留 TODO；“约 34MB”的启动限制按推测处理，Case 5 保持 UNVERIFIED。`git diff --stat` 仅 `report/report.md`、`report/prompt.md`。

## Verification

Leader 审读：事实均可追溯到 ai_logs/evidence。待改：① 实验环境段落写“工具与环境信息依据 Task 06”，把提示词当成事实来源，应改为引用启动输出与环境配置文档；② QEMU 8.2.2 的说明可补充 Task 07 发现的 `write_satp` 旧 TLB 表项解释。两处在分支任务章节完成后一起修订。

状态：**DONE**（P0 部分）。
