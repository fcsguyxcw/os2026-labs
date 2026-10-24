set pagination off
set confirm off
set remotetimeout unlimited
file bin/kernel
set arch riscv:rv64
target remote localhost:1234
continue
