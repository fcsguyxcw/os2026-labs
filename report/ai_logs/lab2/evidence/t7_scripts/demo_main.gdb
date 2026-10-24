set pagination off
set confirm off
handle SIGPIPE nostop noprint
set $va = 0xffffffffc0207008
tbreak target/riscv/cpu_helper.c:451 if address == $va && access_type == 0
continue
bt 6
p/x address
p access_type
p mmu_idx
p/x env->satp
p env->priv
step
bt 7
tbreak target/riscv/cpu_helper.c:254 if addr == $va
continue
p levels
p ptidxbits
p ptesize
p i
p ptshift
p/x base
p idx
p/x pte_addr
p/x pte
next
p/x ppn
p/x pte & 0xff
tbreak target/riscv/cpu_helper.c:334 if addr == $va
continue
p/x vpn
p/x ppn
p ptshift
next
p/x *physical
p/x *physical + ($va & 0xfff)
finish
p/x pa
p prot
next
p ret
tbreak tlb_set_page_with_attrs if vaddr == ($va & ~0xfffULL)
continue
p/x vaddr
p/x paddr
p prot
p mmu_idx
p/x size
tbreak accel/tcg/cputlb.c:1338 if addr == $va && !code_read
continue
p/x entry->addr_read
p/x entry->addend
next
p/x haddr
x/gx haddr
detach
quit
