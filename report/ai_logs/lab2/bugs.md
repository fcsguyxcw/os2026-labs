# Lab2 Bug 与问题记录

## 问题 1：子 Agent 沙箱无法访问 Docker（环境问题，非代码 bug）

- 现象：Task 01、Task 02 中 Codex 执行 `docker run ... os2026-dbg ...` 均返回退出码 1，提示访问 OrbStack 的 `docker.sock` permission denied，编译未开始。
- 失败命令：`docker run --rm -v "$PWD:/code" -w /code os2026-dbg bash -c 'make 2>&1; timeout 10 make qemu'`（证据 `evidence/t1_qemu.txt`、`evidence/t2_compile.txt`）。
- 原因：Codex CLI 的命令沙箱不允许访问 Docker 的 Unix socket。
- 处理：不放宽 Codex 沙箱权限；改为“子 Agent 实现 + 静态自查，Leader 在 Docker 中编译、运行、评分”。后续任务的 prompt 里直接写明这一分工。
- 验证：Leader 运行结果见 `evidence/t3_qemu.txt`、`evidence/t3_grade.txt`（20/20）。

## Bug 2：page_init 未检查描述符数组是否放得下（spec 遗漏，Review 发现）

- 现象：交叉 review（Task 03，`evidence/t3_review_page_init.md`）指出，`page_init` 先把 `pages[0 .. npage-nbase)` 全部置 `PG_reserved`，之后才判断有没有空闲页；设备树报告的内存很小时，描述符数组末端会超出内存，循环写越界内存。
- 失败命令/测试：默认 128MB 的 `make qemu`、`make grade` 都通过，测试无法发现；由 review 推导得出（size=0x208000 时数组物理区间 `[0x80207000, 0x8020c140)` 超过末端 `0x80208000`）。
- 原因：Leader 在 Task 01 SPECIFICATION 里只写了 Case 3“数组之后没有完整页”，漏掉了“数组本身放不下”。AI 严格按 spec 实现，所以 spec 的漏洞原样进了代码。
- 修复：Task 04 在 spec 中新增 Case 5（写描述符之前检查 `PADDR(&pages[npage - nbase]) > mem_end`，成立则 panic），由 Debug Agent 修改 `page_init`。
- 修复后验证：128MB 运行与 `make grade` 20/20 通过（`evidence/t4_qemu.txt`、`t4_grade.txt`）；越界分支因 OpenSBI 0.4 需要至少约 34MB 内存才能启动，无法在 QEMU 中复现，只经静态审查。
- 教训：spec 的边界分支要从“每一步用到的内存是否在范围内”去枚举，而不只是看最终结果；测试全绿不代表 spec 完整。
