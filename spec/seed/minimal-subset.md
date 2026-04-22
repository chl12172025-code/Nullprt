## aegc0（seed）支持的 Nullprt 极小子集（Minimal Subset v0）

本文件定义 Stage0 种子编译器 `aegc0` 必须支持的 NPRT 语法/语义子集。\n
目标：让 `aegc0` 能把该子集源码翻译为等价 C 代码，并用系统 C 编译器生成可执行文件，从而编译出 `aegc1`。

### 1. 类型
- `i32`：32 位有符号整数（二进制补码；溢出语义：与 C 的实现相关，seed 阶段按 C 行为）\n
- `u64`：64 位无符号整数\n

不支持：浮点、字符串、数组、指针类型（除 FFI 需要的原生指针）；自定义类型（struct/enum）。

### 2. 顶层项（items）
#### 2.1 外部函数声明（FFI）
语义：声明一个 C ABI 函数符号，供 NPRT 代码直接调用。\n
仅支持：\n
- `extern \"C\" fn <name>(<params...>) -> <type>;`\n

参数/返回类型仅允许：`i32`、`u64`（MVP seed 先固定这两种）。

#### 2.2 函数定义
语义：定义一个函数。\n
- `fn <name>(<params...>) -> <type> { <stmts...> }`\n

必须支持入口：\n
- `fn main() -> i32 { ... }`\n

### 3. 语句（statements）
- 变量声明：`let <ident> : <type> = <expr>;`\n
- 赋值：`<ident> = <expr>;`\n
- if：`if (<expr>) { <stmts...> }`（可选 else：`else { ... }`）\n
- while：`while (<expr>) { <stmts...> }`\n
- return：`return <expr>;`\n
- 表达式语句（函数调用）：`<call_expr>;`\n

### 4. 表达式（expressions）
- 整数字面量：十进制（可选支持 `0x`）\n
- 变量引用：`<ident>`\n
- 二元算术：`+ - * / %`\n
- 比较：`== != < <= > >=`\n
- 逻辑：`&& || !`\n
- 括号：`(expr)`\n
- 函数调用：`name(arg1, arg2, ...)`\n

### 5. 词法规则（tokenization）
- 注释：`//` 行注释\n
- 标识符：`[A-Za-z_][A-Za-z0-9_]*`\n
- 关键字：`fn let extern return if else while`（以及 `\"C\"` 字符串字面量）\n

### 6. 明确不包含的特性（必须拒绝并报错）
- 泛型、trait/interface\n
- 模式匹配、宏\n
- 所有权/借用系统（seed 不做借用检查）\n
- 并发/异步\n
- 自定义类型（struct/enum）\n
- 模块系统/import\n

### 7. 诊断要求（seed 级别）
`aegc0` 至少输出：\n
- 错误位置（行列）\n
- 错误类别（期望 token、未定义变量、类型不匹配等）\n

### 8. 与 C 的映射（规范）
- `i32` → `int32_t`\n
- `u64` → `uint64_t`\n
- `fn main() -> i32` → `int32_t main(void)`\n
- `extern \"C\" fn` → C `extern` 声明\n

### 9. 示例（子集）
```text
extern \"C\" fn puts(x: u64) -> i32;

fn main() -> i32 {
  let x: i32 = 1;
  let y: i32 = 2;
  if (x < y) {
    return 0;
  } else {
    return 1;
  }
}
```
