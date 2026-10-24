# Task 05：QEMU 4.1.1 访存静态分析及调试方案

本次只读本地源码，不运行 Docker、QEMU、gdb、编译器或交叉工具。下文“预期”均为源码推导，不是实测记录。QEMU 文件路径相对 `/Users/wesley/大三上课程/操作系统/tmp/qemu-src/`；ucore 路径相对仓库 `os2026-labs/`。运行输出须由 Leader 补充，不能将本文当作实验输出。

## 1. 一次 RV64 数据 load 的路径

必须区分“翻译客户机指令”和“运行生成的宿主代码”。`trans_ld`（`target/riscv/insn_trans/trans_rvi.inc.c:213`）调用 `gen_load`（`:138`），计算 `rs1+imm`（`:142-143`），发出 `tcg_gen_qemu_ld_tl(..., ctx->mem_idx, MO_TEQ)`（`:145,215`）。`ctx->mem_idx` 取 TB flags（`target/riscv/translate.c:759`）；TB flags 的 MMU 部分来自 `cpu_mmu_index`（`target/riscv/cpu.h:290-301`）。这些是生成代码的函数，不应写成每次运行 load 都经过它们的 C 调用栈。

慢路径的一行版：生成的 TCG 访存代码 → `helper_le_ldq_mmu`（`accel/tcg/cputlb.c:1446`）→ 内联 `load_helper`（`:1248`）→ `tlb_hit`（`include/exec/cpu-all.h:359`）→ 主表 miss 时 `victim_tlb_hit`（`cputlb.c:977`）→ 再 miss 时 `tlb_fill`（`:868`）→ `CPUClass.tlb_fill`（`:878`，RISC-V 注册于 `target/riscv/cpu.c:489`）→ `riscv_cpu_tlb_fill`（`target/riscv/cpu_helper.c:435`）→ `get_physical_address`（`:155`）→ PMP 最终地址检查（`:463-467`）→ `tlb_set_page`（`cputlb.c:843`）→ `tlb_set_page_with_attrs`（`:697`）→ 返回 `load_helper`，重取可能被扩容移动的 entry（`:1276-1279`）→ `haddr=addr+entry->addend`（`:1338`）→ `ldq_le_p(haddr)`（`:1361`）读取宿主所承载的客户机 RAM。

范围限制：所给副本没有顶层 `tcg/` 后端及 `tcg-op.c`，无法逐行核对宿主后端如何选择 ldq helper 和生成主 TLB 快路径。因此“生成代码到 helper”的具体后端函数名、宿主架构及汇编布局待调试确认；不虚构后端文件行号。快路径可绕过 C helper，这由 `include/exec/cpu-defs.h:133-136,179-188` 对生成代码直接访问 TLB 的说明支撑；不能承诺每次执行 `ld` 都命中 `helper_le_ldq_mmu`。本文已给齐副本内可核对的 helper→翻译→RAM 路径。

关键分支（均为这份 4.1.1 的实现）：

- 对齐要求不满足时调用架构异常处理：`cputlb.c:1264-1268`，RISC-V 对数据 load 报地址未对齐：`cpu_helper.c:411-431`。
- 主表命中：`cputlb.c:1271` 跳过 miss 处理；主表未命中但 victim 命中：`:1272-1273`，交换主表/victim 和配套 IOTLB（`:994-1007`），不遍历页表。
- 两者未命中才 fill：`:1274-1278`；fill 成功并非仅算出 PA，还要装入软件 TLB。`cpu_helper.c:471-479`：成功装表；probe 失败返回 false；非 probe 失败设置异常并调用 `riscv_raise_exception`。该函数调用 `cpu_loop_exit_restore`（`target/riscv/op_helper.c:28-34`），不能期待正常 return。load 的 `tlb_fill` 传 probe=false（`cputlb.c:878`）。页故障/访问故障区分在 `cpu_helper.c:355-379`，PMP 失败按访问故障处理。
- entry 的低位有特殊标志时进入慢处理：`cputlb.c:1282-1310`；`TLB_RECHECK` 再做权限检查/fill，MMIO 经 `io_readx`（`:882`），使用 IOTLB 求 region offset（`:894-896`），调用 `memory_region_dispatch_read`（`:909`；定义 `memory.c:1439`），失败交给 `cpu_transaction_failed`（`:911-917`）。不能把 MMIO 当作 host RAM 指针读取。
- 跨页/特殊未对齐读取分成两次 `full_load` 并按端序拼接：`cputlb.c:1313-1334`。普通 RAM 在 `:1337-1362` 直接读宿主指针。

