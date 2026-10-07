# Lab1 Bug 记录

## Bug 1：make qemu 无内核输出

- 现象：`make qemu` 只打印 OpenSBI 横幅，没有 `(THU.CST) os is loading ...`。
- 失败命令：`cd code && timeout 10 make qemu`（证据 `evidence/t1_qemu.txt`）。
- 原因（Leader 分析）：本机 QEMU 8.2.2 的 `-bios default` 是 OpenSBI v1.3 的 fw_dynamic 固件，下一阶段入口地址由 QEMU 通过 dynamic info 传入。Makefile 只用 `-device loader,file=ucore.img,addr=0x80200000` 把镜像放进内存，没有用 `-kernel`，QEMU 便把下一阶段地址设为 0，OpenSBI 日志中 `Domain0 Next Address : 0x0000000000000000` 印证了这一点。指导书基于 QEMU 4.1.1 + OpenSBI v0.4，该版本固件固定跳转到 0x80200000，因此原 Makefile 在课程环境可用。
- 修复方案：将 `qemu`/`debug` 目标中的 `-device loader,file=$(UCOREIMG),addr=0x80200000` 替换为 `-kernel $(UCOREIMG)`；QEMU 会把 raw 镜像加载到固件之后 2 MiB 对齐处（0x80200000）并把该地址告诉 OpenSBI。备选方案（自行编译 QEMU 4.1.1）需要改动系统环境，未采用。
- 修复执行：Task 02（Codex，Debug Agent）。
- 修复后验证：`evidence/t2_qemu.txt` 中 `Domain0 Next Address : 0x0000000080200000`，并输出 `(THU.CST) os is loading ...`；`evidence/t2_gdb_check.txt` 中 GDB 在 `0x80200000 <kern_entry>` 命中。已修复。
