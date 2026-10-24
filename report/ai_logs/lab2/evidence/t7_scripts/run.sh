#!/bin/bash
# 用法：run.sh <qemu侧gdb脚本> <ucore侧gdb脚本> <输出前缀>
cd /code
make >/dev/null 2>&1
qemu-system-riscv64 -machine virt -nographic -bios default -device loader,file=bin/ucore.img,addr=0x80200000 -s -S > /out/$3_console.txt 2>&1 &
sleep 1
QPID=$(pgrep -f '^qemu-system-riscv64')
timeout 60 gdb -q -batch -p $QPID -x /dbg/$1 > /out/$3_qemu_gdb.txt 2>&1 &
G1=$!
sleep 3
timeout 50 riscv64-unknown-elf-gdb -q -batch -x /dbg/$2 > /out/$3_ucore_gdb.txt 2>&1 &
G2=$!
wait $G1
sleep 3
kill $G2 2>/dev/null
kill $QPID 2>/dev/null
wait
