# Final Status

Completed Tasks:

- Task 01 — DONE：实现page_init；子Agent Docker权限失败后由Leader整合运行，Test 1通过；依据 [Task 01 Verification](task_01.md)。
- Task 02 — DONE：Best-Fit实现、注册、Leader切换与20/20；依据 [Task 02 Verification](task_02.md)。
- Task 03 — DONE：交叉review完成，Best-Fit PASS、page_init NEEDS_FIX，发现Bug 2；依据 [Task 03 Verification](task_03.md)。
- Task 04 — DONE（Case 5运行时UNVERIFIED）：补描述符写入前边界检查；128MB、64MB及默认grade通过；小内存未进入固件，不能证明越界分支实跑；依据 [Task 04 Verification](task_04.md)。
- Task 05 — DONE：QEMU源码分析与调试方案；核心预测经Task 07验证，原交互脚本未直接运行；依据 [Task 05 Verification](task_05.md)。
- Task 06 — DONE：P0报告、Challenge和First-Fit分析；环境引用修订在Task 08完成；依据 [Task 06 Verification](task_06.md)。
- Task 07 — DONE：Leader两轮双gdb实际调试，记录Sv39翻译、Bare及旧代码TLB项；依据 [Task 07 Verification](task_07.md)。
- Task 08 — DONE：分支六问与演示稿，主线A～F命令经Leader批处理验证；用户手敲未练习；客户机侧日志尾部的 gdb 提示经复查为批处理模式产物（见下方）；依据 [Task 08 Verification](task_08.md) 与原始t8日志。
- Task 09 — DONE：Buddy实现、编译开关和占位运行，默认grade20/20；依据 [Task 09 Verification](task_09.md)。
- Task 10 — DONE：Buddy八组通过，M1～M3均检出；依据 [Task 10 Verification](task_10.md)。
- Task 11 — DONE：SLUB两层实现、占位运行，Leader补diff，默认grade20/20；依据 [Task 11 Verification](task_11.md)。
- Task 12 — DONE：SLUB九组通过，S1～S3均检出；S3先触发对齐断言；依据 [Task 12 Verification](task_12.md)。
- Task 13 — DONE：报告Buddy设计、八组测试与变异结论经Leader审读，未覆盖项如实列明；依据最新 [Task 13 Verification](task_13.md)。
- Task 14 — DONE：完整报告、提示词及此总结；发现 Task 08 记录不一致，经 Leader 复查更正；依据 [Task 14 Verification](task_14.md)。
- Task 15 — DONE：全文独立核对 PASS，2 条 NOTE 已采纳；依据 [Task 15](task_15.md)、[t15_report_review.md](evidence/t15_report_review.md)。


Tests:

命令均记录容器内的操作，运行者是Leader，本Report Agent未运行Docker/QEMU/gdb。切换配置时先清理旧目标文件是复现建议，不新增一次测试记录。

| 构建配置 | 已记录命令 | 结果 | 证据 |
|---|---|---|---|
| 默认Best-Fit | `make`、`make qemu`；`make grade` | page_init与Best-Fit自检通过，默认grade20/20 | [t4_qemu.txt](evidence/t4_qemu.txt)、[t4_grade.txt](evidence/t4_grade.txt)、[t11_grade_default.txt](evidence/t11_grade_default.txt) |
| Buddy | `make DEFS=-DLAB2_PMM_BUDDY`、`make qemu` | 八组通过，`buddy_check() succeeded!`、`check_alloc_page() succeeded!`；正常运行未见panic | [t10_buddy_qemu.txt](evidence/t10_buddy_qemu.txt) |
| Best-Fit上层SLUB | `make DEFS=-DLAB2_SLUB_TEST`、`make qemu` | 九组通过，`slub_check() succeeded!`；正常运行未见panic | [t12_slub_qemu.txt](evidence/t12_slub_qemu.txt) |

- 默认配置的64MB补测：`qemu-system-riscv64 ... -m 64M`，日志识别64MB并通过物理分配检查；不是一次64MB grade。证据 [t4_qemu_m64M.txt](evidence/t4_qemu_m64M.txt)。
- Case 5尝试：`qemu-system-riscv64 ... -m 2080K`，仅timeout终止，未出现OpenSBI横幅、未到待验证分支。证据 [t4_qemu_m2080K.txt](evidence/t4_qemu_m2080K.txt)。
- Buddy变异：M1禁用合并→borrowing initial blocks，304行计数恢复断言；M2反向拆分→splitting at every level，变异副本325行first==isolated；M3按n扣账→rounding组，229行整块页数断言。均检出，见 [t10_mutation.txt](evidence/t10_mutation.txt)。
- SLUB变异：S1空slab不退页→第3组后223行页数恢复断言；S2选级条件错误→第2组265行最小class断言；S3步长减半→第2组255行8字节对齐断言。不是第3组检测S3数据重叠，见 [t12_mutation.txt](evidence/t12_mutation.txt)。
- 双gdb：实际load的VA `0xffffffffc0207008` → PA `0x80207008` → 当次宿主指针 `0xffff70007008`；根表0x80205000、idx511、PTE0x200000cf；Bare store仍装软件TLB；写satp后0x80200038代码旧项命中。见 [t7_paged_qemu_gdb.txt](evidence/t7_paged_qemu_gdb.txt)、[t7_bare_qemu_gdb.txt](evidence/t7_bare_qemu_gdb.txt)、[t7_inasm_kern_entry.txt](evidence/t7_inasm_kern_entry.txt)。
- 演示主线：A～F交互命令由Leader写成批处理复核，host日志到haddr读取；客户机符号地址正确；日志末尾的 `Cannot execute this command while the target is running` 经加标记复查为 gdb 批处理模式产物，`continue` 已生效（`evidence/t8_marker_ucore_gdb.txt`）。见 [t8_demo_validation_qemu_gdb.txt](evidence/t8_demo_validation_qemu_gdb.txt)、[t8_demo_validation_ucore_gdb.txt](evidence/t8_demo_validation_ucore_gdb.txt)。
- 最终回归：新增 [final_qemu_default.txt](evidence/final_qemu_default.txt)、[final_qemu_LAB2_PMM_BUDDY.txt](evidence/final_qemu_LAB2_PMM_BUDDY.txt)、[final_qemu_LAB2_SLUB_TEST.txt](evidence/final_qemu_LAB2_SLUB_TEST.txt)，再次记录三种配置成功；[final_grade.txt](evidence/final_grade.txt) 再次给出默认20/20。以上文件由Leader生成，未据此编造新的命令参数或编译器诊断。
- 编译结论按各Task Verification为编译器无警告；t11/t12的make日志含Clock skew/未来时间戳提示，不应改写为“整份日志没有warning”。QEMU由超时/信号停止，停止本身不等于测试通过。

