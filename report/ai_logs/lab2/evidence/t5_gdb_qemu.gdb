# 终端2：附加 QEMU 后 source /tmp/t5_gdb_qemu.gdb；默认最后 continue。
set pagination off
# 关闭分页，便于记录。
set confirm off
# 关闭常规确认提示。
set breakpoint pending off
# 缺失符号或行号显式报错，避免静默 pending。
handle SIGPIPE nostop noprint pass
# 按入口脚本要求忽略 SIGPIPE 停顿。
set remotetimeout unlimited
# 保留无限远程等待设置，真正的 remote 会话在终端3。
directory /opt/qemu-src/qemu-4.1.1
# 为本地 DWARF 相对路径提供容器源码搜索目录。
if $_isvoid($t5_phase)
# 允许 source 前指定阶段；0为 Bare，1为分页后。
  set $t5_phase = 0
  # 默认先记录 Bare 对比。
end
# 结束可选初始化。
if $_isvoid($t5_watch_va)
# 精确内核数据目标可由终端3打印地址后传入。
  set $t5_watch_va = 0
  # 0表示捕获任意高地址内核数据 load。
end
# 结束初始化。
if $_isvoid($t5_bare_va)
# 固件 ld 的有效地址需由实际反汇编确定。
  set $t5_bare_va = 0
  # 0表示匹配固件区间数据 load。
end
# 结束初始化。
set $t5_from_fill = 0
# 初始禁止 debug 翻译直接命中下游断点。
set $t5_thread = -1
# 尚无真实 fill 的线程。
set $t5_fill_addr = 0
# 尚无真实 fill 的地址。
break helper_le_ldq_mmu if (($t5_phase == 0 && (env->satp >> 60) == 0 && addr >= 0x80000000 && addr < 0x80200000 && ($t5_bare_va == 0 || addr == $t5_bare_va)) || ($t5_phase == 1 && addr >= 0xffffffffc0200000 && ($t5_watch_va == 0 || addr == $t5_watch_va)))
# 捕获64位数据load的慢路径入口；不含 code_read helper。
set $t5_helper_bp = $bpnum
# 保存编号供需要时手动 disable。
commands
# 定义 helper 命中时的打印命令。
  printf "TCG ldq helper: addr=%#lx oi=%#x retaddr=%#lx\n", addr, oi, retaddr
  # 只打印函数入口已就绪的参数。
  bt 10
  # 核验 helper 与 JIT 调用链；此处停住，用 continue 进入 fill。
end
# 结束断点命令。
break accel/tcg/cputlb.c:1271 if !code_read && (($t5_phase == 0 && (env->satp >> 60) == 0 && addr >= 0x80000000 && addr < 0x80200000 && ($t5_bare_va == 0 || addr == $t5_bare_va)) || ($t5_phase == 1 && addr >= 0xffffffffc0200000 && ($t5_watch_va == 0 || addr == $t5_watch_va)))
# 强制内联 load_helper 的源码行替代停点。
set $t5_load_line_bp = $bpnum
# 保存可能含多个位置的断点编号。
commands
# 定义内联停点打印。
  p/x addr
  # 被读取的客户机地址。
  p mmu_idx
  # 当前软件TLB组。
  p index
  # 主表索引。
  p/x tlb_addr
  # 本次读取类型的tag。
  p/x entry->addr_read
  # entry已经初始化，可查看读tag。
  p/x entry->addend
  # 宿主地址偏移；失配entry的值不代表此VA的有效映射。
  bt 10
  # 核验不是取指路径。
end
# 结束断点命令。
disable $t5_load_line_bp
# 默认关闭昂贵多位置断点，需要时按编号启用。
break target/riscv/cpu_helper.c:451 if access_type == 0 && (($t5_phase == 0 && (env->satp >> 60) == 0 && address >= 0x80000000 && address < 0x80200000 && ($t5_bare_va == 0 || address == $t5_bare_va)) || ($t5_phase == 1 && address >= 0xffffffffc0200000 && ($t5_watch_va == 0 || address == $t5_watch_va)))
# 在env初始化后捕获真实数据load fill，而不是debug物理地址查询。
set $t5_fill_bp = $bpnum
# 保存fill断点编号。
commands
# fill处打印并给下一次真正翻译开门。
  set $t5_from_fill = 1
  # 标记这一次调用来自实际tlb_fill。
  set $t5_thread = $_thread
  # 限定同一个宿主线程，排除其他线程调试读。
  set $t5_fill_addr = address
  # 记录本次地址。
  set $t5_env = env
  # 保存env以便之后检查。
  printf "real fill: address=%#lx size=%d access=%d mmu_idx=%d probe=%d\n", address, size, access_type, mmu_idx, probe
  # 展示真实访问类型和大小。
  p env->priv
  # 当前CPU特权模式。
  p/x env->satp
  # 当前地址翻译控制寄存器。
  p/x env->mstatus
  # 核验MPRV、MPP、SUM、MXR相关状态。
  bt 10
  # 应包含tlb_fill与实际load helper。
  continue
  # 自动进入下一层get_physical_address停点。
end
# 结束fill断点命令。
break get_physical_address if $t5_from_fill == 1 && $_thread == $t5_thread && addr == $t5_fill_addr && access_type == 0
# 只接收真实fill紧接着的同线程同地址调用，debug路径不满足门控。
set $t5_walk_bp = $bpnum
# 保存页表翻译入口断点编号。
commands
# 真正翻译入口处停住。
  set $t5_from_fill = 0
  # 立即关门，防止后续调试内存读取误命中。
  set $t5_va_seen = addr
  # 保存本次真实数据地址。
  set $t5_out = physical
  # 保存返回物理地址的指针，finish后仍可读取。
  printf "translate actual data load: VA=%#lx access=%d mmu_idx=%d\n", addr, access_type, mmu_idx
  # 输出本次目标。
  p env->priv_ver
  # 核验使用哪一版特权规范分支。
  bt 12
  # 不应出现cpu_memory_rw_debug；若出现应丢弃样本。
end
# 结束页表入口断点命令。
printf "T5 armed: phase=%d; helper first stop: continue; paged walk: tbreak target/riscv/cpu_helper.c:254 then continue\n", $t5_phase
# 给出下一个人工操作，不把静态预期写成运行结果。
continue
# 恢复QEMU宿主线程，使终端3的remote连接和客户机执行能进行。
