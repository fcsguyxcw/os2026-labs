# Lab 3 AI 生成与调试过程中的问题记录（Bugs & Fixes）

本文件记录 Lab 3 在使用四段式提示词生成代码、编译及运行验证过程中发现的真实问题与解决过程。

---

## Bug 1：跨实验迁移 `page_init()` 时缺少 `PHYSICAL_MEMORY_END` 宏定义导致编译报错

- **阶段**：`task_01`（将 Lab 2 的 `page_init()` 迁移到 Lab 3 的 `code/kern/mm/pmm.c`）
- **现象**：将 Lab 2 中经过验证的 `page_init()` 直接填入 `code/kern/mm/pmm.c` 后，执行编译时报错：
  ```text
  kern/mm/pmm.c:96:20: error: 'PHYSICAL_MEMORY_END' undeclared (first use in this function)
  ```
- **原因分析**：对比 Lab 2 与 Lab 3 的课程骨架头文件 `code/kern/mm/memlayout.h` 发现，Lab 2 的 `memlayout.h` 中定义了 `#define PHYSICAL_MEMORY_END 0x88000000`，而 Lab 3 骨架的 `memlayout.h` 移除了该宏定义，只保留了 `KERNBASE`、`KMEMSIZE (0x7E00000)`、`KERNTOP` 和 `PHYSICAL_MEMORY_OFFSET`。
- **修复方法**：由于实验规则禁止修改 `memlayout.h` 等非任务头文件，在 `code/kern/mm/pmm.c` 的 `//lab2 function1 2412172` 代码块顶部增加条件宏定义兜底：
  ```c
  #ifndef PHYSICAL_MEMORY_END
  #define PHYSICAL_MEMORY_END 0x88000000
  #endif
  ```
- **修复后验证**：重新执行 `/Users/wesley/大三上课程/操作系统/os2026.sh build`，零警告、零错误通过编译，QEMU 启动后正确输出 `physical memory map:` 并通过 `check_alloc_page() succeeded!`。

---

## Bug 2：RISC-V 压缩指令（`c.ebreak`）与 32 位非法指令（`.word 0x0000007b`）的 `sepc` 步进长度差异

- **阶段**：`task_03`（设计 `exception_handler` 的提示词与 Spec）
- **现象与潜在陷阱**：在 RISC-V 64GC（含 C 压缩指令集扩展）下，工具链默认会将汇编中的 `ebreak` 汇编为 2 字节（16 位）的压缩指令 `c.ebreak`（机器码 `0x9002`，最低两位为 `10`），而 `tools/test_lab3.c` 中的非法指令测试 `.word 0x0000007b` 则是 4 字节（32 位）标准指令（最低两位为 `11`）。若在 Prompt 中简单让 AI 对所有异常统一执行 `tf->epc += 4` 或统一 `tf->epc += 2`：
  - 若统一 `+4`，在 `c.ebreak` 后会跳过紧随其后的 2 字节指令，导致 `test_lab3_register_context` 中第一条寄存器赋值指令被跳过而测试失败；
  - 若统一 `+2`，在 4 字节非法指令 `.word 0x0000007b` 后 `sepc` 会指向该指令的后半个半字（`0x0000`，同样是非法指令），导致二次触发异常或执行错位。
- **解决策略**：在 `task_03` 的 `SPECIFICATION` 中明确指定 RISC-V 指令长度编码规则：读取 `*(const uint16_t *)(tf->epc)` 的低 2 位，`((inst & 0x3) != 0x3) ? 2 : 4`，动态决定 `tf->epc` 增加 2 字节还是 4 字节。
- **验证结果**：`make grade` 中 Test 1（`c.ebreak` 断点返回）和 Test 2（4 字节非法指令 `.word 0x0000007b` 返回）均一次通过。

---

## Bug 3：`interrupt_handler` 被单元测试直接传入无最高位标志的 `tf.cause = 15` 时的未知中断识别

- **阶段**：`task_03`（分析 `tools/test_lab3.c` 与 `tools/grade.sh`）
- **现象与潜在陷阱**：硬件触发中断时，`scause` 最高位（bit 63）为 `1`；但 `test_lab3.c` 的 `test_lab3_handler_fallbacks()` 在测试未知中断兜底逻辑时，直接构造了 `tf.cause = 15; interrupt_handler(&tf);`（未置位 bit 63），且 `grade.sh` 检查输出字符串 `"Unknown interrupt: cause 15"`。
- **解决策略**：在 `interrupt_handler` 中使用 `(tf->cause << 1) >> 1` 提取中断号 `cause`，使得无论 `tf->cause` 的最高位是否为 1，都能提取出低位原因码 `15`，并在 `default` 分支中同时输出 `cprintf("Unknown interrupt: cause %d\n", (int)cause);` 与 `print_trapframe(tf);`。
- **验证结果**：`make grade` Test 2 完整匹配 `"Supervisor software interrupt"`、`"Unknown interrupt: cause 15"` 与 `"cause    0x00000014"`，得分 `25/25`。
