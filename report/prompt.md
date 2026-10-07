# Lab1 Prompt 汇总

本文件汇总 Leader（Claude Code）发给子 Agent 的全部提示词。每个任务包含两部分：① 通过 Herdr 发送到 Agent 终端的调度消息原文；② 该消息要求 Agent 阅读执行的规格文件中 PROMPT / RELY / GUARANTEE / SPECIFICATION 原文（规格文件中 Agent Result、Verification 为事后回填的执行记录，未列入）。


## Task 01：构建内核并验证 make qemu 启动

### 调度消息

```text
你是 OS Lab1 的 Implementation/Test Agent。完整任务规格（PROMPT / RELY / GUARANTEE / SPECIFICATION）在文件 \\wsl$\Ubuntu-24.04\home\wesly\courses\os2026\report\ai_logs\lab1\task_01.md 中（WSL 路径 /home/wesly/courses/os2026/report/ai_logs/lab1/task_01.md）。请先完整阅读该文件，然后严格按其中的要求执行，不要修改 task_01.md 本身。完成后按 PROMPT 中“输出”一节的格式简短回复。
```

### 规格原文

执行者：Codex（Implementation + Test Agent，Herdr 面板 w9:p7）

#### PROMPT

你是 OS Lab1 的 Implementation/Test Agent。仓库在 WSL `Ubuntu-24.04` 的 `/home/wesly/courses/os2026`，当前分支 `lab1`，实验代码在 `code/`。你运行在 Windows 上，所有命令请通过 `wsl -d Ubuntu-24.04 -- bash -lc '<cmd>'` 在 WSL 内执行（复杂命令先写成 WSL 里的 .sh 脚本再执行，避免引号问题）。

任务：
1. 修复 `code/Makefile` 中 `gdb` 目标：本机没有 `riscv64-unknown-elf-gdb`，只有 `/usr/bin/gdb-multiarch`。只改 `gdb:` 目标里调用的程序名为 `gdb-multiarch`，其它行一律不动。
2. 在 `code/` 下执行 `make clean && make`，确认生成 `bin/kernel` 和 `bin/ucore.img`。
3. 运行 `make qemu`（它不会自己退出，用 `timeout 5 make qemu` 并把输出保存），确认内核输出 `(THU.CST) os is loading ...`。
4. 用 `riscv64-unknown-elf-nm` / `riscv64-unknown-elf-objdump -h` / `readelf -l` 验证：`kern_entry` 地址为 `0x80200000`；记录 `.text/.rodata/.data/.bss` 起始地址、`bootstack`、`bootstacktop`、`edata`、`end` 的地址；反汇编 `kern_entry` 的前几条指令（`objdump -d bin/kernel --start-address=0x80200000 --stop-address=0x80200010`）。

允许修改：仅 `code/Makefile` 的 gdb 目标那一行。
禁止：修改任何 .c/.S/.h/.ld 文件；git commit/push；删除仓库已有文件；安装软件或改系统配置。

输出：
- 把每条命令的原始输出分别保存到 `report/ai_logs/lab1/evidence/` 下：`t1_build.txt`、`t1_qemu.txt`、`t1_symbols.txt`（nm/objdump -h/readelf/反汇编）。
- `git diff code/Makefile` 保存为 `evidence/t1_makefile.diff`。
- 回复我一段简短结果（≤15 行）：每步成功/失败、关键地址、遇到的问题。不要编造任何未实际运行得到的输出。

#### RELY

