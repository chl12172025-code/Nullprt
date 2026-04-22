## NPRT 运行时/标准库模块边界（Module Map v0）

本文件把 `docs/50-runtime-stdlib.md` 的分层原则落到“可直接建仓实现”的模块清单与接口边界，避免后续实现时循环依赖与平台泄漏。

### 1. 模块分层（从底到顶）
```text
core  -> alloc -> rt -> sys(pal) -> std
                 \-> crt_shim (可选：C 运行时适配)
```

### 2. `core`（无分配、无 OS）
职责：
- 基础类型、算术、比较、`Option/Result`\n
- 字符串视图（只读 slice 语义，`str_view`）\n
- trait/interface 基元（若有）\n

禁止：
- 堆分配\n
- 文件/线程/网络\n

必须公开的最小 API（语义）：
- `core::result::Result<T,E>`\n
- `core::option::Option<T>`\n
- `core::mem::{size_of, align_of}`（编译器内建即可）\n

### 3. `alloc`（分配抽象与容器基础）
职责：
- 全局分配器 trait/interface\n
- `Box/Vec/String`（或等价）\n

接口边界：
- `alloc` 只能依赖 `core`\n
- `alloc` 通过 `rt` 或“编译器注入”获得默认分配器实现\n

最小 API（语义）：
- `alloc::alloc::{alloc, dealloc, realloc}`（低层）\n
- `alloc::vec::Vec<T>`\n
- `alloc::string::String`（UTF-8）\n

### 4. `rt`（runtime）
职责：
- 程序启动/退出（`main` 桥接）\n
- `panic`/`abort` 实现与策略\n
- TLS 初始化、栈保护（可选）\n
- 默认全局分配器注册\n

边界：
- `rt` 可依赖 `core/alloc/sys`\n
- `rt` 提供最小 “不带格式化” 的日志/终止能力（避免早期依赖 std）\n

### 5. `sys`（PAL：平台抽象层）
目标：把 Windows/macOS/Linux 的差异封装成一组稳定接口。

子模块建议：
- `sys::mem`：虚拟内存（reserve/commit/protect）\n
- `sys::thread`：线程创建/join、TLS、原语\n
- `sys::fs`：句柄、读写、目录、元信息\n
- `sys::time`：monotonic/system time、sleep\n
- `sys::net`：socket、poll 抽象（MVP 可先阻塞 IO）\n
- `sys::process`：env、cwd、exit\n
- `sys::crypto`（可选）：若需要 OS entropy（随机数种子）\n

约束：
- `sys` 不暴露“修改硬件标识/绕过 OS 安全/跨进程注入”等禁止能力\n
- 硬件信息读取仅提供只读与最小化采集接口（用于绑定），并支持用户显式关闭\n

### 6. `std`
职责：
- 用户常用 API：fs/path, net, time, thread/sync, io, env, process\n
- 高层错误类型与格式化输出（可选）\n
- `std::dynlib`（动态库加载）\n

边界：
- `std` 依赖 `core/alloc/rt/sys`\n
- `std` 应尽量以纯库实现为主，平台调用通过 `sys` 完成\n

### 7. 发行版/开发者版差异如何落到模块
建议采用“双层门控”：
- **编译期开关**：`cfg(profile="release")` 决定哪些模块/符号参与编译\n
- **链接期剔除**：发行版不链接开发者调试/分析模块\n

模块建议：
- `std_dev`：仅开发者工具链启用（性能分析、断点管理等）\n

### 8. MVP 实施顺序（建议）
1) `core`\n
2) `alloc` + 最小分配器\n
3) `sys::process/sys::time/sys::fs`（先支撑基本 IO）\n
4) `rt`（启动/panic）\n
5) `std::io/std::fs/std::thread/std::sync`（按编译器需求补齐）\n
6) `std::net`（后置）\n