客户机物理地址到宿主 RAM 的中间链路：`tlb_set_page_with_attrs` 在 `cputlb.c:725` 调 `address_space_translate_for_iotlb`（`exec.c:722`），后者查地址空间、处理可选 IOMMU，返回 region 内的 `xlat`（`exec.c:733-770`）。RAM 分支用 `memory_region_get_ram_ptr(section->mr)+xlat`（`cputlb.c:741-748`）；`memory_region_get_ram_ptr` 解开 alias 并调用 `qemu_map_ram_ptr`（`memory.c:2193-2205`）。最后 entry 存 `addend=host_page_pointer-vaddr_page`（`cputlb.c:804`）。`addend` 不是客户机 PA−VA，也不是 ucore 的 `va_pa_offset`。

## 2. get_physical_address 的 Sv39 遍历

1. `mode=mmu_idx`；若 M 模式数据访问且 MPRV=1，则按 MPP 改有效 mode（`cpu_helper.c:163-169`）。有效 M 模式或 CPU 没 MMU 功能，`:171-174` 直接 `*physical=addr`，权限 RWX。不能把所有当前 M 模式访问都称为直接映射。
2. 1.10 及以后用 `satp.PPN<<12` 作为根表**客户机物理地址**，从 mstatus 取 SUM/MXR（`:181-186`）；Sv39：`levels=3, ptidxbits=9, ptesize=8`（`:190-191`）。Bare 直接返回 addr（`:196-199`）。旧特权版本从 sptbr/mstatus.VM 取配置（`:203-220`），本实验应先打印 `env->priv_ver`，不能凭版本名替代观测。
3. 检查 VA canonical：高位必须是 bit38 的符号扩展（`:223-229`）；非 canonical 直接失败。`ptshift=18`（`:231`），循环 i=0、1、2 分别对应标准命名 L2、L1、L0（`:237`），每轮减9。
4. 每轮 `idx=(addr>>(12+ptshift))&0x1ff`（`:238-239`），即 VPN[2]=VA[38:30]、VPN[1]=VA[29:21]、VPN[0]=VA[20:12]。`pte_addr=base+idx*8`（`:242`）。注意这不是宿主地址也不是内核虚拟地址。
5. 可选 PMP 检查 PTE 物理读取权限（`:244-247`）。RV64 用 `ldq_phys(cs->as,pte_addr)` 读 PTE（`:252`），不是再调用 `get_physical_address` 对 pte_addr 做 Sv39。`include/exec/memory_ldst_phys.inc.h:29-32` 展开为 `address_space_ldq(as, addr, MEMTXATTRS_UNSPECIFIED, NULL)`；`exec.c:3819-3825` 以物理 AddressSpace 翻译宏实例化 `memory_ldst.inc.c`。副本缺少该 `.inc.c`，不能给出其内部 RAM/MMIO 分支行号；该函数内部实现待 Leader 在容器源码确认，不影响“读的是客户机物理地址”的结论。
6. `ppn=pte>>10`（`:254`）。V=0 失败（`:256-258`）；R/W/X 全0是非叶子，`base=ppn<<12`，继续下一层（`:259-261`）。走完3层仍没有叶子则失败（`:352`）。W=1且R=0 的 W 或 W+X 组合均失败（`:262-267`），因此不能笼统说“W=1也是合法叶子”。合法叶子还要通过 U/S、SUM、超级页对齐、访问种类和 MXR 检查（`:268-288`）。load 允许 R，或者 X 且 MXR=1（`:279-282`）；S 取用户页指令不会因 SUM=1 被允许（`:268-272`）。
7. 超级页的低 `ptshift` 位 PPN 必须为0（`:276-278`）：L2 叶子 ptshift=18 对应1GB，L1 ptshift=9 对应2MB，L0 ptshift=0 对应4KB。叶子后无须继续3层。
8. `updated_pte=pte|A|(store?D:0)`（`:291-292`）。load 设置 A，不主动设置 D。需要更新时先物理地址空间查 region，再映射 host RAM，对 PTE 做原子 CAS（`:304-322`）；非 RAM 更新失败（`:324-327`）。CAS 不一致 `goto restart`（`:318-319`）。值得说明：此版本标签在 `:235`，位于 base/ptshift 初始化之后，源码没有在此标签显式重置 base/ptshift；不能宣称它一定“从根节点重新开始”。本实验 PTE 已置 A/D，通常不进入该路径，竞争路径行为待调试确认。`TCG_OVERSIZED_GUEST` 下走非原子分支（`:311-315`），须以实际构建为准。
9. `vpn=addr>>12; *physical=(ppn|(vpn&((1L<<ptshift)-1)))<<12`（`:333-334`）。**分页路径返回的是该 VA 所在4KB物理页的基址，不含 VA[11:0]**；完整字节地址是该值加 `addr&0xfff`。Bare/M 直接返回原 addr（可能带页内偏移），所以这两条返回值语义在这里并非完全一致。fill 会把两边都按4KB对齐（`:472-473`），普通读取在 host 加回原始 addr（`cputlb.c:1338`）。
10. R/X 权限写入 prot；只有 store 或已 D 的可写页才给软件 TLB 写权限（`cpu_helper.c:336-348`），保证以后写访问有机会 miss 并更新 D。