- `code/Makefile`：`GCCPREFIX := riscv64-unknown-elf-`；`qemu:` 目标为 `qemu-system-riscv64 -machine virt -nographic -bios default -device loader,file=$(UCOREIMG),addr=0x80200000`；`debug:` 同上加 `-s -S`；`gdb:` 目标调用 `riscv64-unknown-elf-gdb -ex 'file bin/kernel' -ex 'set arch riscv:rv64' -ex 'target remote localhost:1234'`。
- `code/tools/kernel.ld`：`ENTRY(kern_entry)`，`BASE_ADDRESS = 0x80200000`，`.data` 前 `ALIGN(0x1000)`，提供 `etext/edata/end`。
- `code/kern/init/entry.S`：`kern_entry: la sp, bootstacktop; tail kern_init`；`.data` 段中 `.align PGSHIFT` 后 `bootstack: .space KSTACKSIZE`，`bootstacktop` 紧随其后。
- `code/kern/mm/memlayout.h`：`KSTACKPAGE 2`，`KSTACKSIZE (KSTACKPAGE * PGSIZE)`；`mmu.h`：`PGSIZE 4096`，`PGSHIFT 12`。
- `code/kern/init/init.c`：`kern_init` 清零 `[edata, end)`，`cprintf("%s\n\n", "(THU.CST) os is loading ...\n")`，然后 `while(1);`。
- 工具：RISC-V GCC 13.2.0、QEMU 8.2.2、gdb-multiarch 15.1 已安装。

#### GUARANTEE

- `make` 成功，无编译错误。
- `make qemu` 输出含 OpenSBI 横幅及 `(THU.CST) os is loading ...`。
- `kern_entry == 0x80200000`。
- Makefile 只有 gdb 那一行变化。
- 所有证据文件为真实命令输出。

#### SPECIFICATION

- Preconditions：分支 `lab1`，`code/` 为助教发布的原始 lab1 代码，工作区无其他改动。
- Postconditions：`code/bin/kernel`、`code/bin/ucore.img` 存在；evidence 目录下 4 个文件存在且非空；`git diff --stat` 仅显示 `code/Makefile` 1 行改动（以及新增的 evidence 文件）。
- Edge cases：
  - 若 `make` 失败，保存完整错误输出，停止并报告原因，不要擅自改源码。
  - 若 `timeout` 截断导致输出缺失，可适当延长到 10 秒。
  - QEMU 退出码 124（被 timeout 终止）属正常。
  - 若 `kern_entry` 不在 `0x80200000`，如实报告，不要修改链接脚本。


## Task 02：修复 Bug 1（make qemu 无内核输出）

### 调度消息

```text
下一个任务：Debug Agent。完整规格在 /home/wesly/courses/os2026/report/ai_logs/lab1/task_02.md（Windows 路径 \\wsl$\Ubuntu-24.04\home\wesly\courses\os2026\report\ai_logs\lab1\task_02.md）。请完整阅读后严格执行，不要修改该文件，完成后按其中“输出”格式简短回复。
```

### 规格原文

执行者：Codex（Debug Agent，Herdr 面板 w9:p7）

#### PROMPT

你是 OS Lab1 的 Debug Agent。仓库在 WSL `Ubuntu-24.04` 的 `/home/wesly/courses/os2026`（分支 `lab1`），代码在 `code/`。你在 Windows 上，所有命令通过 `wsl -d Ubuntu-24.04 -- bash -l <脚本>` 在 WSL 内执行；脚本请用 LF 换行写到 WSL 的 `/home/wesly/` 下（WSL 的 `/tmp` 与 Windows 侧路径映射不可靠，不要依赖）。

背景：见 `report/ai_logs/lab1/bugs.md` 的 Bug 1。`make qemu` 只打印 OpenSBI，`Domain0 Next Address` 为 0，内核未被执行。

任务：
1. 修改 `code/Makefile` 的 `qemu:` 与 `debug:` 两个目标：把 `-device loader,file=$(UCOREIMG),addr=0x80200000` 替换为 `-kernel $(UCOREIMG)`（`debug:` 保留 `-s -S`）。其他行不动，不要改 Task 01 已修改的 gdb 行。
2. `make clean && make`，然后 `timeout 10 make qemu`，保存输出到 `report/ai_logs/lab1/evidence/t2_qemu.txt`。
3. 验证：输出中 `Domain0 Next Address` 为 `0x0000000080200000`，并出现 `(THU.CST) os is loading ...`。
4. 用 gdb-multiarch 的批处理模式做一次最小确认：后台启动 `make debug`，再运行
   `gdb-multiarch -batch -ex 'file bin/kernel' -ex 'set arch riscv:rv64' -ex 'target remote localhost:1234' -ex 'b *0x80200000' -ex 'c' -ex 'i r pc' -ex 'x/4i $pc' -ex 'kill'`，
   输出保存为 `evidence/t2_gdb_check.txt`；结束后确保 QEMU 进程已退出（`pkill -f qemu-system-riscv64` 仅针对本任务启动的进程）。
