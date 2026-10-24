# 终端3：make gdb已经加载bin/kernel并连接stub后 source 本文件。
set pagination off
# 关闭分页。
set confirm off
# 避免批处理等待常规确认。
set remotetimeout unlimited
# QEMU宿主被终端2暂停时允许remote一直等待。
set disassemble-next-line off
# 避免每次停点自动反汇编造成额外debug内存访问；按需手动x/i。
set $t5_va = (unsigned long long)&npage
# 从本次ELF取得全局变量虚拟地址，仅取符号地址。
p/x $t5_va
# 将此值复制到终端2的$t5_watch_va做精确数据地址过滤。
p/x (unsigned long long)&kern_init
# 取指对照目标地址；不要把取指说成数据load。
p/x (unsigned long long)&kern_entry
# ELF中的高地址入口；上电执行ucore时使用对应物理地址。
p/x (unsigned long long)&boot_page_table_sv39
# 打印根表虚拟地址。
p/x (unsigned long long)&boot_page_table_sv39 - 0xffffffff40000000
# 根据entry.S静态偏移求根表物理地址，稍后与satp核验。
hbreak *0x80000000
# 在本实验OpenSBI物理入口停下，供选择分页前的真实ld。
set $t5_firmware_bp = $bpnum
# 保存固件断点编号，命中后可手动disable避免重入。
hbreak kern_init
# 使用硬件断点，避免插入软件断点改写客户机代码。
set $t5_kernel_bp = $bpnum
# 保存高地址内核入口断点编号。
define t5-firmware
# 定义固件现场检查命令，必须在固件入口停住时使用。
  info registers pc mstatus satp
  # 核验当前执行位置以及Bare/特权相关状态。
  x/16i $pc
  # 反汇编选择真实ld，记其rs1与imm，再si到该条之前。
  info registers
  # 用寄存器值加立即数算实际数据地址，传给终端2的$t5_bare_va。
end
# 结束用户命令。
define t5-kernel
# 定义分页后的目标检查命令，必须已停在kern_init或之后。
  info registers pc satp mstatus
  # 核验已开启Sv39，不能仅凭高地址猜测。
  p/x $t5_va
  # 再次打印npage有效地址。
  disassemble /r 'best_fit_pmm.c'::basic_check
  # 查看确定读取npage的函数，选真实ld的PC，避免直接猜地址。
  disassemble /r kern_entry
  # 对照写satp前的sd以及sfence，确认该入口没有数据ld。
end
# 结束用户命令。
printf "T5: firmware hbreak then t5-firmware; host phase=1 before guest continue to kern_init; there use t5-kernel\n"
# 提示双调试器顺序；host停住时应先恢复host再继续guest。
continue
# 执行到固件硬件断点或host真实访存断点；停止后人工选择下一步。