本实验推导示例（不是实测）：ucore `code/kern/init/entry.S:59-63` 的根表 entry511 为 `(0x80000<<10)|0xcf = 0x200000cf`，V/R/W/X/A/D=1、U=0。对 VA `0xffffffffc0200000`，根层 idx=511，根 PTE 地址=根物理地址+0xff8。若根地址仍是任务 RELY 中的 0x80205000，则 PTE 地址=0x80205ff8；必须用本次符号/寄存器确认。ppn=0x80000，低18位为0，L2即叶子；返回物理页基址0x80200000。对 VA `0xffffffffc0200123`，返回页基址仍为0x80200000，完整 PA=0x80200123。

ucore 目前只构造这一张根页表（`entry.S:53-63`），`pmm_init` 读取并打印其地址（`code/kern/mm/pmm.c:127-130`），未在本实验代码中找到建立下级表的主线。因此展示一层遍历就是正确结果，不应为了“演示三级”编造另外两次 PTE 读取；若要多级映射需另行授权修改客户机，本任务不做。

## 3. 软件 TLB 的组织与查找

`include/exec/cpu-defs.h:110-129` 的 CPUTLBEntry 含 addr_read/addr_write/addr_code（三类访问的虚拟页 tag 和低位标志），以及到宿主地址的 addend。无权限类型的 tag 写 -1（`cputlb.c:805-817`）。IOTLB 另存 region/地址空间偏移和 MemTxAttrs（`cpu-defs.h:138-150`）。

每个 CPU 的 CPUTLB 按 mmu_idx 分组，每组有快速主表及描述符（`cpu-defs.h:218-221`）。RISC-V 有4组、4KB页（`target/riscv/cpu-param.h:20-21`），当前 `riscv_cpu_mmu_index` 返回 env->priv（`cpu_helper.c:27-33`）；不是 ASID，也不是 satp MODE。MPRV 的有效权限转换是在上述翻译函数内做的。ctx 的 index 进入 oi，`load_helper` 用 `get_mmuidx(oi)` 取回（`cputlb.c:1252`），随后按 index 选组。

主表索引是 `(addr>>TARGET_PAGE_BITS)&(mask>>CPU_TLB_ENTRY_BITS)`（`include/exec/cpu_ldst.h:163-180`）。mask 表示动态大小（`cpu-defs.h:183-187`）；默认位数8、最小6（`:92-93`），不能武断称永久固定256项。`tlb_hit` 的定义不在 cputlb.c，而在 `include/exec/cpu-all.h:348-361`：比较页地址并保留 invalid 位参与比较，其他特殊标志留待慢路径处理。

victim 是每组8项全相联表（`cpu-defs.h:83-84,170-174`）；`victim_tlb_hit` 线性扫描并在命中时交换主表和 victim 的 TLB/IOTLB（`cputlb.c:977-1010`）。装表冲突时按 `vindex++%8` 淘汰到 victim（`:777-786`）。这个具体组织属于 QEMU 实现选择，不能当作真实 RISC-V 芯片规定。

## 4. 与硬件 TLB 的逻辑区别、Bare/M 比较

硬件地址翻译的架构结果是客户机 VA→PA 和权限；QEMU 软件 TLB 还缓存 PA 所对应的宿主 RAM 指针偏移以及 MMIO 分发信息，服务的是模拟器的内存访问。这个区别可直接由 `cpu-defs.h:110-150`、`cputlb.c:725-748,800-804,1338` 看出。真实 CPU 的缓存组织、项数和替换策略并不由这段软件实现决定；本文没有实测任何真实 CPU TLB。

