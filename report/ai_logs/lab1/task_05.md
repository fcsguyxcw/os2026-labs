# Task 05：Review 实验报告与代码改动

执行者：pi（Review Agent，deepseek-flash，Herdr 面板 w9:p8）

## PROMPT

你是 OS Lab1 的 Review Agent，只做审查，不修改任何已有文件。仓库根目录（Windows 路径）：`\\wsl$\Ubuntu-24.04\home\wesly\courses\os2026`，以下路径均相对于它。

请阅读：
- 被审查对象：`report/report.md`、`report/images/*.png`（查看图片内容）、`code/Makefile`。
- 依据：`实验报告模板.md`；`report/ai_logs/lab1/task_01.md`～`task_04.md`（其中 RELY / Verification 是已核实事实）；`report/ai_logs/lab1/bugs.md`；`report/ai_logs/lab1/evidence/` 下全部 txt（原始命令输出，唯一事实来源）；助教原版 Makefile 位于 `\\wsl$\Ubuntu-24.04\home\wesly\lab1x\lab1\Makefile`。

逐项检查：
1. 报告中每个地址、寄存器值、指令、输出文字是否能在 evidence 中找到原文；列出所有对不上的地方（报告原句 + evidence 文件与实际内容）。
2. 是否回答了练习 1 的两个问题（操作 + 目的）和练习 2 的全部问题（最初几条指令的地址、功能、调试过程、观察结果）。
3. 是否覆盖指导书五项报告要求：整体逻辑线；核心函数/模块理解；实验知识点与 OS 原理对应（含义、关系、差异）；OS 原理中重要但本实验未对应的知识点；练习答案。
4. 章节是否与模板一致；有无残留的模板占位符（如 `[目的1：...]`、`学号1-姓名1` 之外的示例文字）。注意：`学号-姓名（待填写）` 是有意保留的，不算问题。
5. 每张图片是否在正文中被引用，图片内容是否与其说明一致，图片是否为 evidence 原文截取。
6. `code/Makefile` 相对助教原版是否只有 3 行变化（gdb 程序名、qemu/debug 的 `-kernel`），改动理由在报告中是否解释正确。
7. 有无夸大、无依据或与指导书事实相冲突却未说明的表述。

输出：把审查结果写入新文件 `report/ai_logs/lab1/review_01.md`，格式：
```
# Review 01
## 结论：PASS / NEEDS_FIX
## 问题清单
- [严重|一般|建议] 位置（报告章节或行）：问题描述；依据（evidence 文件 + 原文）；建议修改
## 已核对通过的项目
```
只列有依据的问题，不要为凑数提问题。写完后在终端只回复一行：`REVIEW_DONE <结论> <问题数>`。

## RELY

- 已核实事实见 task_01～task_04 的 Verification；evidence 为原始输出。
- 指导书练习原题与报告要求见 task_04.md 的 RELY。

## GUARANTEE

- `review_01.md` 存在，结论明确，每个问题都带 evidence 依据。
- 未修改任何其他文件。

## SPECIFICATION

- Preconditions：Task 04 完成。
- Postconditions：仅新增 `report/ai_logs/lab1/review_01.md`。
- Edge cases：无法打开图片时，在 review 中注明“未能查看图片”，不要猜测内容。

## Agent Result

pi 写出 `review_01.md`（2026-10-07 约 15:11），终端回复 `REVIEW_DONE`。结论 NEEDS_FIX，2 项问题：[一般] 第 266 行“报告生成前后哈希核对”无记录；[建议] 第 258 行退出码 124 取自 Task 01。其余地址、寄存器、指令、图片、Makefile 3 行改动、五项报告要求均核对通过。

## Verification

Leader 阅读 review_01.md，两项问题依据成立，派 Task 06 修订。

状态：**DONE**。
