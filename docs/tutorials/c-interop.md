# 与 C 语言交互

可通过 C ABI 与外部库互操作：

1. 在 Nullprt 中声明外部函数签名  
2. 通过系统链接器链接 `.a/.lib/.so/.dylib`  
3. 使用稳定 ABI 类型（整数、指针、结构体）

示例：`examples/c_interop/`。
