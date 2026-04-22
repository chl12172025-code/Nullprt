## ABI 与 FFI（互操作边界）

本章定义 NPRT 与外部世界交互的**最低公共接口**：C ABI。对 C++/Rust/Go/.NET/JVM/Python 等的互操作，统一约束为“以其官方/标准方式暴露为 C ABI（或等价稳定 ABI）后再接入”，避免引入不稳定/越权能力。

### 1. C ABI 支持范围（MVP）
- **extern 函数声明/导出**：从 NPRT 调用 C；从 C 调用 NPRT。
- **布局属性**：显式声明 `repr_c`（名称待定）保证与 C 一致的内存布局。
- **调用约定**：默认使用平台 C ABI（Windows x64、SysV AMD64、AAPCS64 等）。
- **链接**：静态库（`.lib/.a`）与动态库（`.dll/.so/.dylib`）。

#### 决策清单
- 名字修饰（name mangling）：C 导出必须禁用修饰；NPRT 内部符号可有修饰。
- 对齐与打包：是否支持 `pack(n)`？（建议限制，防止 UB）

### 2. 可 FFI 的类型集合（MVP 建议）
- 标量：整数、指针、`bool`（映射为 `uint8` 或 `_Bool` 需明确）。
- `struct`：仅含 FFI-safe 字段；无隐式 padding 变更（由 `repr_c` 约束）。
- `enum`：MVP 不建议直接暴露为 C；若需要，提供 `tag + union` 的显式表示。
- 字符串：以 `(ptr, len)` 或 NUL 结尾 `char*` 两种桥接类型提供。

#### 决策清单
- `bool` ABI：不同平台/编译器差异如何规避？（建议统一用 `u8`）
- `char` ABI：是否允许 `u32` 直接跨边界？

### 3. 外部函数接口语法（占位）
为保持“独立语法体系”，此处仅定义语义，不绑定具体关键字形态：
- 声明外部库/符号：`extern(c) { fn ... }`
- 导出符号：`export(extern_c, name="...") fn ...`

### 4. 动态加载（可选）
为避免平台差异暴露到用户代码，建议标准库提供：
- `dynlib.open(path)` / `dynlib.sym(name)`，返回类型安全包装（MVP 可只支持 `void*` + 显式转换）。

### 5. 与其他语言的互操作策略（合规版）
- **C++**：通过 `extern "C"` 导出薄封装。
- **Rust**：通过 `#[no_mangle] extern "C"` 导出。
- **Go**：通过 `cgo` 生成 C ABI shim（或反向由 C 调用 Go）。
- **.NET / Java**：通过 JNI / PInvoke 走官方边界（NPRT 提供生成 glue 的工具可选）。
- **Python**：通过 CPython C API / stable ABI（或 `ctypes` 加载 C ABI 库）。

### 6. 明确禁止的互操作能力（写入规格）
- NPRT 标准工具链 **不提供**：未授权跨进程注入、未授权读写他进程内存、绕过 OS 安全机制的 API。
- 仅允许：在**自进程**内进行动态加载/FFI 交互；所有调试与分析能力仅限开发者工具链且作用域为自进程或用户授权目标。