5. 修复后的 Makefile 差异（相对助教原版 `/home/wesly/lab1x/lab1/Makefile`）保存为 `evidence/t2_makefile.diff`（用 `diff -u`）。

允许修改：仅 `code/Makefile` 的 `qemu:`、`debug:` 两个目标中的 loader 参数行。
禁止：修改 .c/.S/.h/.ld；git commit/push；安装软件；修改 `ai_logs` 下的 .md 文件。

输出：回复 ≤12 行，说明每步结果、`Domain0 Next Address` 的实际值、内核输出是否出现、GDB 是否在 0x80200000 命中。只报告真实运行结果。

#### RELY

- 原 `qemu:` 目标：`$(QEMU) -machine virt -nographic -bios default -device loader,file=$(UCOREIMG),addr=0x80200000`；`debug:` 同上加 `-s -S`。
- `UCOREIMG` 为 `bin/ucore.img`，由 `objcopy -O binary bin/kernel` 生成的 raw 镜像，链接基址 0x80200000（`tools/kernel.ld`）。
- QEMU 8.2.2 `-bios default` = OpenSBI v1.3 fw_dynamic；对 raw `-kernel` 镜像，QEMU 将其加载到固件结束地址向上 2 MiB 对齐处，并通过 fw_dynamic info 把该地址作为下一阶段入口传给 OpenSBI。
- Task 01 证据：`kern_entry = 0x80200000`。

#### GUARANTEE

- `make qemu` 输出 `(THU.CST) os is loading ...`。
- OpenSBI 报告 `Domain0 Next Address : 0x0000000080200000`。
- GDB 在 `0x80200000 <kern_entry>` 命中断点。
- Makefile 相对原版只有 gdb 行、qemu/debug 的 loader 行三处变化。

#### SPECIFICATION

- Preconditions：Task 01 完成；`code/Makefile` 中 gdb 行已改为 `gdb-multiarch`。
- Postconditions：`evidence/t2_qemu.txt`、`t2_gdb_check.txt`、`t2_makefile.diff` 存在且非空；没有遗留的 qemu 进程。
- Edge cases：
  - 若 `-kernel` 后 Next Address 不是 0x80200000，或内核仍无输出：恢复 Makefile 原样，保存输出并报告，不要尝试其他方案。
  - 若 1234 端口被占用，先报告占用进程，不要强杀未知进程。


## Task 03：练习 2 —— GDB 跟踪从加电到内核第一条指令

### 调度消息

```text
下一个任务：Test Agent，采集练习 1/2 的 GDB 数据。完整规格在 /home/wesly/courses/os2026/report/ai_logs/lab1/task_03.md（Windows 路径 \\wsl$\Ubuntu-24.04\home\wesly\courses\os2026\report\ai_logs\lab1\task_03.md）。请完整阅读后严格执行，不要修改该文件，完成后按其中“输出”格式回复。
```

### 规格原文

执行者：Codex（Test Agent，Herdr 面板 w9:p7）

#### PROMPT

你是 OS Lab1 的 Test Agent，负责采集练习 2（GDB 验证启动流程）和练习 1（entry.S）所需的真实调试数据。仓库在 WSL `Ubuntu-24.04` 的 `/home/wesly/courses/os2026`（分支 `lab1`），代码在 `code/`。所有命令通过 `wsl -d Ubuntu-24.04 -- bash -l <脚本>` 执行，脚本以 LF 换行写在 `/home/wesly/` 下。

