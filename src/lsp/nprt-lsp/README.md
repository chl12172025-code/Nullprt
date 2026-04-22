## nprt-lsp（LSP prototype）

目标：按 `spec/lsp/mvp.md` 提供 MVP LSP 功能。\n
当前原型阶段：先实现 JSON-RPC over stdio 的骨架 + `initialize` + 文档生命周期 + 最小诊断发布。\n

实现语言：当前原型用 C（bootstrap 期间可用），后续迁移到 NPRT。\n
