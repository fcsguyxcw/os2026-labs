# Task 14：报告收尾 · SLUB 一节、知识点、总结与 final_summary

执行者：Codex w1:p3（Report Agent）
Leader 发出时间：2026-10-24（北京时间）

## PROMPT

你是 OS Lab2 的 Report Agent。仓库 `/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab2`。

任务：
1. `report/report.md`：
   - “功能模块二 · 任务四：SLUB”替换 TODO，写法与 Buddy 一节一致：两层结构（第一层 `alloc_pages`/`free_pages`，第二层 size class + slab + 对象内嵌 freelist + 大对象）、slab 页内布局与 `object_offset` 的取法及其空间代价（Task 11 Verification 的 NOTE：2048 级每 slab 1 个对象、1024 级 3 个，`kmalloc(PGSIZE)` 占 2 页）、为什么不在 `struct Page` 里存数据（与 Buddy 共存时的 `property` 冲突）、接入方式（`make DEFS=-DLAB2_SLUB_TEST`）、两份 prompt 的要点（引用 `prompt.md`）、9 组测试覆盖与有效性（用 `evidence/t12_mutation.txt` 的 S1～S3 结果说明，**如实写 S3 是被对齐断言检出、不是被“数据不重叠”组检出**）、未覆盖情况、实现迭代。
   - “三、实验整体逻辑分析”中 Buddy/SLUB/分支任务“见后文（进行中）”的句子改成实际内容的简短概括。
   - “五、测试与验证”补 SLUB 的命令与实际输出（`evidence/t12_slub_qemu.txt` 原样短引用）；补一个“三种构建配置汇总”小表（默认 / `LAB2_PMM_BUDDY` / `LAB2_SLUB_TEST`：管理器、运行的检查、结果、证据文件）。
   - “六”中：bug 一节补本轮真实出现的问题（只限日志里有的：如 Task 11 未生成 diff 证据由 Leader 补；变异测试说明测试有效但不证明无 bug；satp 旧 TLB 表项是环境层面的发现而非代码 bug——按 `bugs.md` 和 task 日志如实写，不要编造）；知识点表补充 Buddy（伙伴系统、内部碎片）、SLUB（对象缓存、slab）、Sv39 多级页表与大页、TLB 与 sfence.vma、satp 与地址空间切换，每项写“实验中的含义 / 原理中的含义 / 关系与差异”；“OS 原理中重要但本实验没有对应的知识点”核对并补全；“AI 协作开发的经验”补全（四段式 spec 漏分支导致 Bug 2、交叉 review、子 Agent 权限限制下的分工、变异测试检验测试本身、演示稿经实测验证）。
   - “实验基本信息”完成日期改为 `2026-10-24`；删除所有剩余 TODO（确实没有依据的写“未验证”或“待确认”并说明原因）。
2. `report/prompt.md` 末尾逐字追加 Task 11、Task 12、Task 13、Task 14 的四段式提示词，格式与已有小节一致。
3. 新建 `report/ai_logs/lab2/final_summary.md`，格式参照 lab1（`git show lab1:report/ai_logs/lab1/final_summary.md`）：Completed Tasks（Task 01～14 一行一个，状态 DONE/UNVERIFIED 以各 task 文件 Verification 为准）、Tests（每种构建配置的命令与结果、证据文件）、Remaining Problems（Case 5 运行时未验证、演示需用户本人练习、Buddy/SLUB 未覆盖项、8.2.2 推测、`sw a2,96(s0)` 原因待确认、尚未推送 GitHub、未预约检查——后两项写“由用户决定/操作”）、AI Collaboration Summary、Files Changed（相对 `lab2.zip` 原始骨架：改动 `kern/mm/pmm.c`、`kern/mm/best_fit_pmm.c`，新增 `buddy_pmm.[ch]`、`slub.[ch]`）、`Status:`。

操作要求：只改 `report/report.md`、`report/prompt.md`，新建 `final_summary.md`；不改代码、不改其它日志，不 git commit / push，不运行 Docker。事实只来自 `report/ai_logs/lab2/` 与源码。完成后回复 ≤10 行，列出你拿不准的地方。

## RELY

- `task_01.md`～`task_13.md`、`bugs.md`、`overview.md`、`evidence/`（重点 `t11_*`、`t12_*`）；源码 `code/kern/mm/slub.c`、`slub.h`、`pmm.c`。
- 报告要求（`实验指导书/lab2/lab2_5_labs_requirement.md`）第 1～7 条。
- lab1 的 final_summary 格式：`git show lab1:report/ai_logs/lab1/final_summary.md`。

## GUARANTEE

- `report.md` 无 TODO；`prompt.md` 新增内容与 task 文件逐字一致；`final_summary.md` 每条状态都能在 task 文件中找到依据。

## SPECIFICATION

- Preconditions：Task 01～13 完成；SLUB 测试已通过且变异测试完成。
- Postconditions：`grep -n TODO report/report.md` 无结果；`git diff --stat` 只涉及上述 3 个文件。
- Edge cases：各处说法不一致时以 evidence 为准，在回复中列出。

## Agent Result

（待填写）

## Verification

（待填写）