方式：后台运行 `make debug`（QEMU 带 `-s -S` 暂停在复位处），用 `gdb-multiarch -batch -x <gdb脚本>` 连接 `localhost:1234` 执行下列阶段，每个阶段的 GDB 原始输出分别保存到 `report/ai_logs/lab1/evidence/`。每个阶段结束后 `kill` 并确保本任务启动的 QEMU 退出，下一阶段重新启动 `make debug`（这样每个阶段都从加电开始，互不干扰）。GDB 前置命令统一为：`set pagination off`、`file bin/kernel`、`set arch riscv:rv64`、`target remote localhost:1234`。

阶段 A —— 复位入口（`t3_a_reset.txt`）：
- 连接后立即：`i r pc`、`x/10i $pc`、`x/6gx 0x1000`（显示 MROM 及其后的数据字）。
- 连续 `si` 单步，每步后执行 `i r pc t0 a0 a1 a2`（可用 `display`），直到 pc 离开 0x1000 区域（预计约 5～8 步跳到 0x80000000）。
- 再执行一次 `x/2gx 0x80200000`，记录此刻（内核尚未运行）0x80200000 处是否已经有内核代码；同时用 `riscv64-unknown-elf-objdump -d bin/kernel --start-address=0x80200000 --stop-address=0x80200008` 的机器码作对比，写入同一文件末尾。

阶段 B —— OpenSBI 入口（`t3_b_opensbi.txt`）：
- `b *0x80000000`、`c`，命中后 `i r pc a0 a1 a2`、`x/12i $pc`。

阶段 C —— 观察内核加载时刻（`t3_c_watch.txt`）：
- 按指导书提示：`watch *(int *)0x80200000`，同时 `b *0x80200000` 作为兜底，然后 `c`。记录先触发的是 watchpoint 还是 breakpoint，以及触发时的 pc、`x/2gx 0x80200000`。若 watchpoint 从未触发，这是一个需要如实报告的观察结果。
- 为防止卡死，整个阶段用 `timeout 60` 包裹。

阶段 D —— 内核入口与练习 1（`t3_d_kernel.txt`）：
- `b *0x80200000`、`c`，命中后：`i r pc sp ra a0 a1`、`x/4i $pc`、`p/x &bootstack`、`p/x &bootstacktop`、`x/8gx $sp`（栈初始化前 sp 指向的内存）。
- `si` 逐条执行 `la sp, bootstacktop` 展开的两条指令，每条之后 `i r pc sp`。
- 再 `si` 执行 `tail kern_init` 对应的跳转指令，之后 `i r pc sp ra`、`bt`、`x/8i $pc`。
- `b *0x80200000` 之外再 `b kern_init` 也可以，用于确认 pc 进入 kern_init。
- 最后 `c` 前先 `b sbi_console_putchar`，命中后 `i r a0`、`bt`（看到 cprintf → cons_putc → sbi_console_putchar 调用链）；再执行到 `ecall` 前停一次并 `x/4i $pc`。

最后把四个阶段用到的 GDB 脚本原文保存为 `evidence/t3_gdb_scripts.txt`。

允许修改：只在 `report/ai_logs/lab1/evidence/` 下新增文件；可在 `/home/wesly/` 下创建临时脚本。
禁止：修改 `code/` 下任何文件；git commit/push；修改 `ai_logs` 下的 .md 文件；强杀不是本任务启动的进程。

输出：回复 ≤20 行：每个阶段是否成功、复位后执行的指令列表（地址 + 指令 + 关键寄存器变化）、跳转到 0x80000000 前 t0/a0/a1/a2 的值、阶段 C 的触发情况、阶段 D 中 sp 在 `la` 前后的值和 tail 后的 pc/ra。只写真实输出中的数据。

#### RELY