未开启分页也可走软件 TLB；`load_helper` 的查表/fill 入口没有 satp 开关判断（`cputlb.c:1252-1275`）。miss 后仍经 `riscv_cpu_tlb_fill`，在 `get_physical_address` 的有效 M/无MMU分支（`cpu_helper.c:171-174`）或 Bare 分支（`:196-199`；旧版本`:214-217`）直接令 PA=addr，再经装表与 host 地址映射。**不是每次 Bare load 都 fill**：主表、victim 或生成代码快路径命中时均可跳过翻译。开启分页后的命中同样不会遍历页表。

另外：即使客户机使用1GB叶子，此 RISC-V fill 仍传 TARGET_PAGE_SIZE 装软件 TLB（`cpu_helper.c:472-473`；`cpu-param.h:20`），页表遍历用 VA 的低 VPN 合成对应4KB物理页（`:331-334`）。不要将“页表1GB大页”和“QEMU单个软件TLB项覆盖1GB”混为一谈。

`sfence.vma` 翻译为 flush helper（`target/riscv/insn_trans/trans_privileged.inc.c:90-95`），后者调用 `tlb_flush`（`op_helper.c:144-153`）。写 satp 不应被概括为此版本每次全刷：`csr.c:739-749` 在 ASID 改变时显式 flush；本实验紧随其后的 sfence 才提供明确刷新路径（ucore `entry.S:29-31`）。

## 5. 两脚本的执行与观察方案（尚未实测）

入口脚本的实际行为：`os2026.sh:21-28` 中 debug 启动容器和 make debug；gdb-qemu attach 且只显式设置 SIGPIPE；gdb 运行 make gdb。`set remotetimeout unlimited` 由 debug 写入容器 /root/.gdbinit，不是 gdb-qemu 子命令里直接配置。脚本再次显式设置，避免依赖该副作用。客户机连接参数来自 `code/Makefile:179-182`。不要把 QEMU 的 host gdb 和 RISC-V remote gdb 混用。

按以下顺序由 Leader 操作（本 Agent 未执行）：

1. 宿主课程目录终端1 `./os2026.sh debug`。
2. 终端2 `./os2026.sh gdb-qemu`，输入 `source /code/../report/ai_logs/lab2/evidence/t5_gdb_qemu.gdb` **不可用**，因为入口只挂载 code 到 /code（`os2026.sh:13`）。应先由 Leader 将这两个已生成脚本复制到容器 /tmp（例如宿主运行 `docker cp os2026-labs/report/ai_logs/lab2/evidence/t5_gdb_qemu.gdb os:/tmp/t5_gdb_qemu.gdb` 及另一文件），然后 `source /tmp/t5_gdb_qemu.gdb`。脚本自动 continue，让 host QEMU 能处理后续 remote 连接。
3. 终端3 `./os2026.sh gdb`，连接完成后 `source /tmp/t5_gdb_ucore.gdb`。它打印符号地址、设置固件入口0x80000000及 kern_init 的硬件断点，然后 continue。0x80000000 来自本实验 OpenSBI 固件基址（Task 05 RELY）；实际是否可停、调试器支持的硬件断点行为待调试确认。host 在真正数据 load 上停住时，终端3的 remote 命令暂时等待是正常的；先在终端2继续，不能同时让两边单步。
4. host 默认 `$t5_phase=0`：捕获 Bare 下访问 `[0x80000000,0x80200000)` 的实际数据 load，用于 OpenSBI 对比。源码副本不含固件，不能预先给出确定的固件 ld PC/有效地址。终端3停在固件入口时 `x/16i $pc`，逐步用 `si` 找到 `ld rd,imm(rs)`，用 `info registers` 计算 rs+imm，把该实际地址记下来；可在 host `set $t5_bare_va=0x实际地址` 后继续实现精确过滤。默认0为范围捕获，不代表0地址本身。若没遇到满足条件的 load，需从固件反汇编选择其实际数据地址并调整条件，不能声称已验证。ucore 的 entry.S 在写 satp 前只有两条 sd（`:10-13`），没有可直接选用的数据 ld，故不虚构一条“entry.S 的 ld”。
5. Bare 的 get_physical_address 停点可 `next` 检查 mode、mstatus，走到`:172`或`:197`后执行赋值，或用 `set $t5_out=physical`、`finish`、`p/x *$t5_out` 看原地址返回。记录完 host `set $t5_phase=1`、`continue`，终端3再继续到 kern_init。此时 host 改捕获高地址内核数据 load。
6. 分页后具体数据目标选全局 `npage`：在容器 /code 执行 `riscv64-unknown-elf-nm -n bin/kernel` 查 npage/kern_init；`riscv64-unknown-elf-objdump -d -S bin/kernel` 查引用 npage 的 ld。`p/x &npage` 只取符号地址，不取内容；由终端3脚本直接打印。`best_fit_check` 的 `basic_check` 使用 npage，内核 pmm 测试会走到此读（`code/kern/mm/best_fit_pmm.c` 的 basic_check，且 `code/kern/mm/pmm.c:124-125` 调检查）。在 host `set $t5_watch_va=0x打印的地址` 可精确匹配；也可暂时保持0捕获第一条高地址数据 miss，把现场 addr 作为此次具体观察目标。不要把 kern_init 的取指冒充 npage 数据 load；取指可另用 access_type=2 在同一页表路径演示。
7. npage 所在页可能早已由其他读写装入软件 TLB，因此精确 npage 条件不保证 miss，也不保证进入 C helper。默认先捕获真实高地址数据 miss 是为了拿到可观察的页表遍历，不为结果编造冷缓存。若只需 npage 本身，在终端3反汇编选中引用它的实际 ld，硬件断点停在该指令之前；在终端2暂停 QEMU 后可在 **已有 cs 的 riscv_cpu_tlb_fill 帧** 执行 `call tlb_flush(cs)`，或在已有 env 帧按实际类型找到 CPUState 再调用，随后恢复到该 ld。此操作是人为使缓存变冷，不是原程序自然发生的 miss，须记录且有停机/锁重入风险，待调试确认；不放入默认脚本。优先使用默认自然 miss。

