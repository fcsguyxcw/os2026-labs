# Lab 3 AI 协作总览（Overview）

- **实验名称**：Lab 3：断，都可以断（RISC-V 中断与异常处理机制）
- **小组成员**：2412172-许城玮（单人小组）
- **仓库与分支**：`/Users/wesley/大三上课程/操作系统/os2026-labs`，分支 `lab3`
- **运行环境**：macOS（Apple Silicon）+ OrbStack Docker 镜像 `os2026-dbg`（`riscv64-unknown-elf-gcc 13.2.0`、`QEMU 4.1.1`）

## 任务拆分与依赖关系

| 任务编号 | 模块 / 任务名称 | 修改文件 | 依赖 | 状态 |
|---|---|---|---|---|
| `task_01` | Lab 2 依赖迁移：`page_init()` 与 `best_fit_pmm_manager` | `code/kern/mm/pmm.c`、`code/kern/mm/best_fit_pmm.c` | 无 | DONE |
| `task_02` | 功能模块一：Trap 上下文恢复（`RESTORE_ALL`）与 Trap 入口（`__alltraps`） | `code/kern/trap/trapentry.S` | `task_01` | DONE |
| `task_03` | 功能模块二：中断处理（`interrupt_handler`）与异常处理（`exception_handler`） | `code/kern/trap/trap.c` | `task_01`, `task_02` | DONE |
| `task_04` | 综合评测、QEMU 实机运行验证与 Spec 代码审查 | 只读审查与评测证据收集 | `task_01`~`task_03` | DONE |
