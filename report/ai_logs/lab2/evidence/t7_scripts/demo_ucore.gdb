set pagination off
set confirm off
set remotetimeout unlimited
file bin/kernel
set arch riscv:rv64
target remote localhost:1234
p/x (((unsigned long long)&end + 4095) & ~4095ULL) + 8
p/x &kern_init
continue
