# Leader 调试记录（分支任务，2026-10-24）

## 1. 自动化双 gdb 链路 tracer bullet

脚本：`/Users/wesley/大三上课程/操作系统/tmp/lab2-debug/{run.sh,qemu_tracer.gdb,ucore_tracer.gdb}`。一个容器内：QEMU 4.1.1 `-s -S` 后台启动 → `gdb -batch -p <QEMU PID>` 设条件断点并 continue → `riscv64-unknown-elf-gdb -batch` 连 1234 端口让 ucore 继续运行。

断点：`break get_physical_address if (addr >> 12) == 0xffffffffc0207`（`pages` 数组首页，开启分页后才第一次访问）。命中结果见 `t7_tracer_qemu_gdb.txt`：
- 访问地址 `addr = 18446744072637935624 = 0xffffffffc0207008`，即 `pages[0].flags`（`page_init` 中 `SetPageReserved` 的读-改-写，先读，`access_type=0`=MMU_DATA_LOAD，`size=8`）。
- 调用栈：`code_gen_buffer`（TCG 生成的宿主代码）→ `helper_le_ldq_mmu`（cputlb.c:1449）→ `load_helper`（cputlb.c:1274，TLB 未命中后调用 tlb_fill）→ `tlb_fill`（cputlb.c:878）→ `riscv_cpu_tlb_fill`（cpu_helper.c:451）→ `get_physical_address`（cpu_helper.c:158）。
- `env->satp = 0x8000000000080205`（MODE=8 即 Sv39，PPN=0x80205 → 页表物理地址 0x80205000，与内核打印的 `satp physical address` 一致）；`env->priv = 1`（S 态）；`mmu_idx = 1`。

## 2. 为什么写 satp 之后低地址取指没有缺页（QEMU 4.1.1）

现象：`entry.S` 中 `csrw satp`（PA 0x80200034）开启 Sv39 后，PC 仍是物理地址 0x80200038；启动页表只映射了 `0xffffffffc0000000` 起的 1GB（第 511 项），没有低地址映射，按规范下一次取指应当缺页。但 QEMU 4.1.1 中内核正常运行。

`-d in_asm,exec,nochain` 日志（`t7_inasm_kern_entry.txt`）：
- 第一个 TB 从 `0x80200000` 到 `csrrw zero,satp,t0`（0x80200034）结束。
- 下一个 TB 起点 `0x80200038` 被执行（`Trace 0: ... /0000000080200038/`），之后直接进入 `0xffffffffc02000d6`（kern_init）。
- 该 TB 的 in_asm 反汇编显示成一串 `sw a2,96(s0)`，不是真实指令（推测：日志反汇编走调试读内存接口，此时按新 satp 查页表读不到这段低地址；**待确认**）。

源码依据（QEMU 4.1.1 `target/riscv/csr.c:729-751` `write_satp`）：satp 的 MODE/PPN 变化时只做 `env->satp = val`，**只有 ASID 变化才 `tlb_flush`**（csr.c:746-748）。因此开启分页前，取指在 Bare 模式下填进软件 TLB 的“VA 0x80200000 → PA 0x80200000”表项仍然有效，0x80200038 的取指命中这条旧表项。随后 `sfence.vma`（`trans_privileged.inc.c:90-96`，`gen_helper_tlb_flush`）刷新 TLB，但 0x80200038～0x80200048（`jr t0`）已在同一个 TB 内翻译完毕，执行时不再取指，于是跳到高地址 `kern_init`，此后高地址按页表翻译。

推测（未验证）：QEMU 8.2.2 在 satp 变化时总会刷新 TLB，所以 0x80200038 的取指缺页——与环境调研时在 8.2.2 上看到的 `exec_page_fault, epc=0x80200038` 一致。8.2.2 源码未核对。

意义：这同时说明 QEMU 的软件 TLB 在 Bare 模式（未开启分页）时也被使用（VA=PA 的表项），而真实 CPU 在 Bare 模式下不经过地址翻译，也不会有这类表项残留——第 4 问的一个实例。