- Task 02 修复后的 Makefile：`debug:` = `qemu-system-riscv64 -machine virt -nographic -bios default -kernel bin/ucore.img -s -S`；`gdb:` 使用 `gdb-multiarch`。
- Task 01 符号：`kern_entry=0x80200000`，`bootstack=0x80201000`，`bootstacktop=0x80203000`（`KSTACKSIZE = 2*4096`）。
- `kern/init/entry.S`：`kern_entry: la sp, bootstacktop; tail kern_init`。
- `kern/init/init.c`：`kern_init()` 调用 `memset(edata,0,end-edata)` 与 `cprintf`。
- `kern/driver/console.c`：`cons_putc(c)` → `sbi_console_putchar((unsigned char)c)`；`libs/sbi.c`：`sbi_call` 以 `x17=type, x10..x12=args` 执行 `ecall`，`SBI_CONSOLE_PUTCHAR = 1`。
- QEMU virt 复位向量 0x1000（MROM），OpenSBI 位于 0x80000000。

#### GUARANTEE

- 四个阶段的证据文件 + 脚本文件存在且非空，内容为真实 GDB 输出。
- 阶段 A 能看到 0x1000 起的复位指令及跳到 0x80000000 的过程。
- 阶段 D 能看到 sp 被设置为 `bootstacktop`（预期 0x80203000）以及跳入 `kern_init`。
- 任务结束后无本任务遗留的 qemu 进程。

#### SPECIFICATION

- Preconditions：Task 02 通过（`make qemu` 能输出内核信息）；1234 端口空闲。
- Postconditions：`evidence/t3_a_reset.txt`、`t3_b_opensbi.txt`、`t3_c_watch.txt`、`t3_d_kernel.txt`、`t3_gdb_scripts.txt` 存在。
- Branch / Edge cases：
  - 若某阶段 GDB 连接失败，重试一次（先确认 QEMU 已启动 1 秒以上），仍失败则保存错误并继续下一阶段。
  - 阶段 C 若 60 秒内两者均未触发，记录超时。
  - `si` 进入 OpenSBI 后不必继续单步；阶段 A 一旦 pc 到达 0x80000000 即停止。
  - `x/8gx $sp` 若因地址无效报错，原样保存错误信息。


## Task 04：撰写实验报告与测试截图

### 调度消息

```text
下一个任务：Report Agent，生成测试截图并撰写实验报告。完整规格在 /home/wesly/courses/os2026/report/ai_logs/lab1/task_04.md（Windows 路径 \\wsl$\Ubuntu-24.04\home\wesly\courses\os2026\report\ai_logs\lab1\task_04.md）。请完整阅读后严格执行，不要修改该文件，完成后按其中“输出”格式回复。
```

### 规格原文

执行者：Codex（Report Agent，Herdr 面板 w9:p7）

#### PROMPT

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

#### RELY

- 指导书 lab1_5 报告要求：① 本章整体逻辑线（按什么顺序、围绕哪些功能逐步实现）；② 每个功能的核心函数或模块的理解；③ 本实验重要知识点与 OS 原理知识点的对应、含义、关系和差异；④ OS 原理中重要但本实验未对应的知识点；⑤ 回答所有练习问题。
- 练习 1 原题：阅读 kern/init/entry.S，说明 `la sp, bootstacktop` 完成了什么操作、目的是什么；`tail kern_init` 完成了什么操作、目的是什么。
- 练习 2 原题：用 GDB 跟踪 QEMU 模拟的 RISC-V 从加电到执行内核第一条指令（跳转到 0x80200000）的过程；RISC-V 硬件加电后最初执行的几条指令位于什么地址、主要完成了哪些功能；报告中简要记录调试过程、观察结果和答案。提示：复位地址 0x1000 → OpenSBI 初始化并（据指导书）加载内核到 0x80200000（可用 `watch *0x80200000`）→ `b *0x80200000` 验证。
- 指导书执行流：加电复位 → CPU 从 0x1000 进入 MROM → 跳转 0x80000000(OpenSBI) → 加载内核到 0x80200000 → entry.S → kern_init() → 输出信息。
- 代码事实：见 Task 01～03 的 RELY 与 Verification 章节。
- 证据文件：`report/ai_logs/lab1/evidence/` 下 t1_*、t2_*、t3_*。

