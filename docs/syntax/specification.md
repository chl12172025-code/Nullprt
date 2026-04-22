# Nullprt Syntax Specification

This document defines the independent Nullprt syntax system for language, tooling, and diagnostics.

## 1. Keywords

Nullprt reserves keywords by domain to avoid overlap ambiguity and maintain grammar stability.

### 1.1 Core control and declarations
`fn`, `let`, `mut`, `const`, `static`, `type`, `struct`, `enum`, `union`, `trait`, `impl`, `mod`, `use`, `import`, `export`, `extern`, `as`, `where`, `pub`, `crate`, `super`, `self`.

### 1.2 Flow and error handling
`if`, `else`, `unless`, `when`, `cond`, `switch`, `match`, `loop`, `while`, `for`, `in`, `break`, `continue`, `return`, `yield`, `await`, `try`, `catch`, `finally`, `defer`, `throw`, `panic`, `recover`, `commit`, `rollback`.

### 1.3 Ownership, lifetime, safety
`move`, `copy`, `clone`, `borrow`, `lifetime`, `unsafe`, `trusted`, `invariant`, `assert`, `assume`, `proof`, `contract`.

### 1.4 Generics, type-level, effects
`generic`, `associated`, `exists`, `higher`, `linear`, `session`, `effect`, `handler`, `depends`, `refine`, `gadt`.

### 1.5 Async, concurrency, memory model
`async`, `sync`, `channel`, `send`, `recv`, `select`, `atomic`, `fence`, `acquire`, `release`, `relaxed`, `seqcst`.

### 1.6 Attributes, macros, cfg
`derive`, `macro`, `macro_rules`, `cfg`, `cfg_attr`, `test`, `bench`, `doc`, `feature`, `platform`, `target`.

### 1.7 Reflection and metaprogramming
`reflect`, `meta`, `quote`, `unquote`, `constexpr`, `comptime`, `generate`.

## 2. Operators and Precedence

From lowest to highest:

1. Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `%=` and bitwise compound assignments.
2. Pipeline/composition: `|>`, `<|`, `>>`, `<<`.
3. Logical OR: `||`.
4. Logical AND: `&&`.
5. Equality: `==`, `!=`.
6. Comparison: `<`, `<=`, `>`, `>=`, `in`, `matches`.
7. Range: `..`, `..=`.
8. Additive: `+`, `-`, `|`, `^`.
9. Multiplicative: `*`, `/`, `%`, `&`.
10. Unary: `!`, `~`, unary `-`, unary `*`, unary `&`.
11. Postfix: call `()`, index `[]`, field `.`, cast `as`.

Associativity:
- Right-associative: assignment.
- Left-associative: most binary operators.
- Non-associative: range and comparison chains unless explicitly grouped.

## 3. Type System

### 3.1 Primitive
- Integers: `i8/i16/i32/i64/i128/isize`, `u8/u16/u32/u64/u128/usize`
- Floating: `f16/f32/f64/f128`
- `bool`, `char`, `str`, `bytes`, `rawptr`, `unit`, `never`

### 3.2 Compound
- Arrays, vectors, matrices, slices, tuples
- Struct, union, enum, option, result
- Function pointer, closure, trait object, opaque and dynamic types

### 3.3 Containers
- `list`, `array_list`, `linked_list`, `stack`, `queue`, `deque`, `priority_queue`
- set/map variants including ordered/hash/tree/multi forms
- graph/tree/heap/disjoint_set/bit_set/sparse_set

### 3.4 Pointer and reference families
- Shared/unique/weak/atomic/non-null/nullable/custom/external pointers
- Shared/mutable/borrowed references with explicit lifetime parameters

### 3.5 Advanced
- ADT/GADT, higher-kinded type constructors, dependent/refinement/indexed types
- Linear/session/effect types, associated types and constraints

## 4. Control Flow

- Conditional: `if`, `if let`, `unless`, `when`, `cond`, `switch`, `match`
- Pattern forms: literal/range/guard/nested/destructure/ref/move/copy/wildcard
- Loops: `loop`, `while`, `while let`, `for`, `for in`, `for range`, iterator loops
- Jumps: `break`, `continue`, `return`, `yield`, `throw`, `defer`
- Labels allowed on loop/block/match/coroutine scopes.

## 5. Modules and Visibility

- Declarations: `mod`, `pub`, `crate`, `extern`, `use`, `import`, `export`, `from`.
- Paths: absolute, relative, root, parent, alias, re-export, cfg-gated.
- Organization: file module, directory module, inline module, nested module.

## 6. Attributes and Macros

- Outer attribute: `#[attr]`, inner attribute: `#![attr]`.
- Compiler attrs include: `derive`, `repr`, `inline`, `must_use`, `deprecated`,
  `allow/warn/deny/forbid`, `cfg/cfg_attr`, `link_section`, `export_name`, `target_feature`.
- Macro forms: declarative `macro_rules`, derive macro, attribute macro, function macro.

## 7. Metaprogramming and Reflection

- Compile-time evaluation via `comptime` and constant functions.
- Reflection categories: type/value/module/function/trait/attribute metadata.
- Codegen supports quote/unquote, AST transforms, and staged generation.

## 8. Concurrency and Parallelism

- Coroutine syntax: `async`, `await`, `yield`, `resume`, `suspend`, `cancel`.
- Channels: buffered/unbuffered and multi-producer/consumer patterns.
- Atomics: load/store/exchange/CAS/fetch operations with explicit ordering.
- Sync primitives: mutex, rwlock, semaphore, condvar, barrier, lock-free structures.

## 9. Error Model

- Recoverable: `Result`, `Option`, `?`, `try/catch/finally`, context chaining.
- Non-recoverable: `panic`, `abort`, `assert`, `unreachable`.
- Contracts and verification hooks: pre/post/invariant/assert/assume/proof.

## 10. Unsafe Syntax

- Unsafe forms: `unsafe {}`, `unsafe fn`, `unsafe trait`, `unsafe impl`.
- Restricted operations: raw pointer deref, union access, inline asm, extern low-level calls.
- Unsafe blocks must document invariants and safety rationale.

## 11. Ethical Guardrail Notes

High-risk APIs (hardware simulation, VM behavior simulation, cross-process memory operations)
must remain developer-only and pass triple gate checks:
- compile-time gate
- runtime research token
- warning and auditable logging