脚本设计说明：所有说明性 `#` 注释位于命令的下一行，避免 GDB 部分命令不支持尾随注释导致表达式解析失败。两个文件只含 GDB 命令/注释，可直接 source；没有硬编码外部输出、没有 shell 命令或自动改客户机内存。脚本尚未用 gdb 实跑，是否所有 DWARF 行号都可停需 Leader 确认。

### 断点、变量、预期栈与排除 debug 访问

QEMU 脚本设置条件化 `helper_le_ldq_mmu`，内联 load 的备用断点 `accel/tcg/cputlb.c:1271` 默认关闭（其编号保存在 `$t5_load_line_bp`，需要时 `enable $t5_load_line_bp`，若旧 gdb 不接受该写法则 `info break` 后使用编号）。`load_helper` 本身强制内联（`:1247`），不强求存在独立符号。helper 函数入口只打印已就绪参数 addr/oi/retaddr；只有走到1271才读取 index、entry、tlb_addr、entry->addr_read、addend；不能在函数入口打印未初始化 locals。helper 之外可能看到 JIT 地址而不是完整 C 符号，待调试确认。

`riscv_cpu_tlb_fill` 停在 `cpu_helper.c:451`：env、pa/prot 指针已准备完，打印 address/size/access_type/mmu_idx/probe/env->priv/env->satp/env->mstatus 并 bt。脚本对实际 load 与地址做条件过滤，在此标记当前 host 线程及地址，然后自动继续；`get_physical_address` 入口断点只接收同线程同地址的这次实际 fill，并立即清门控标记，避免 gdb 内存读取误触发。debug 路径是 `exec.c:3964-3990` 的 `cpu_memory_rw_debug` → CPU debug translation → `cpu_helper.c:382-389` → get_physical_address，未经过 fill 标记；所以无需依赖 gdb Python 的 caller 过滤函数。仍应 bt 核验无 cpu_memory_rw_debug；若有，明确丢弃该样本。不要仅凭 addr/访问类型来排除它，debug 传的 access_type 也是0且地址被对齐（`:389`、`exec.c:3975`）。

实际 load miss 的预期栈（仅结构，非运行记录）：`get_physical_address → riscv_cpu_tlb_fill → tlb_fill → helper_le_ldq_mmu（内联 load_helper）→ JIT/CPU执行循环`。其他读取宽度是 helper_le_ldul_mmu 等，不应强求每次都有 ldq。debug 栈中则可见 riscv_cpu_get_phys_page_debug/cpu_memory_rw_debug，不能作为一次客户机实际 ld 的证据。

