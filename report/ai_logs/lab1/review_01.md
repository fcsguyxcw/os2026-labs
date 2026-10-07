# Review 01

审查对象：`report/report.md`、`report/images/*.png`（6 张，已逐张查看）、`code/Makefile`
对照依据：`实验报告模板.md`、`report/ai_logs/lab1/task_01.md`～`task_05.md`、`report/ai_logs/lab1/bugs.md`、`report/ai_logs/lab1/evidence/` 全部 txt、助教原版 `\\wsl$\Ubuntu-24.04\home\wesly\lab1x\lab1\Makefile`
审查者：pi（Review Agent，只读；本次除新增本文件外未改动任何文件）

## 结论：NEEDS_FIX

问题共 2 项（1 一般、1 建议），均属"过程/证据表述"层面；报告中的地址、寄存器值、指令与输出文字未发现与 evidence 冲突之处，两道练习与五项报告要求均已覆盖。

## 问题清单

- [一般] 位置：`report/report.md` 第 266 行（"五、测试与验证" Git 边界段）："报告生成前后对禁止修改的已有文件做内容哈希核对"
  - 问题描述：这句话描述了一次"哈希核对"过程，但在指定的唯一事实来源（`ai_logs/lab1/` 与 `evidence/`）中找不到任何记录；且实际存在的哈希校验发生在报告生成之前的 Task 02、Task 03 执行阶段，不是"报告生成前后"。
  - 依据：
    1. 对 `report/ai_logs/lab1/` 全文检索 `hash`/`哈希`/`sha256`，只在 `report.md` 该行本身命中；`evidence/` 目录下 12 个文件（`t1_build.txt`、`t1_makefile.diff`、`t1_qemu.txt`、`t1_symbols.txt`、`t2_gdb_check.txt`、`t2_makefile.diff`、`t2_qemu.txt`、`t3_a_reset.txt`、`t3_b_opensbi.txt`、`t3_c_watch.txt`、`t3_d_kernel.txt`、`t3_gdb_scripts.txt`）中没有任何哈希清单或校验输出。
    2. 实际哈希校验位于 WSL 侧执行脚本：`/home/wesly/os2026-t2-run.sh` 中 `manifest = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() ...}` 与 `os2026-t3-run.sh` 中的 `snapshot_dir`/`manifest.json`、末尾 `print('PASS: all original repository files unchanged; ...')`。这两次校验分别属于 Task 02（14:57）与 Task 03（15:00），即报告生成（`report.md` 时间戳 15:08）之前；WSL `/home/wesly`、`/tmp` 下不存在 Task 04 的哈希校验脚本或结果。
    3. `task_04.md` 的 "## Agent Result" 与 "## Verification" 两节仍为"（待回填）"，无法据其确认报告生成阶段做过哈希核对。
  - 建议修改：改为有据可查的表述，例如"Task 02、Task 03 的执行脚本用 sha256 清单校验了仓库原有文件未被改动（`PASS: all original repository files unchanged`）"；若确实在报告生成阶段也做了核对，请把该步的哈希输出补入 `evidence/` 后再引用，否则直接删除这半句。

- [建议] 位置：`report/report.md` 第 258 行（"五、测试与验证"表格"修复后 `timeout 10 make qemu`"一行）："timeout 退出码 124 为正常"
  - 问题描述："实际结果"一栏引用的证据文件 `t2_qemu.txt` 内没有退出码，"124"这一数值取自 Task 01 的运行（同一命令形式），并非本次修复后运行的实测记录；把推断值与实测值写在同一条记录里，容易被当成实测结论。
  - 依据：
    1. `evidence/t2_qemu.txt` 全文最后两行为 `(THU.CST) os is loading ...` 与 `qemu-system-riscv64: terminating on signal 15 from pid 467 (timeout)`，无退出码；`Task 02` 的 Agent Result 与 Verification 也未记录退出码。
    2. 数值 `124` 只出现在 `task_01.md`：SPECIFICATION 的"QEMU 退出码 124（被 timeout 终止）属正常"，以及 Agent Result 的"退出码 124（timeout）"，对象是 Task 01 的两次失败运行。
  - 建议修改：改为"被 `timeout` 终止（`timeout` 超时终止子进程时返回 124，故不作为失败判据）"，或把修复后运行的实测退出码补入 `evidence/t2_qemu.txt` 后再引用。
  - 附注：同一段中"四个 GDB 阶段退出码均为 0"有依据（`task_03.md` Agent Result："A/B/C/D 四阶段完成，GDB 退出码均为 0"），不计为问题。

## 已核对通过的项目

### 1. 地址、寄存器值、指令、输出文字与 evidence 的逐项核对（全部命中）

