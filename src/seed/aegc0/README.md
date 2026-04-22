## aegc0（seed compiler）

`aegc0` 是 Stage0 的 bootstrap-only 种子编译器：\n
输入：Nullprt 极小子集（见 `spec/seed/minimal-subset.md`）\n
输出：等价 C 代码，并调用系统 C 编译器生成可执行文件。\n

### 构建（示例）
Windows（MSVC Developer Prompt）：\n
- `cl /W4 /EHsc /nologo /Fe:aegc0.exe main.c`\n

### 运行（示例）
- `aegc0.exe -i input.nprt -o out.exe`\n

> 说明：该目录内容不进入最终发行包（见 `spec/bootstrap-independence.md`）。\n
