## C ABI 互操作测试集（跨平台验收）

本目录定义 NPRT 与 C ABI 互操作的验收用例。目标是让 `aegc` 的 FFI 与布局属性在 Windows/macOS/Linux 上可验证一致。

### 1. 测试原则
- 每个用例由 **C 侧导出函数** + **NPRT 侧调用/导出** 两部分组成。
- 所有二进制交互必须通过 **C ABI**（`extern "C"`）。
- 产物覆盖：动态库（优先）+ 静态库（可选）。

### 2. 用例清单（MVP 必测）
#### ABI-01：标量传参返回
- `i32/u32/i64/u64/usize` 传参、返回值一致。

#### ABI-02：结构体布局（`repr_c`）
- 字段顺序、对齐、padding 与 C 一致。\n
- 覆盖：混合字段、显式对齐、嵌套 struct。

#### ABI-03：指针与切片桥接
- `const u8* + len` 传递 buffer；C 侧计算 hash/校验后返回。

#### ABI-04：字符串桥接
- NUL 结尾 `char*`（UTF-8）传参。\n
- `ptr+len` 字符串传参（避免 NUL 限制）。

#### ABI-05：回调（函数指针）
- NPRT 传递回调给 C；C 调用回调并返回结果。\n
- 约束：回调不得捕获不可移动环境（MVP 可限制为裸函数指针）。

#### ABI-06：动态链接与符号名
- `dlopen/LoadLibrary` 加载库、解析符号并调用（由 NPRT std::dynlib 封装或测试工具实现）。\n
- 验证导出名无修饰（按 `export(extern_c, name=...)` 规则）。

### 3. 平台构建说明（C 侧）
- Windows：生成 `.dll`（MSVC/clang-cl）\n
- Linux：生成 `.so`（clang/gcc）\n
- macOS：生成 `.dylib`（clang）\n

> 工具链独立目标下，未来可由 `aegc` 自带 C shim 编译器或内建 minimal C compiler；MVP 阶段允许使用系统 C 编译器构建测试用例。
