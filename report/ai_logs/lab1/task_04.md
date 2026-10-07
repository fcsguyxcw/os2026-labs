# Task 04：撰写实验报告与测试截图

执行者：Codex（Report Agent，Herdr 面板 w9:p7）

## PROMPT

你是 OS Lab1 的 Report Agent。仓库在 WSL `Ubuntu-24.04` 的 `/home/wesly/courses/os2026`（分支 `lab1`）。Windows 侧路径 `\\wsl$\Ubuntu-24.04\home\wesly\courses\os2026`。

任务一：生成测试截图到 `report/images/`。
- WSL 里没有 Pillow，请用 Windows 侧 Python（已有 Pillow 11.0）编写脚本，把下列真实证据文件的关键片段渲染成终端风格 PNG（深色背景、等宽字体，中文用 `C:\Windows\Fonts\msyh.ttc` 或其他系统字体）。只裁剪片段，不得改写或编造任何一行内容：
  1. `01_make_qemu.png`：`evidence/t2_qemu.txt` 中 OpenSBI 横幅里的 `Domain0 Next Address` 附近若干行 + 最后的 `(THU.CST) os is loading ...`。
  2. `02_reset_mrom.png`：`evidence/t3_a_reset.txt` 中 `x/10i $pc`、`x/6gx 0x1000` 以及跳转到 0x80000000 的单步片段。
  3. `03_opensbi_entry.png`：`evidence/t3_b_opensbi.txt` 中断点命中与 `x/12i`。
  4. `04_watch_vs_break.png`：`evidence/t3_c_watch.txt` 全部有效输出。
  5. `05_kernel_entry.png`：`evidence/t3_d_kernel.txt` 中 kern_entry 命中、`la` 前后 sp 变化、tail 后进入 kern_init 的片段。
  6. `06_putchar_ecall.png`：`evidence/t3_d_kernel.txt` 中 sbi_console_putchar 调用栈和 ecall 前寄存器。
  - 每张图顶部加一行灰色标题注明「来源：evidence/xxx.txt（终端输出渲染）」。渲染脚本保存为 `report/ai_logs/lab1/render_images.py`。

任务二：撰写 `report/report.md`。
- 结构严格参照仓库根目录 `实验报告模板.md`（先完整阅读），保留其所有章节标题；模板里“示例/提示”性质的占位内容要替换为本实验的真实内容或删除。
- 必须覆盖指导书的报告要求（见下方 RELY），并回答练习 1、练习 2 的全部问题。
- 小组信息：单人小组；学号、姓名写成 `学号-姓名（待填写）`；分工表只有一人，负责全部练习与报告。
- 实验环境 / AI 工具表写三行：Claude Code（Claude Opus 5.5）——Leader，负责读需求、拆任务、写 Spec、复核；Codex CLI（GPT-6.1-Sol，medium）——实现 / 测试 / 调试 / 报告撰写；pi（deepseek-flash）——Review。另列软件环境：WSL Ubuntu-24.04、riscv64-unknown-elf-gcc 13.2.0、QEMU 8.2.2、OpenSBI v1.3、gdb-multiarch 15.1。
- 必须写进报告的事实（全部来自 evidence 与 ai_logs，引用具体数值）：
  - Bug 1 及修复（读 `ai_logs/lab1/bugs.md`、`evidence/t1_qemu.txt`、`t2_qemu.txt`、`t2_makefile.diff`），解释 QEMU 4.1.1/OpenSBI v0.4 与 QEMU 8.2.2/OpenSBI v1.3 fw_dynamic 的差异。
  - 练习 1：结合 `t3_d_kernel.txt` 说明 `la sp, bootstacktop` 被汇编为 `auipc sp,0x3` + `mv sp,sp`（即 addi sp,sp,0），sp 由 OpenSBI 留下的 0x80046eb0 变为 0x80203000；bootstack=0x80201000，KSTACKSIZE=2 页=8KB，栈向低地址增长；目的是给 C 代码建立独立内核栈。`tail kern_init` 被汇编为 `j kern_init`（不写 ra），ra 仍为 0x8000ae9a，目的是转入 C 入口且不需要返回（kern_init 为 noreturn）。
  - 练习 2：复位地址 0x1000 的 6 条指令逐条说明（auipc/addi/csrr mhartid/ld/ld/jr），a0=hartid=0、a1=设备树地址 0x87e00000、a2=0x1028 指向 fw_dynamic_info（魔数 0x4942534f="OSBI"）、t0=0x80000000；OpenSBI 入口指令；`watch *0x80200000` 没有触发而断点先命中——因为内核镜像由 QEMU 在 CPU 开始执行前就放进内存（阶段 A 在 0x1000 处已能读到内核机器码），这与指导书提示“SBI 加载内核”不同，需如实说明；最后跳入 0x80200000。附调试过程使用的 GDB 命令。
  - 输出链：kern_init → cprintf → vcprintf → vprintfmt → cputch → cons_putc → sbi_console_putchar → sbi_call 中 `ecall`（a7=1 即 SBI_CONSOLE_PUTCHAR，a0=0x28 '('）。
  - 链接脚本：`ENTRY(kern_entry)`、`BASE_ADDRESS=0x80200000`、段地址（`evidence/t1_symbols.txt`）。
