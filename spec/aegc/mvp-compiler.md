## `aegc` MVP 编译器：语言子集、IR、后端与里程碑

本文件将 “Stage1 MVP 能编译到原生机器码” 细化为可验收的工程规格。

### 1. MVP 语言子集（必须实现）
#### 1.1 模块与符号
- 单包内模块：`src/` 下文件映射模块（MVP 可先 1 文件 1 模块）。
- `import`（语义）：导入模块符号；避免循环依赖（MVP 可先禁止）。

#### 1.2 声明
- `fn`：函数定义、参数、返回类型。
- `let`：不可变绑定；`let mut`：可变绑定（若采用）。
- `struct`：字段结构体（无泛型也可）。
- `enum`：带 payload 的枚举；用于 `match`。
- `const`：编译期常量（MVP 可先只支持整数/字符串）。

#### 1.3 表达式与语句
- 字面量：整数、字符串、布尔。
- 运算：算术、比较、逻辑。
- 控制流：`if/else`、`while`（或 `loop`）、`return`。
- 函数调用；方法调用可延后（先用自由函数）。

#### 1.4 错误模型
- 内建 `Result<T,E>`（可作为标准库类型，编译器识别 `?` 语法糖可延后）。

#### 1.5 所有权与借用（MVP 最小集）
- 移动语义 + 借用互斥：同一作用域 `&mut` 独占；`&` 可共享且不能与 `&mut` 共存。
- 禁止借用逃逸：MVP 先限制为“引用不得存入堆结构/不得返回”（后续再放开到生命周期系统）。

> 备注：该限制能显著降低首版借用检查复杂度，确保先跑通端到端编译器。

### 2. 非 MVP（明确延后）
- 完整泛型与 trait/interface 系统（先把 monomorphization 方案写入设计，但不强制实现）。
- 过程宏、async/await、完整覆盖率/静态分析、跨 crate 增量编译。

### 3. IR 与后端策略（可落地优先）
#### 3.1 IR 选择（建议）
MVP 推荐实现一个自研的 **NPRT-IR（SSA-like）**，原因：
- 避免依赖外部 LLVM/MLIR（满足独立性要求）
- IR 设计可为后续混淆/自保护变换提供稳定切入点

NPRT-IR 最小能力：
- 基本块、跳转、条件分支
- 局部 SSA 值（或显式虚拟寄存器）
- 调用约定抽象（支持 C ABI）
- 栈对象与 load/store

#### 3.2 后端（MVP：单后端先行）
MVP 推荐从 **x86_64（Windows MSVC 或 Linux SysV 二选一）** 起步：
- 生成 `.obj/.o`，由系统链接器完成链接（MVP 可先依赖系统 linker；最终可逐步自研 linker 或随工具链分发自有 linker）。\n

> 独立性解释：MVP 阶段允许借用系统链接器作为“平台组件”；最终独立交付时再替换为自有实现/分发组件（见 `bootstrap-strategy`）。

#### 决策清单
- MVP 首选平台：`x86_64-windows-msvc` vs `x86_64-unknown-linux-gnu`。
- 目标文件格式：PE/COFF vs ELF vs Mach-O 的优先级。

### 4. `aegc` 命令行（MVP）
- `aegc build`：读取 `NPRTcfg`，调用 `npkg`，编译并链接。
- `aegc check`：只做解析/名称解析/类型检查（不出二进制）。
- `aegc run`：`build` 后运行产物。
- `aegc fmt`（可选占位）：调用格式化器或内置格式化。

### 5. 里程碑与验收用例
#### Milestone M1：词法/语法/AST
- 能解析 `examples/hello_nprt/src/main.nprt`（占位语法）并输出 AST（debug）。

#### Milestone M2：名称解析 + 类型检查（最小）
- `let`/`fn`/`if`/`while` 能通过类型检查；错误信息含行列号。

#### Milestone M3：IR 生成 + 解释器（可选加速）
- 先实现 IR 解释器跑通 `hello`（便于调试后端之前的语义）。

#### Milestone M4：x86_64 代码生成 + 链接
- 生成可执行文件，返回退出码；能打印字符串（先通过最小 sys 调用封装或 C ABI 调用 `puts`）。

#### Milestone M5：C ABI FFI
- 从 NPRT 调用 C 动态库函数（见 `tests/abi/` 用例）。

#### Milestone M6：release profile 基础保护接入
- 能在 IR 级开启至少 1 个变换（例如 CFG 变换）并保持测试通过。
