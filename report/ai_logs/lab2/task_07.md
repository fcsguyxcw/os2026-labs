# Task 07：分支任务 · 在 Docker 中实际调试 QEMU 的地址翻译（Leader 执行）

执行者：Leader（Claude Code）。原因：子 Agent 沙箱无法访问 Docker（bugs.md 问题 1）。调试方案参考 Task 05 的源码分析 `evidence/t5_qemu_analysis.md`（行号、断点位置、需打印的变量），脚本按批处理方式改写。

## 做法

一个容器内跑完三终端流程（`evidence/t7_scripts/run.sh`）：QEMU 4.1.1 `-s -S` 后台启动 → `gdb -q -batch -p <QEMU PID> -x <脚本>` 附加 QEMU、设条件断点、continue → `riscv64-unknown-elf-gdb -batch` 连 `localhost:1234` 让 ucore 继续运行。交互式演示仍用 `./os2026.sh debug / gdb-qemu / gdb` 三个终端，命令与脚本相同。

观察目标（`riscv64-unknown-elf-nm bin/kernel`、`objdump -d`）：
- 开启分页后：`pages` = `0xffffffffc0207000`，`page_init` 中 `SetPageReserved(&pages[0])` 第一次访问该页（自然 TLB miss）。
- 开启分页前：`kern_entry` 第三条指令 `sd a0,0(t0)`，`t0 = 0x80206000`（`boot_hartid`），此时 PC 与数据地址都是物理地址。
- 写 satp 之后的下一条取指 `0x80200038`。

## 结果

### 第一轮 `paged.gdb` → `evidence/t7_paged_qemu_gdb.txt`

- [A] `get_physical_address(addr=0xffffffffc0207008, access_type=0 (load), mmu_idx=1)`；调用栈 `code_gen_buffer → helper_le_ldq_mmu (cputlb.c:1449) → load_helper (cputlb.c:1274) → tlb_fill (cputlb.c:878) → riscv_cpu_tlb_fill (cpu_helper.c:451) → get_physical_address`；`satp = 0x8000000000080205`，`priv = 1`。
- [B] `levels=3 ptidxbits=9 ptesize=8 i=0 ptshift=18`；`base = 0x80205000`、`idx = 511`、`pte_addr = 0x80205ff8`、`pte = 0x200000cf`；`ppn = 0x80000`，V/R/W/X/A/D=1，U=0。
- [C] 第一层就是叶子（1GB 大页）：`vpn = 0xffffffffc0207`，`*physical = 0x80207000`，完整物理地址 `0x80207008`。
- [D] `finish` 回到 `riscv_cpu_tlb_fill`：`get_physical_address` 返回值 `$1 = 0`（`TRANSLATE_SUCCESS`）；`pa = 0x80207000`、`prot = 7`。此时停在 `:451` 赋值之前，`info locals` 显示的 `ret = 1` 是赋值前的旧值，不是本次结果。
- [E] `tlb_set_page_with_attrs(vaddr=0xffffffffc0207000, paddr=0x80207000, prot=7, mmu_idx=1, size=0x1000)`：页表是 1GB 大页，软件 TLB 只装 4KB 一项。
- [F] `load_helper` `cputlb.c:1338`：`addr_read = 0xffffffffc0207000`、`addend = 0xffffafe00000`，`haddr = 0xffff70007008`（QEMU 进程里存放客户机该字节的宿主地址），读出 `0x0`。
- [G] 同页第二次访问的断点（`cputlb.c:1271` 与 `get_physical_address`）在 ucore 运行完之前没有再命中：同一页后续访问在 TCG 生成的宿主代码里直接比较 TLB 命中（快速路径），不进入 C helper。

### 第二轮 `bare.gdb` → `evidence/t7_bare_qemu_gdb.txt`

- [H] `store_helper`（`cputlb.c:1519`，`helper_le_stq_mmu` 调用）：`addr = 0x80206000`、`mmu_idx = 1`、`index = 6`、`tlb_addr(addr_write) = 0xffffffffffffffff`（空表项）→ 未命中；`satp = 0`、`priv = 1`；下一行进入 `victim_tlb_hit`（`:1520`）。
- [I] `get_physical_address(addr=0x80206000, access_type=1 (store))`，`priv_ver = 0x11100`（1.11）；停在 `cpu_helper.c:197`（`case VM_1_10_MBARE`），`mode=1 vm=0`，`*physical = 0x80206000`，没有查页表。
- [J] `tlb_set_page_with_attrs(vaddr=0x80206000, paddr=0x80206000, prot=7, mmu_idx=1)`：未开启分页时也装入软件 TLB，VA==PA。
- [K] 写 satp 之后（`satp = 0x8000000000080205`），`0x80200038` 的取指两次进入 `get_page_addr_code`（`tb_htable_lookup` 与 `tb_gen_code` 调用），代码 TLB 表项 `addr_code = 0x80200000`（开启分页前填入的旧表项）命中；为 `0x80200038` 设的 `get_physical_address` 断点没有命中。与 `t7_leader_notes.md` 第 2 节结论一致：QEMU 4.1.1 `write_satp` 只在 ASID 变化时刷 TLB（csr.c:746）。第二次停下后脚本 detach，没有继续观察到 kern_init。

## Verification

以上每条均来自 gdb 实际输出（evidence 文件）。未验证：真实 CPU TLB 的行为（只能引用规范和教材）；QEMU 8.2.2 的 satp 写入源码。

状态：分支任务 1～4 问的调试数据 **DONE**；交互式三终端演示待用户按 `演示步骤` 自己练一遍。
