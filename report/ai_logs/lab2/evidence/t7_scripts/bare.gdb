set pagination off
set confirm off
handle SIGPIPE nostop noprint
# ===== 第二轮：开启分页之前的访存（kern_entry: sd a0,0(t0)，t0=0x80206000 即 boot_hartid）=====
# ---- 1. store_helper 里的 C 语言 TLB 查找（:1519）----
break accel/tcg/cputlb.c:1519 if addr == 0x80206000
continue
echo \n===== [H] store_helper 查 TLB =====\n
bt 3
printf "addr=0x%lx mmu_idx=%ld index=%ld tlb_addr(addr_write)=0x%lx 页=0x%lx -> %s\n", addr, mmu_idx, index, tlb_addr, addr & ~0xfffUL, ((tlb_addr & ~0xfffUL) == (addr & ~0xfffUL)) ? "命中" : "未命中"
printf "satp=0x%lx priv=%d\n", env->satp, env->priv
next
echo [H] next 后：\n
frame
delete
# ---- 2. get_physical_address：Bare 分支 ----
break get_physical_address if addr == 0x80206000
continue
echo \n===== [I] get_physical_address（Bare）=====\n
bt 4
printf "addr=0x%lx access_type=%d mmu_idx=%d satp=0x%lx priv_ver=0x%x\n", addr, access_type, mmu_idx, env->satp, env->priv_ver
delete
break target/riscv/cpu_helper.c:197 if addr == 0x80206000
continue
echo \n===== [I] 走到 case VM_1_10_MBARE =====\n
printf "mode=%d vm=%d\n", mode, vm
next
printf "*physical=0x%lx（等于 addr，没有查页表）\n", *physical
delete
break tlb_set_page_with_attrs if vaddr == 0x80206000
continue
echo \n===== [J] 装入软件 TLB（VA==PA）=====\n
printf "vaddr=0x%lx paddr=0x%lx prot=%d mmu_idx=%d\n", vaddr, paddr, prot, mmu_idx
delete
# ---- 3. 写 satp 之后在 0x80200038 取指：看旧 TLB 表项 ----
break accel/tcg/cputlb.c:1030 if addr == 0x80200038
break get_physical_address if addr == 0x80200038
break get_physical_address if addr == 0xffffffffc02000d6
continue
echo \n===== [K] 写 satp 之后第一次停下 =====\n
bt 2
printf "addr=0x%lx satp=0x%lx\n", addr, env->satp
printf "（若停在 get_page_addr_code，下面是代码 TLB 表项）\n"
print/x entry->addr_code
print/x index
continue
echo \n===== [K] 第二次停下 =====\n
bt 2
printf "addr=0x%lx satp=0x%lx\n", addr, env->satp
detach
quit
