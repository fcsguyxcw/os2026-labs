# Task 01：构建内核并验证 make qemu 启动

执行者：Codex（Implementation + Test Agent，Herdr 面板 w9:p7）

## PROMPT

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

## RELY

- `code/Makefile`：`GCCPREFIX := riscv64-unknown-elf-`；`qemu:` 目标为 `qemu-system-riscv64 -machine virt -nographic -bios default -device loader,file=$(UCOREIMG),addr=0x80200000`；`debug:` 同上加 `-s -S`；`gdb:` 目标调用 `riscv64-unknown-elf-gdb -ex 'file bin/kernel' -ex 'set arch riscv:rv64' -ex 'target remote localhost:1234'`。
- `code/tools/kernel.ld`：`ENTRY(kern_entry)`，`BASE_ADDRESS = 0x80200000`，`.data` 前 `ALIGN(0x1000)`，提供 `etext/edata/end`。
- `code/kern/init/entry.S`：`kern_entry: la sp, bootstacktop; tail kern_init`；`.data` 段中 `.align PGSHIFT` 后 `bootstack: .space KSTACKSIZE`，`bootstacktop` 紧随其后。
- `code/kern/mm/memlayout.h`：`KSTACKPAGE 2`，`KSTACKSIZE (KSTACKPAGE * PGSIZE)`；`mmu.h`：`PGSIZE 4096`，`PGSHIFT 12`。
- `code/kern/init/init.c`：`kern_init` 清零 `[edata, end)`，`cprintf("%s\n\n", "(THU.CST) os is loading ...\n")`，然后 `while(1);`。
- 工具：RISC-V GCC 13.2.0、QEMU 8.2.2、gdb-multiarch 15.1 已安装。

## GUARANTEE

- `make` 成功，无编译错误。
- `make qemu` 输出含 OpenSBI 横幅及 `(THU.CST) os is loading ...`。
- `kern_entry == 0x80200000`。
- Makefile 只有 gdb 那一行变化。
- 所有证据文件为真实命令输出。

## SPECIFICATION

- Preconditions：分支 `lab1`，`code/` 为助教发布的原始 lab1 代码，工作区无其他改动。
- Postconditions：`code/bin/kernel`、`code/bin/ucore.img` 存在；evidence 目录下 4 个文件存在且非空；`git diff --stat` 仅显示 `code/Makefile` 1 行改动（以及新增的 evidence 文件）。
- Edge cases：
  - 若 `make` 失败，保存完整错误输出，停止并报告原因，不要擅自改源码。
  - 若 `timeout` 截断导致输出缺失，可适当延长到 10 秒。
  - QEMU 退出码 124（被 timeout 终止）属正常。
  - 若 `kern_entry` 不在 `0x80200000`，如实报告，不要修改链接脚本。

## Agent Result

Codex 回复（2026-10-07 14:54）摘要：
- Makefile：仅 `gdb:` 目标程序名改为 `gdb-multiarch`。
- 构建成功，`bin/kernel`、`bin/ucore.img` 非空。
- `make qemu`：5 秒、10 秒两次均只输出 OpenSBI 横幅，**没有** `(THU.CST) os is loading ...`；退出码 124（timeout）。
- 地址：`kern_entry=0x80200000`；`.text=0x80200000`、`.rodata=0x802004c0`、`.data=0x80201000`；`bootstack=0x80201000`、`bootstacktop=0x80203000`、`edata=end=0x80203008`（ELF 无独立 `.bss` 段）。
- 入口反汇编：`auipc sp, ...` / `mv sp, sp`（`la` 伪指令展开）/ 跳转 `kern_init`。
- 偏差：`code/`、`report/` 尚未被 git 跟踪，diff 证据用 `git diff --no-index` 生成。

## Verification

Leader 复核 `evidence/t1_qemu.txt`：OpenSBI v1.3 打印 `Domain0 Next Address : 0x0000000000000000`，即 OpenSBI 准备跳到 0x0 而不是 0x80200000，内核从未执行。`t1_makefile.diff` 确认仅 1 行改动。

状态：构建与地址验证 **DONE**；启动输出 **FAILED** → 见 `bugs.md` Bug 1，由 Task 02 修复。