- 报告中用相对路径 `images/xx.png` 引用截图；每张图都要在正文中被引用并说明。
- 写清楚“AI 协作过程”：按 task_01～task_04 概述 Leader→Agent 的分工、Bug 1 的发现与修复（这些 md 文件在 `report/ai_logs/lab1/`，可以链接）。
- 语言：简体中文，以文字说明为主，不要堆砌无依据的套话；不确定的内容不要写成事实。

允许修改：新建/覆盖 `report/report.md`、`report/images/*.png`、`report/ai_logs/lab1/render_images.py`。
禁止：修改 `code/`、`evidence/`、`ai_logs` 下已有 .md 文件；git commit/push。

输出：回复 ≤10 行：生成了哪些图片（尺寸）、report.md 的章节列表、有无无法确认而留空的内容。

## RELY

- 指导书 lab1_5 报告要求：① 本章整体逻辑线（按什么顺序、围绕哪些功能逐步实现）；② 每个功能的核心函数或模块的理解；③ 本实验重要知识点与 OS 原理知识点的对应、含义、关系和差异；④ OS 原理中重要但本实验未对应的知识点；⑤ 回答所有练习问题。
- 练习 1 原题：阅读 kern/init/entry.S，说明 `la sp, bootstacktop` 完成了什么操作、目的是什么；`tail kern_init` 完成了什么操作、目的是什么。
- 练习 2 原题：用 GDB 跟踪 QEMU 模拟的 RISC-V 从加电到执行内核第一条指令（跳转到 0x80200000）的过程；RISC-V 硬件加电后最初执行的几条指令位于什么地址、主要完成了哪些功能；报告中简要记录调试过程、观察结果和答案。提示：复位地址 0x1000 → OpenSBI 初始化并（据指导书）加载内核到 0x80200000（可用 `watch *0x80200000`）→ `b *0x80200000` 验证。
- 指导书执行流：加电复位 → CPU 从 0x1000 进入 MROM → 跳转 0x80000000(OpenSBI) → 加载内核到 0x80200000 → entry.S → kern_init() → 输出信息。
- 代码事实：见 Task 01～03 的 RELY 与 Verification 章节。
- 证据文件：`report/ai_logs/lab1/evidence/` 下 t1_*、t2_*、t3_*。

## GUARANTEE

- `report/report.md` 存在，章节与模板一致，覆盖 RELY 中五项报告要求和两道练习。
- 报告中的所有地址、寄存器值、指令与 evidence 文件一致。
- `report/images/` 下 6 张 PNG，内容是 evidence 原文的截取，正文全部引用。

## SPECIFICATION

- Preconditions：Task 01～03 DONE；evidence 文件齐全。
- Postconditions：上述文件存在；`git status` 中 `code/`、`evidence/` 无新改动。
- Edge cases：
  - 模板中要求填写但本实验没有对应内容的项（如“测试结果统计/make grade”），写明“本实验无 grade 脚本（压缩包缺 tools/grade.sh），以 make qemu 输出与 GDB 观察作为验证”。
  - 字体缺失时退回 Consolas/默认字体，但必须保证中文可读。

## Agent Result

Codex 回复（2026-10-07 15:08）摘要：生成 6 张 PNG（01_make_qemu 1100×379、02_reset_mrom 1100×1000、03_opensbi_entry 1100×676、04_watch_vs_break 1100×352、05_kernel_entry 1100×1243、06_putchar_ecall 2355×811）；`report.md` 按模板章节撰写；学号姓名留待填写；代码、证据与已有 Markdown 未改。

## Verification

Leader 通读 `report.md`，抽查 `06_putchar_ecall.png` 与 `t3_d_kernel.txt` 一致；发现需在 Review 后更新的表述（pi 结果未出、“不执行 commit/push”）。交 Task 05 Review。

状态：**DONE**（经 Task 06 修订）。