- 链接与符号：`.text`/`kern_entry` `0x80200000` 大小 `0x4c0`、`.rodata` `0x802004c0` 大小 `0x270`、`.data`/`bootstack` `0x80201000` 大小 `0x2000`、`.sdata`/`bootstacktop` `0x80203000` 大小 `0x8`、`edata`/`end` `0x80203008`、`SBI_CONSOLE_PUTCHAR` `0x80203000`、`kern_init` `0x8020000a`，均见 `t1_symbols.txt`（nm 与 `objdump -h` 原文）。
- ELF 入口 `0x80200000` 及两个 LOAD 段起点 `0x80200000`、`0x80201000`，见 `t1_symbols.txt` 的 readelf 原文。
- entry 反汇编 `auipc sp,0x3` / `mv sp,sp` / `j 0x8020000a <kern_init>`，见 `t1_symbols.txt`、`t2_gdb_check.txt`、`t3_a_reset.txt`、`t3_d_kernel.txt`。
- Bug 1 相关：`Domain0 Next Address : 0x0000000000000000`（`t1_qemu.txt`）与修复后 `0x0000000080200000`、`(THU.CST) os is loading ...`（`t2_qemu.txt`），报告引用一致。
- 练习 1：`sp` 由 `0x80046eb0` 变 `0x80203000`、`ra` 保持 `0x8000ae9a`、`pc=0x8020000a`、`$1 = 0x80201000`/`$2 = 0x80203000`，见 `t3_d_kernel.txt`；`KSTACKPAGE 2`、`PGSIZE 4096` 见 `code/kern/mm/memlayout.h`、`code/kern/mm/mmu.h`（本机复核），故 8 KB 结论正确。
- 练习 2：`0x1000 auipc t0,0x0`→`t0=0x1000`；`0x1004 addi a2,t0,40`→`a2=0x1028`；`0x1008 csrr a0,mhartid`→`a0=0`；`0x100c ld a1,32(t0)`→`a1=0x87e00000`；`0x1010 ld t0,24(t0)`→`t0=0x80000000`；`0x1014 jr t0`→`pc=0x80000000`，逐条见 `t3_a_reset.txt`。MROM 数据 `0x1018`=`0x0000000080000000`、`0x1020`=`0x0000000087e00000`、`0x1028`=`0x000000004942534f` 同见该文件；`0x4942534f` 按小端即 ASCII "OSBI"，与 `task_03.md` Verification 的解释一致。
- OpenSBI 入口：`add s0,a0,zero`/`add s1,a1,zero`/`add s2,a2,zero`/`jal 0x80000580` 及随后 `a6/a7` 分支，见 `t3_b_opensbi.txt` 的 `x/12i`。
- 阶段 C：`Hardware watchpoint 1: *(int *)0x80200000` 已设置，实际先停在 `Breakpoint 2, kern_entry () at kern/init/entry.S:7`，见 `t3_c_watch.txt`；报告"先断点、watchpoint 未触发"与"不能仅凭未触发推断原因"的表述与原始输出一致。阶段 A 在 `pc=0x80000000` 时 `x/2gx 0x80200000` 已得 `0x0001011300003117 0x051300003517a009`（`t3_a_reset.txt`、`t3_c_watch.txt`），支撑报告中"预加载"的解释，且报告明确写"指导书所述'SBI 加载内核'不能原样当作本机的观察结果"。
- 字符输出链：`kern_init → cprintf → vcprintf → vprintfmt → cputch → cons_putc → sbi_console_putchar`，及 `ecall` 前 `pc=0x8020048a`、`a0=0x28`、`a1=a2=0`、`a7=1`，见 `t3_d_kernel.txt` 的 `bt` 与寄存器原文（含 `sbi_call` 内联于 `sbi_console_putchar+18` 的说明）；`sbi_call` 的 `x17/x10..x12/ecall` 与 `SBI_CONSOLE_PUTCHAR = 1` 亦经 `code/libs/sbi.c` 复核。
- GDB 命令块与 `t3_gdb_scripts.txt` 中各阶段脚本一致（阶段 A 用 while 循环单步、阶段 C/D 的 `watch`、`tbreak *0x8020048a`、`kill` 等）。

### 2. 练习覆盖

- 练习 1：`la sp, bootstacktop` 的操作（置 `sp` 为栈顶、实际两条指令及 `auipc` 的偏移计算）与目的（为 C 代码建立独立内核栈、栈向低地址增长）都回答了；`tail kern_init` 的操作（`j` 不写 `ra`、`ra` 仍为 `0x8000ae9a`）与目的（转入 `noreturn` 的 C 入口，无需返回点）都回答了。
- 练习 2：最初几条指令的地址（`0x1000` MROM）、每条的功能与寄存器结果、调试过程（GDB 前置命令与四阶段命令）、观察结果（六步离开 MROM、命中 `0x80000000`、watchpoint 未触发、最终进入 `0x8020000a`）均已给出，并有"不要把 `x/10i` 解出的 `unimp` 当作实际执行指令"的说明。

### 3. 指导书五项报告要求

① 整体逻辑线（"三、实验整体逻辑分析"2.1/2.2）；② 核心函数/模块理解（`kern_entry`、`kern_init`、`cprintf/vcprintf/vprintfmt/cputch/cons_putc/sbi_console_putchar/sbi_call`）；③ 重要知识点与 OS 原理的含义/关系/差异（"六、实验总结与收获"表格六行）；④ OS 原理中重要但本实验未对应的知识点（调度、上下文切换、用户态隔离与系统调用分发、页表与虚拟内存、同步互斥、文件系统、完整中断/异常处理）；⑤ 两道练习答案。五项均覆盖。

