# Nullprt Config Format Specification

Nullprt config syntax is independent and does not reuse JSON/YAML/TOML/XML/INI grammars.

## 1. Lexical rules
- Line comment: `// ...`
- Block comment: `/* ... */`
- Section: `[section.name]`
- Assignment: `key = value`
- Array: `[v1, v2, v3]`
- Map: `{ k1: v1, k2: v2 }`
- Tuple: `(v1, v2)`
- Multi-line string: `""" ... """`

## 2. Primitive types
- string, int, float, bool
- path, url, regex, uuid, ip, mac, hostname, email
- duration (`10ms`, `2s`, `5m`)
- bytesize (`4KB`, `10MB`)
- semver and semver-range
- hex (`0x1F`), oct (`0o70`), bin (`0b1010`)

## 3. Expressions
- Arithmetic: `+ - * / % **`
- Bitwise: `& | ^ << >>`
- Logical: `&& || !`
- Compare: `== != < <= > >= in matches`
- Ternary: `cond ? a : b`

## 4. Includes and variables
- Include: `include "path/to/file.nprtcfg"`
- Conditional include: `include_if env("NPRT_ENV") == "dev" "dev.nprtcfg"`
- Variable interpolation: `${section.key}` and `${env:VAR_NAME}`

## 5. Built-in functions
- `env(name, default)`
- `file(path)`, `dir(path)`, `exists(path)`
- `read(path)`, `write(path, value)`, `size(path)`
- `time()`
- `hash(algo, value)`
- `encode(fmt, value)`, `decode(fmt, value)`
- `sign(key, value)`, `verify(key, value, sig)`

## 6. Merge/override model
- File merge order: base -> included -> later inputs
- Environment override prefix: `NPRT_CFG_`
- CLI override form: `--cfg key=value`
- Last write wins for scalar; map deep-merge; arrays replace by default.

## 7. Validation model
- Type validation, range validation, pattern validation
- Required keys can be asserted by runtime API
- Validation reports include key path, expected type, and source location

## 8. Project sections
- `[package]`, `[dependencies]`, `[dev-dependencies]`, `[build-dependencies]`
- `[target]`, `[profile]`, `[feature]`, `[optimization]`, `[obfuscation]`
- `[link]`, `[test]`, `[doc]`, `[security]`
