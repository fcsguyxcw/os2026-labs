# Task 02：修复 Bug 1（make qemu 无内核输出）

执行者：Codex（Debug Agent，Herdr 面板 w9:p7）

## PROMPT

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

## RELY

- 原 `qemu:` 目标：`$(QEMU) -machine virt -nographic -bios default -device loader,file=$(UCOREIMG),addr=0x80200000`；`debug:` 同上加 `-s -S`。
- `UCOREIMG` 为 `bin/ucore.img`，由 `objcopy -O binary bin/kernel` 生成的 raw 镜像，链接基址 0x80200000（`tools/kernel.ld`）。
- QEMU 8.2.2 `-bios default` = OpenSBI v1.3 fw_dynamic；对 raw `-kernel` 镜像，QEMU 将其加载到固件结束地址向上 2 MiB 对齐处，并通过 fw_dynamic info 把该地址作为下一阶段入口传给 OpenSBI。
- Task 01 证据：`kern_entry = 0x80200000`。

## GUARANTEE

- `make qemu` 输出 `(THU.CST) os is loading ...`。
- OpenSBI 报告 `Domain0 Next Address : 0x0000000080200000`。
- GDB 在 `0x80200000 <kern_entry>` 命中断点。
- Makefile 相对原版只有 gdb 行、qemu/debug 的 loader 行三处变化。

## SPECIFICATION

- Preconditions：Task 01 完成；`code/Makefile` 中 gdb 行已改为 `gdb-multiarch`。
- Postconditions：`evidence/t2_qemu.txt`、`t2_gdb_check.txt`、`t2_makefile.diff` 存在且非空；没有遗留的 qemu 进程。
- Edge cases：
  - 若 `-kernel` 后 Next Address 不是 0x80200000，或内核仍无输出：恢复 Makefile 原样，保存输出并报告，不要尝试其他方案。
  - 若 1234 端口被占用，先报告占用进程，不要强杀未知进程。

## Agent Result

Codex 回复（2026-10-07 14:58）摘要：qemu、debug 两处改为 `-kernel $(UCOREIMG)`（保留 `-s -S` 与 gdb 行修改）；构建成功；`Domain0 Next Address` = `0x0000000080200000`；出现 `(THU.CST) os is loading ...`；GDB 在 `kern_entry`（pc=0x80200000）命中；无遗留 QEMU 进程。

## Verification

Leader 复核：`evidence/t2_qemu.txt` 第 40 行 `Domain0 Next Address : 0x0000000080200000`、第 57 行 `(THU.CST) os is loading ...`；`t2_gdb_check.txt` 显示 `Breakpoint 1, kern_entry () at kern/init/entry.S:7`，`x/4i` 为 `auipc sp,0x3` / `mv sp,sp` / `j 0x8020000a <kern_init>`；`t2_makefile.diff` 相对原版仅 3 行变化；`pgrep` 无 qemu 进程。

注：QEMU 输出含 CR 字符，普通 grep 会把文件当二进制，需 `grep -a`。

状态：**DONE**。