#### GUARANTEE

- `report/report.md` 存在，章节与模板一致，覆盖 RELY 中五项报告要求和两道练习。
- 报告中的所有地址、寄存器值、指令与 evidence 文件一致。
- `report/images/` 下 6 张 PNG，内容是 evidence 原文的截取，正文全部引用。

#### SPECIFICATION

- Preconditions：Task 01～03 DONE；evidence 文件齐全。
- Postconditions：上述文件存在；`git status` 中 `code/`、`evidence/` 无新改动。
- Edge cases：
  - 模板中要求填写但本实验没有对应内容的项（如“测试结果统计/make grade”），写明“本实验无 grade 脚本（压缩包缺 tools/grade.sh），以 make qemu 输出与 GDB 观察作为验证”。
  - 字体缺失时退回 Consolas/默认字体，但必须保证中文可读。


## Task 05：Review 实验报告与代码改动

### 调度消息

```text
你是 OS Lab1 的 Review Agent。完整任务规格在文件 \\wsl$\Ubuntu-24.04\home\wesly\courses\os2026\report\ai_logs\lab1\task_05.md 中，请先完整阅读该文件，然后严格按其中要求执行审查，不要修改除 review_01.md 以外的任何文件。
```

### 规格原文

执行者：pi（Review Agent，deepseek-flash，Herdr 面板 w9:p8）

#### PROMPT

你是 OS Lab1 的 Review Agent，只做审查，不修改任何已有文件。仓库根目录（Windows 路径）：`\\wsl$\Ubuntu-24.04\home\wesly\courses\os2026`，以下路径均相对于它。

请阅读：
- 被审查对象：`report/report.md`、`report/images/*.png`（查看图片内容）、`code/Makefile`。
- 依据：`实验报告模板.md`；`report/ai_logs/lab1/task_01.md`～`task_04.md`（其中 RELY / Verification 是已核实事实）；`report/ai_logs/lab1/bugs.md`；`report/ai_logs/lab1/evidence/` 下全部 txt（原始命令输出，唯一事实来源）；助教原版 Makefile 位于 `\\wsl$\Ubuntu-24.04\home\wesly\lab1x\lab1\Makefile`。

逐项检查：
1. 报告中每个地址、寄存器值、指令、输出文字是否能在 evidence 中找到原文；列出所有对不上的地方（报告原句 + evidence 文件与实际内容）。
2. 是否回答了练习 1 的两个问题（操作 + 目的）和练习 2 的全部问题（最初几条指令的地址、功能、调试过程、观察结果）。
3. 是否覆盖指导书五项报告要求：整体逻辑线；核心函数/模块理解；实验知识点与 OS 原理对应（含义、关系、差异）；OS 原理中重要但本实验未对应的知识点；练习答案。
4. 章节是否与模板一致；有无残留的模板占位符（如 `[目的1：...]`、`学号1-姓名1` 之外的示例文字）。注意：`学号-姓名（待填写）` 是有意保留的，不算问题。
5. 每张图片是否在正文中被引用，图片内容是否与其说明一致，图片是否为 evidence 原文截取。
6. `code/Makefile` 相对助教原版是否只有 3 行变化（gdb 程序名、qemu/debug 的 `-kernel`），改动理由在报告中是否解释正确。
7. 有无夸大、无依据或与指导书事实相冲突却未说明的表述。

输出：把审查结果写入新文件 `report/ai_logs/lab1/review_01.md`，格式：
```
# Review 01
#### 结论：PASS / NEEDS_FIX
#### 问题清单
- [严重|一般|建议] 位置（报告章节或行）：问题描述；依据（evidence 文件 + 原文）；建议修改
#### 已核对通过的项目
```
只列有依据的问题，不要为凑数提问题。写完后在终端只回复一行：`REVIEW_DONE <结论> <问题数>`。