从 paged 的 get_physical_address 停点继续：`tbreak target/riscv/cpu_helper.c:254`、`continue`，打印 `addr, mode, base, levels, ptidxbits, ptesize, i, ptshift, idx, pte_addr, pte`；next 后才读取 ppn。`tbreak ...:333`、`continue` 跳过权限/A/D检查，到叶子拼接；`next` 执行 vpn 赋值，`next` 执行 physical 赋值，然后 `p/x *physical`，并算 `*physical+(addr&0xfff)`。保留 `$t5_env` 和 `$t5_va_seen` 便于离开作用域后使用。`finish` 返回 fill 后看 pa/prot/ret；PMP 检查及装表之后，返回 helper 后在 `cputlb.c:1338` 看 entry 与 addend，再 next 到 haddr 赋值后 `p/x haddr`。源码行停点如映射到多个内联实例，用 `info break` 核对 location，并增加 addr/线程条件，避免误命中取指或其他 CPU。

装表单独可在 fill 帧 `tbreak tlb_set_page_with_attrs`，命中打印 vaddr/paddr/prot/mmu_idx/size；然后 `tbreak accel/tcg/cputlb.c:835` 看 tn.addr_read/tn.addend/index/xlat，此时新值尚未拷入 te，next后再看 *te。源码中的 `tlb_entry` 局部名实际为 entry（load_helper）或 te（装表），不能照抄不存在的 tlb_entry 变量。TLB main/victim 观察点分别是 `cputlb.c:1271` 与 `:994`，victim命中与否取决于现场缓存，不保证演示必然发生。

批处理：脚本在第一次感兴趣事件处停止并允许 batch 结束，不会凭空生成全程翻译日志。Leader 可在宿主使用 `docker exec os gdb -q -batch -p <真实QEMU_PID> -x /tmp/t5_gdb_qemu.gdb`，另终端运行已连接后的 RISC-V 脚本，或 `riscv64-unknown-elf-gdb -q bin/kernel -ex 'set remotetimeout unlimited' -ex 'target remote localhost:1234' -batch -x /tmp/t5_gdb_ucore.gdb`。第一次 Bare 命中与分页主线建议分两轮重新启动；第二轮 source host 脚本前 `set $t5_phase=1`，其初始化保留已有设置。batch gdb 退出会改变 attach/stub状态，完整自动化要由 Leader 按实际输出增加命令，不能声称本文两份交互脚本已验证可无人工完成两阶段。

## 6. 三分钟答辩主线

- 0:00—0:30：在 host 的真实数据 miss 对应 get_physical_address 停点，bt 排除 debug；记录 addr、access_type=0、env->satp、mmu_idx 和 mode。先说明这是客户机一次数据读，PC 与被读地址不同，函数注释已提示 env->pc 此时未必准确（cpu_helper.c:159-161）。
- 0:30—1:30：停254行，展示 Sv39 levels=3 / idx=511 / ptesize=8、根物理 base、pte_addr 与 pte；解释 ldq_phys 读客户机物理页表。next 得 ppn，说明这是根层1GB叶子，不需要三次读PTE（cpu_helper.c:237-278、entry.S:59-63）。
- 1:30—2:15：停333行后 next两次展示物理页基址，再加 VA 页内偏移得到完整 PA；观察 prot，finish回fill。补充这次 PTE 的 A/D 已置位，因此无CAS写回，不伪造 A/D 更新现场（entry.S:63、cpu_helper.c:291-334）。
- 2:15—2:45：用预先设置的装表835行与load1338行展示 tn.addend 和最终 haddr，说明 VA→客户机PA→宿主指针三种地址不能混用（cputlb.c:804,835,1338-1361）。实际单步数受DWARF停点影响，建议两个临时源码断点加约4次next，而不是按固定次数盲走。
- 2:45—3:00：展示独立一轮 Bare/M 样本：同样可 TLB miss，但在171或196分支直接 PA=addr，不遍历页表（cpu_helper.c:171-199）。这解释软件 TLB 还承担客户机物理内存到宿主内存的加速映射。

最不确定的三点：① 副本缺失 TCG 宿主后端及 memory_ldst.inc.c，其具体生成函数/物理读取内部路径须在容器源码补证；② 优化与内联、当前软件 TLB 冷热及实际固件 ld 地址影响断点能否命中；③ GDB 行号/多位置断点、双调试器交接与批处理退出行为须实跑确认。以上均未标为已验证。指导书“有趣细节/LLM交互记录”只可记录此次真实静态发现（inline、debug伪访问、大页和返回页基址），实际调试趣事待 Leader 反馈，不能编造。