Remaining Problems:

- Case 5运行时UNVERIFIED；检查代码已修复并静态核对。小内存固件启动限制的约34MB解释仍为推测，精确阈值未验证。
- 本人需按 [分支任务演示步骤.md](分支任务演示步骤.md) 练习三终端。主线批处理验证不等于现场熟练；可选Bare/[K]补充段未按演示稿另跑一轮。
- Task 08 Verification 最初写“全部命令无报错”，但客户机 t8 日志有 `Cannot execute this command while the target is running`；已更正。复查：四次运行都有，出现在 `continue` 之后，`continue` 已生效；gdb 内部成因待确认。
- Buddy：未运行会panic的非法调用；order 0已分配头与普通块内页无法可靠区分，误传块内页不保证检出。MAX_ORDER整块、区域外伙伴、同阶多候选分配和总量够但缺大块等情况未专门独立动态覆盖；检查分支和具体限制见报告。
- SLUB：未动态测试重复释放/非对象地址/伪造magic等panic分支、多CPU、每类新slab耗尽以及Buddy+SLUB组合。S3对≥16字节级的数据重叠是否由第3组检出未单独验证。
- 变异测试全部检出仅对选定六种错误成立，不构成无bug证明；SLUB的页头填充开销（2048级一页1对象、1024级3对象）保留为简化设计限制。
- QEMU 8.2.2 satp变化是否总刷新TLB仍是推测，未核对源码；真实CPU TLB行为未实测。
- `-d in_asm`把0x80200038的TB显示为连续 `sw a2,96(s0)` 的原因待确认；不能据此断言实际执行相同sw。
- 尚未推送 GitHub：Leader 已在本地 `lab2` 分支提交（作者 fcsguyxcw），未推送；是否推送由用户决定。
- 尚未预约检查：由用户决定/操作；不把完成报告写成已经提交或预约。

AI Collaboration Summary:

Leader使用Claude Code，按PROMPT/RELY/GUARANTEE/SPECIFICATION拆分任务，通过Herdr调度Codex实现、静态分析、review和报告。最初子Agent因Docker socket权限失败，后续明确由Leader在os2026-dbg实跑并保存输出。交叉review找出规格遗漏的描述符容量分支，Task 04修复，未覆盖的运行分支继续标UNVERIFIED。

Buddy和SLUB各分实现/测试两份提示词，测试任务不得改分配器；Leader进一步以M1～M3、S1～S3检验测试。Task 11的diff证据遗漏由Leader补齐。Task 05源码预测经Task 07调试核对，Task 08演示主线经批处理复核；报告保留旧TLB发现、反汇编疑点和尾部错误，不虚构用户对话或未执行的实验。Task 14补齐报告、四份历史提示词和最终总结，完成静态一致性核对，不替代Leader复核和用户答辩练习。

Files Changed:

以下源码清单相对课程lab2.zip原始骨架，路径以仓库根目录为基准；对应Task 01/02/04/09/10/11/12实现与Leader集成记录，并由 [final_diff_check.txt](evidence/final_diff_check.txt) 确认：

- `code/kern/mm/pmm.c`：page_init及写描述符前的容量检查；默认选择Best-Fit；Buddy与SLUB编译开关接入。
- `code/kern/mm/best_fit_pmm.c`：Best-Fit实现和注册，保留两个框架测试函数。
- 新增 `code/kern/mm/buddy_pmm.h`、`buddy_pmm.c`：伙伴分配器与八组检查。
- 新增 `code/kern/mm/slub.h`、`slub.c`：两层对象分配器与九组检查。
- 报告交付：`report/report.md`、`report/prompt.md`、`report/ai_logs/lab2/`内任务/审查/运行证据、演示稿及本总结；它们不是新增内核接口。

（Task 14 的写入范围是 `report/report.md`、`report/prompt.md`、本文件；之后 Leader 按 Task 14/15 的复核结果做了少量更正，见对应 task 文件。）

Status: COMPLETE（实现、测试、报告均已完成；Case 5 运行时 UNVERIFIED；推送与预约检查待用户操作）
