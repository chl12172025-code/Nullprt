## aegc1（self-host compiler, initial source）

本目录存放 `aegc1` 的 NPRT 源码：目标是最终能编译完整 Nullprt。\n
在自举早期阶段，这里的代码会先限定在 `aegc0` 支持的极小子集语法之内（见 `spec/seed/minimal-subset.md`），以便：\n
`aegc0` → 编译出第一个 `aegc1` 可执行文件。\n

随后 `aegc1` 自身逐步扩展语言前端/IR/后端能力，最终实现自举：\n
`aegc0` -> `aegc1` -> `aegc2`。\n

当前目录结构（C 原型实现，后续迁移至 NPRT 源码）：
- `frontend/`：完整语法 token + parser（含错误恢复）
- `semantic/`：typecheck / borrowcheck / monomorphize 前置
- `ir/`：NPRT-IR 降低、校验、dump
- `backend/c_emitter.*`：IR -> C
- `backend/native/x86_64/`：原生后端接口（对象文件生成骨架）
- `driver/main.c`：编译器入口（`--emit-c` / `--emit-native` / `--dump-ir`）
