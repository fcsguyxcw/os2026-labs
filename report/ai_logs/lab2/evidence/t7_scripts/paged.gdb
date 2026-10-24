set pagination off
set confirm off
set print pretty off
handle SIGPIPE nostop noprint
# ---- 1. 断在开启分页后第一次访问 pages[0] 所在页（自然 TLB miss）----
break get_physical_address if (addr >> 12) == 0xffffffffc0207 && access_type == 0
continue
echo \n===== [A] 命中 get_physical_address：调用栈 =====\n
bt 7
echo \n===== [A] 参数与 CPU 状态 =====\n
info args
printf "addr=0x%lx satp=0x%lx priv=%d\n", addr, env->satp, env->priv
set $va = addr
delete
# ---- 2. 页表遍历：读出 PTE 之后（:254 计算 ppn 之前）----
break target/riscv/cpu_helper.c:254 if addr == $va
continue
echo \n===== [B] 第一层页表项 =====\n
printf "levels=%d ptidxbits=%d ptesize=%d i=%d ptshift=%d\n", levels, ptidxbits, ptesize, i, ptshift
printf "base(根页表物理地址)=0x%lx idx=%ld pte_addr=0x%lx pte=0x%lx\n", base, idx, pte_addr, pte
next
printf "ppn=0x%lx  V=%ld R=%ld W=%ld X=%ld U=%ld A=%ld D=%ld\n", ppn, pte&1, (pte>>1)&1, (pte>>2)&1, (pte>>3)&1, (pte>>4)&1, (pte>>6)&1, (pte>>7)&1
delete
# ---- 3. 叶子：拼出物理页地址（:334）----
break target/riscv/cpu_helper.c:334 if addr == $va
continue
echo \n===== [C] 叶子页表项，拼接物理地址 =====\n
printf "i=%d ptshift=%d vpn=0x%lx ppn=0x%lx\n", i, ptshift, vpn, ppn
next
printf "*physical(4KB 物理页基址)=0x%lx  完整物理地址=0x%lx\n", *physical, *physical + ($va & 0xfff)
delete
finish
echo \n===== [D] 返回 riscv_cpu_tlb_fill =====\n
info locals
# ---- 4. 装入软件 TLB ----
break tlb_set_page_with_attrs if vaddr == ($va & ~0xfffUL)
continue
echo \n===== [E] tlb_set_page_with_attrs =====\n
printf "vaddr=0x%lx paddr=0x%lx prot=%d mmu_idx=%d size=0x%lx\n", vaddr, paddr, prot, mmu_idx, size
delete
# ---- 5. 回到 load_helper，用 addend 得到宿主指针 ----
break accel/tcg/cputlb.c:1338 if addr == $va
continue
echo \n===== [F] load_helper：VA + addend = 宿主指针 =====\n
printf "addr_read(tag)=0x%lx addend=0x%lx\n", entry->addr_read, entry->addend
next
printf "haddr=%p  *(uint64_t*)haddr=0x%lx\n", haddr, *(unsigned long *)haddr
delete
# ---- 6. 同一页的下一次读：TLB 命中，不再 fill ----
break accel/tcg/cputlb.c:1271 if (addr >> 12) == 0xffffffffc0207 && addr != $va
break get_physical_address if (addr >> 12) == 0xffffffffc0207
continue
echo \n===== [G] 同页第二次 load：查 TLB =====\n
bt 3
printf "addr=0x%lx index=%ld tlb_addr=0x%lx (addr & 页掩码)=0x%lx -> %s\n", addr, index, tlb_addr, addr & ~0xfffUL, ((tlb_addr & ~0xfffUL) == (addr & ~0xfffUL)) ? "命中" : "未命中"
next
echo \n===== [G] next 之后所在行（命中则跳过 victim/tlb_fill）=====\n
frame
detach
quit
