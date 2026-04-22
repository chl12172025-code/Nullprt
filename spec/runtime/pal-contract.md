## 平台抽象层（PAL）契约 v0

本文件定义 `sys` 层对上层暴露的**稳定契约**（语义级），以便：\n
- 三平台实现一致\n
- 上层 `std` 不直接出现平台分支\n
- 安全/合规边界可在 `sys` 层集中约束\n

### 1. 错误模型
- 所有 `sys` API 返回 `Result<T, SysError>`（或等价）\n
- `SysError` 至少包含：`code`（平台码）、`kind`（统一分类）、`message`（可选）\n

统一 `kind`（最小集）：
- `NotFound`、`PermissionDenied`、`InvalidInput`、`TimedOut`、`Interrupted`、`AlreadyExists`、`Unsupported`、`Other`\n

### 2. `sys::fs`（最小）
- `open(path, flags) -> Handle`\n
- `read(handle, buf) -> n`\n
- `write(handle, buf) -> n`\n
- `close(handle)`\n
- `stat(path) -> Metadata`\n
- `list_dir(path) -> Iterator<DirEntry>`（MVP 可先回调式）\n

### 3. `sys::time`
- `monotonic_now() -> u64`（ns 或 ticks，需定义）\n
- `system_now() -> (sec, nsec)`\n
- `sleep(duration)`\n

### 4. `sys::thread` / `sys::sync`
- `spawn(entry, arg) -> ThreadHandle`\n
- `join(handle) -> ExitCode`\n
- `mutex_create/lock/unlock/destroy`\n
- `condvar_wait/notify`\n
- `atomic`：可由编译器内建或平台原语支持\n

### 5. `sys::process`
- `get_env(name)` / `set_env`（发行版可允许 set_env）\n
- `current_dir()` / `set_current_dir()`\n
- `exit(code)`\n

### 6. 合规约束（必须硬编码）
`sys` 层不得提供：
- 跨进程句柄枚举/打开并读写他进程内存\n
- 未授权注入/远程线程创建\n
- 修改硬件标识、伪装虚拟机环境\n

硬件信号读取（用于绑定）必须：
- 只读\n
- 可被全局配置禁用\n
- 返回最小化信息（建议哈希化后的 token）\n