### 4. 章节与占位符

- 章节与 `实验报告模板.md` 一致：实验基本信息 / 小组分工 / 一、实验目的 / 二、实验环境 / 三（含 2.1、2.2）/ 四（功能模块：模块功能描述、最终提示词、实现迭代过程、一/二/三次迭代、练习 1、练习 2、Challenge）/ 五、测试与验证 / 六、实验总结与收获（对操作系统的理解、AI 协作开发的经验）。模板中保留的 `### 2.1`、`##### 第二次迭代（如有）` 等标题属模板原有写法，Task 04 要求保留章节标题，不计为问题。
- 未发现残留的示例/占位文字：无 `[目的1：...]`、无 `学号1-姓名1`、无 `<!--` 注释、无 `YYYY-MM-DD`、无 `Lab X`、无 `test_result.png`、无"练习如何分工？"等模板提示句；`学号-姓名（待填写）` 为有意保留（Task 05 已说明）。
- "五、测试与验证"用表格 + 正文引用 6 张图替代模板的 `test_result.png` 占位，并写明"本实验无 grade 脚本（压缩包缺 `tools/grade.sh`）"；本机复核 `code/tools/` 只有 `function.mk`、`kernel.ld`，`Makefile` 第 193 行确有 `grade:` 目标但报告中未声称执行过 `make grade`，表述正确。

### 5. 图片

- 6 张图均在正文被引用：`images/01_make_qemu.png`（§2.2 第二次迭代）、`05_kernel_entry.png`（练习 1）、`02_reset_mrom.png`、`03_opensbi_entry.png`、`04_watch_vs_break.png`、`06_putchar_ecall.png`（练习 2）。
- 已逐张打开查看，内容与图注一致：图 1 含 `Domain0 Next Address : 0x0000000080200000` 与 `(THU.CST) os is loading ...`；图 2 含 `x/10i $pc`、`x/6gx 0x1000`、`jr t0` 后 `pc=0x80000000` 及 objdump 对照；图 3 含断点命中与 `x/12i` 的 12 条指令；图 4 含硬件 watchpoint 设置、`Breakpoint 2, kern_entry` 命中与入口机器码；图 5 含 `sp=0x80046eb0→0x80203000`、`$1/$2`、`tail` 后 `pc=0x8020000a`；图 6 含 `sbi_console_putchar` 调用栈与 `ecall` 前 `a0=0x28`、`a7=1`。
- 每张图顶部都有灰色来源行"来源：evidence/xxx.txt（终端输出渲染）"；文字与 `evidence/` 原文逐字一致（`render_images.py` 的 `selections()` 只做行切片并用 `assert all(line in original for line in lines)` 校验，无改写），符合"只裁剪片段"的要求。图 2 为同一文件多段切片拼接，属裁剪不属改写。

### 6. `code/Makefile` 改动

- 本机执行 `diff -u /home/wesly/lab1x/lab1/Makefile courses/os2026/code/Makefile` 与 `diff -r --brief code lab1x/lab1`：除 `Makefile` 外代码树无其他差异（仅多出构建产物 `bin/`、`obj/`）。
- 相对助教原版确为 3 行变化：`gdb:` 的 `riscv64-unknown-elf-gdb`→`gdb-multiarch`；`qemu:` 与 `debug:` 的 `-device loader,file=$(UCOREIMG),addr=0x80200000`→`-kernel $(UCOREIMG)`（`debug:` 保留 `-s -S`），与 `evidence/t2_makefile.diff`、`t2-run.sh` 中的断言一致。
- 报告对改动理由的解释正确且已划界：`-device loader` 只把镜像放入内存、不传递下一阶段入口，故 `Domain0 Next Address` 为 0；`-kernel` 把镜像加载到固件之后 2 MiB 对齐处并把该地址经 fw_dynamic info 传给 OpenSBI；旧版 QEMU 4.1.1/OpenSBI v0.4 行为被明确标注为"据指导书及 Leader 记录""不是本次独立实测结论"。

### 7. 夸大/无依据/与事实冲突未说明的表述

- 未发现虚报：报告明确写了"本实验无需补写 C 函数"、"`edata=end`，清零长度为 0，不能据此声称已经测试了非空 BSS 的清零过程"、"没有继续逐条跟踪所有固件初始化函数，不能把未观察的内部调用细节写成实测"、"本实验没有进程切换，也没有每进程内核栈管理"、"本实验的启动输出不能作为这些功能已经实现的证明"、"未见该轮 Review 结果"，与 evidence 及 `task_01`～`task_04` 的记录相符。
- Git 边界部分除上述第 1 项外均可复核：本机 `git status --short` 现为 `?? code/`、`?? report/`，与"`code/` 与 `report/` 尚未被跟踪"一致；`git log` 最近提交为 `c23270f Add course report template`，分支为 `lab1`，与"不执行 commit 或 push"一致；`t1_makefile.diff` 的头部 `a/tmp/os2026-task01.rnTgxw/Makefile.before` 与"no-index diff"的说法一致。