#### RELY

- 已核实事实见 task_01～task_04 的 Verification；evidence 为原始输出。
- 指导书练习原题与报告要求见 task_04.md 的 RELY。

#### GUARANTEE

- `review_01.md` 存在，结论明确，每个问题都带 evidence 依据。
- 未修改任何其他文件。

#### SPECIFICATION

- Preconditions：Task 04 完成。
- Postconditions：仅新增 `report/ai_logs/lab1/review_01.md`。
- Edge cases：无法打开图片时，在 review 中注明“未能查看图片”，不要猜测内容。


## Task 06：按 Review 01 修订报告

### 调度消息

```text
下一个任务：按 Review 修订报告。完整规格在 /home/wesly/courses/os2026/report/ai_logs/lab1/task_06.md（Windows 路径 \\wsl$\Ubuntu-24.04\home\wesly\courses\os2026\report\ai_logs\lab1\task_06.md）。请完整阅读后严格执行，不要修改该文件，完成后按其中“输出”格式回复。
```

### 规格原文

执行者：Codex（Report Agent，Herdr 面板 w9:p7）

#### PROMPT

你是 OS Lab1 的 Report Agent。请按 Review 结果修订 `report/report.md`（仓库 `/home/wesly/courses/os2026`，Windows 路径 `\\wsl$\Ubuntu-24.04\home\wesly\courses\os2026`）。先完整阅读 `report/ai_logs/lab1/review_01.md`。

只做以下 4 处修改，其他内容不动：
1. Review 问题 1（第 266 行附近“报告生成前后对禁止修改的已有文件做内容哈希核对”）：改为有据可查的表述——Task 02、Task 03 的执行脚本用 sha256 清单校验仓库原有文件未被改动；删掉“报告生成前后”的说法。同一段最后的“本任务不执行 commit 或 push”改为：“代码与报告最终提交到仓库 `lab1` 分支。”
2. Review 问题 2（第 258 行表格“修复后 timeout 10 make qemu”一行）：把“timeout 退出码 124 为正常”改为“被 `timeout` 终止（QEMU 日志 `terminating on signal 15 ... (timeout)`），属预期结束方式”。
3. 第二章 AI 工具表 pi 那一行的备注改为：“Review：审查报告与代码改动，见 [review_01.md](ai_logs/lab1/review_01.md)”。
4. 第六章“AI 协作开发的经验”列表：把 Task 04 条目中“pi 被指定承担 Review，但目前没有可引用的该轮复核结果。”删除，并在 Task 04 之后新增两条：
   - `[Task 05](ai_logs/lab1/task_05.md)：Review Agent（pi）逐项对照 evidence 审查报告，结论 NEEDS_FIX，提出 2 项表述问题（无记录的哈希核对说法、借用 Task 01 的退出码），未发现数据错误；见 [review_01.md](ai_logs/lab1/review_01.md)。`
   - `[Task 06](ai_logs/lab1/task_06.md)：Report Agent 按 Review 修订上述表述。`

允许修改：仅 `report/report.md`。禁止：修改其他文件；git commit/push。
输出：回复 ≤6 行，列出 4 处修改是否完成。

#### RELY

- `report/ai_logs/lab1/review_01.md` 问题清单与建议。
- `evidence/t2_qemu.txt` 末行：`qemu-system-riscv64: terminating on signal 15 from pid 467 (timeout)`。
- 哈希校验依据：Review 01 问题 1 依据第 2 点（`os2026-t2-run.sh`、`os2026-t3-run.sh` 中的 sha256 清单与 `PASS: all original repository files unchanged`）。

#### GUARANTEE

- 4 处修改全部完成，报告其余文字不变。

#### SPECIFICATION

- Preconditions：Review 01 已完成。
- Postconditions：`report.md` 中不再出现“报告生成前后”“退出码 124”“未见该轮 Review 结果”“目前没有可引用的该轮复核结果”“本任务不执行 commit 或 push”。
