## Bootstrap 独立性边界（Stage0 vs Stage1+）

本文件明确记录“自举阶段允许的外部依赖”与“最终交付物必须满足的独立性要求”，以避免实现偏离目标。

### 1. Stage0（aegc0 / seed）允许依赖（仅开发/CI）
Stage0 的目标是得到第一个可执行的 NPRT 编译器 `aegc1`。\n
因此 **Stage0 允许依赖系统组件**，但必须满足：\n
- 仅用于开发/CI（bootstrap-only）\n
- 不进入最终发行安装包\n
- 可被版本锁定、可复现\n

Stage0 允许依赖：
- 系统 C 编译器（Windows: `cl`/`clang-cl`；macOS/Linux: `clang`/`gcc`）\n
- 系统链接器（由 C 编译器驱动）\n
- 系统头文件与基础 C 运行时（用于生成可执行文件）\n

Stage0 不允许依赖：
- 任何需要最终用户预装的语言运行时（例如 Java/.NET/Python/Rust 等）\n
- 任何用于越权/攻击的能力（与 `docs/60-security-release.md` 禁止项一致）\n

### 2. Stage1+（aegc1 及以上）最终交付物要求
最终用户获得的 Nullprt 工具链（Stage1+）必须满足：
- 工具链由 **Nullprt 源码生成**（自举）\n
- 安装后 **不需要** 系统预装任何其他语言环境或工具链\n
- 不包含 C 代码或其他语言代码作为必需运行组件\n
- 编译器、标准库、运行时、包管理器、LSP 等均为 NPRT 自主实现并随工具链分发\n

### 3. 允许的“平台组件”与替换策略
由于跨平台系统编程的现实，最终交付物仍会使用 OS 提供的：\n
- 系统调用与基础 API（不可避免）\n
- 动态加载器/系统 loader（运行时加载动态库时）\n

在工具链层面（构建/链接）：
- Stage1 可暂时使用系统 linker 作为平台组件\n
- 进入“独立发行”阶段时，应逐步替换为随工具链分发的 linker 或自研 linker\n

### 4. 交付物切分（必须）
- `src/seed/*`：bootstrap-only，不进入最终发行包\n
- `src/self/*`、`src/pal/*`、`src/pkg/*`、`src/lsp/*`：最终交付物来源\n
