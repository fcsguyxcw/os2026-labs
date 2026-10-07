# Task 03：练习 2 —— GDB 跟踪从加电到内核第一条指令

执行者：Codex（Test Agent，Herdr 面板 w9:p7）

## PROMPT

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

## RELY

- Task 02 修复后的 Makefile：`debug:` = `qemu-system-riscv64 -machine virt -nographic -bios default -kernel bin/ucore.img -s -S`；`gdb:` 使用 `gdb-multiarch`。
- Task 01 符号：`kern_entry=0x80200000`，`bootstack=0x80201000`，`bootstacktop=0x80203000`（`KSTACKSIZE = 2*4096`）。
- `kern/init/entry.S`：`kern_entry: la sp, bootstacktop; tail kern_init`。
- `kern/init/init.c`：`kern_init()` 调用 `memset(edata,0,end-edata)` 与 `cprintf`。
- `kern/driver/console.c`：`cons_putc(c)` → `sbi_console_putchar((unsigned char)c)`；`libs/sbi.c`：`sbi_call` 以 `x17=type, x10..x12=args` 执行 `ecall`，`SBI_CONSOLE_PUTCHAR = 1`。
- QEMU virt 复位向量 0x1000（MROM），OpenSBI 位于 0x80000000。

## GUARANTEE

- 四个阶段的证据文件 + 脚本文件存在且非空，内容为真实 GDB 输出。
- 阶段 A 能看到 0x1000 起的复位指令及跳到 0x80000000 的过程。
- 阶段 D 能看到 sp 被设置为 `bootstacktop`（预期 0x80203000）以及跳入 `kern_init`。
- 任务结束后无本任务遗留的 qemu 进程。

## SPECIFICATION

- Preconditions：Task 02 通过（`make qemu` 能输出内核信息）；1234 端口空闲。
- Postconditions：`evidence/t3_a_reset.txt`、`t3_b_opensbi.txt`、`t3_c_watch.txt`、`t3_d_kernel.txt`、`t3_gdb_scripts.txt` 存在。
- Branch / Edge cases：
  - 若某阶段 GDB 连接失败，重试一次（先确认 QEMU 已启动 1 秒以上），仍失败则保存错误并继续下一阶段。
  - 阶段 C 若 60 秒内两者均未触发，记录超时。
  - `si` 进入 OpenSBI 后不必继续单步；阶段 A 一旦 pc 到达 0x80000000 即停止。
  - `x/8gx $sp` 若因地址无效报错，原样保存错误信息。

## Agent Result

Codex 回复（2026-10-07 15:00）摘要：A/B/C/D 四阶段完成，GDB 退出码均为 0。
- A：`0x1000 auipc t0,0`→t0=0x1000；`0x1004 addi a2,t0,40`→a2=0x1028；`0x1008 csrr a0,mhartid`→a0=0；`0x100c ld a1,32(t0)`→a1=0x87e00000；`0x1010 ld t0,24(t0)`→t0=0x80000000；`0x1014 jr t0`→pc=0x80000000。内核运行前 0x80200000 已有与 objdump 一致的机器码（00003117、00010113）。
- B：命中 OpenSBI 入口 0x80000000。
- C：先命中 0x80200000 断点，watchpoint 未触发。
- D：`la` 前 sp=0x80046eb0，`auipc` 后 sp=0x80203000，`mv` 后不变；tail 后 pc=0x8020000a（kern_init）、ra=0x8000ae9a（未改变）；调用栈 kern_init→cprintf→vcprintf→vprintfmt→cputch→cons_putc→sbi_console_putchar；`ecall` 前 a0=0x28（'('）、a7=1。

## Verification

Leader 逐文件复核 `t3_a_reset.txt`、`t3_b_opensbi.txt`、`t3_c_watch.txt`、`t3_d_kernel.txt`，上述数值均与原始输出一致。补充观察：`x/6gx 0x1000` 中 0x1018 处为 0x80000000（OpenSBI 入口）、0x1020 处为 0x87e00000（设备树地址）、0x1028 处为 0x4942534f（ASCII "OSBI"，fw_dynamic_info 魔数），说明 a2 指向传给 OpenSBI 的 fw_dynamic_info 结构。`bootstacktop` 与 `.sdata` 起点重合，故 GDB 将 0x80203000 符号化为 `<SBI_CONSOLE_PUTCHAR>`，栈向低地址增长，不冲突。

状态：**DONE**。
