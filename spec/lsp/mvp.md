## NPRT Language Server（LSP）MVP 范围与依赖

目标：让 NPRT 在主流编辑器（优先 VS Code）中具备“可用”的开发体验：语法诊断、跳转、引用、补全、格式化（基础）。

### 1. 运行形态
- 可执行文件：`nprt-lsp`（名称可调整）\n
- 通信：stdio（MVP）\n
- 协议：Language Server Protocol（JSON-RPC 2.0 over stdio）\n

> 独立性要求：`nprt-lsp` 自带 JSON 解析/序列化实现（不得依赖现有语言运行时）。

### 2. MVP 必须支持的请求/通知
#### 初始化与文档生命周期
- `initialize` / `initialized`\n
- `shutdown` / `exit`\n
- `textDocument/didOpen`\n
- `textDocument/didChange`（增量变更可选；MVP 可先全量文本）\n
- `textDocument/didClose`\n

#### 诊断（Diagnostics）
- 基于语法解析错误（lexer/parser）\n
- 基于名称解析错误（未定义符号、重复定义）\n
- 类型检查错误（最小类型系统）\n

实现方式：
- `textDocument/publishDiagnostics`\n

#### 跳转与引用
- `textDocument/definition`\n
- `textDocument/references`\n
- `textDocument/documentSymbol`\n

#### 补全（Completion）
- `textDocument/completion`\n
- MVP 补全来源：\n
  - 当前作用域局部变量\n
  - 当前模块符号\n
  - import 的公开符号\n

#### 悬停（Hover）（可选但建议）
- `textDocument/hover`：显示符号类型、定义位置摘要

#### 格式化（Formatting）
- `textDocument/formatting`\n
MVP 可实现：缩进、空格、换行的稳定化（不追求完美）\n

### 3. 明确不进 MVP 的 LSP 能力
- `rename`（需要全工程引用精确更新）\n
- `codeAction` / `quickFix`\n
- `semanticTokens`（语义高亮）\n
- 跨包的增量索引与缓存（先做单包/单 workspace）\n

### 4. 内部依赖与模块拆分（实现指导）
#### 4.1 Parser 前置依赖
LSP 复用 `aegc` 的 lexer/parser（推荐抽成共享库模块）：\n
- 输出 AST + 位置范围（span）\n
- 支持错误恢复（尽量在一个文件内产出多个错误）\n

#### 4.2 符号表与索引
- 文件级符号表：定义/引用集合\n
- 模块级索引：导出符号、import 图\n
- workspace 级：以包为单位缓存解析结果（MVP 可先“打开的文件集合”）\n

#### 4.3 最小类型检查
- 只需要支撑：\n
  - 基本类型\n
  - `let` 推断\n
  - 函数签名\n
  - 简单表达式\n

### 5. 验收标准（端到端）
- 打开 `examples/hello_nprt/src/main.nprt`：\n
  - 无崩溃\n
  - 语法错误能在 200ms 内反馈（冷启动可放宽）\n
  - `definition/references/completion` 可用\n

### 6. 与构建系统集成（后续）
- 可选读取 `NPRTcfg` 获取 `build.features/targets`，用于条件编译诊断一致性。\n
- MVP 可先忽略 `npkg` 下载，假设依赖已在工作区可见（或只做本地缓存索引）。
