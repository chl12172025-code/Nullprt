## aegc1 AST -> IR Contract (Frozen v1)

This document freezes the lowering contract used by `aegc1` driver and backends.

### Version
- `ir_version = 1`
- `pointer_bits = 64`

### Item Lowering Rules
- `fn` items lower to `A1IrFunction` entries with `source_kind = A1_ITEM_FN`.
- `struct` and `enum` declarations also produce IR entries so both C/native backends can emit layout stubs.
- `import` and macro-only top-level items are compile-time only and do not emit runtime IR entries.

### Semantic Flags in IR
- `is_generic_instance`: true if parsed item has generic parameter list.
- `is_result_like`: true if lowered symbol name starts with `Result`.
- `cfg_enabled`: false when item is gated by cfg/attribute branch not selected in current lowering context.

### Stable Dump Format (`--dump-ir`)
Line format:

`item=<kind> name=<symbol> bb=<n> generic=<bool> result_like=<bool> cfg_enabled=<bool>`

The dump output is intended to be deterministic and testable by golden files under `tests/aegc1/ir/`.
