## NPRTcfg / deps.npkg 规范键空间（Keyspace）

本文件把 `docs/30-config-format.md` 中的“字段语义”固化为**规范键名集合**，用于：
- 配置解析器的允许/警告策略（未知键 → warning 或 error，取决于严格模式）
- 条件编译/条件配置中 `when(...)` 的标准原子集合
- 文档与工具（格式化器、IDE 补全）

### 1. 标准条件原子（`when(cond)` 可使用）
所有原子统一以 `env.*` 命名空间暴露，保证扩展不破坏用户自定义键。

- `env.target_os`：`"windows" | "macos" | "linux"`
- `env.target_arch`：`"x86_64" | "arm64"`
- `env.target_abi`：`"msvc" | "gnu" | "darwin"`（可扩展）
- `env.profile`：`"dev" | "release" | "test" | "bench"`
- `env.feature.<name>`：`true/false`（由 `build.features` 或命令行注入）

示例：
- `when(env.target_os == "windows" and env.profile != "release") { ... }`

### 2. 项目配置规范键（`.nprtcfg/.nullprtconfig`）
#### `project.*`
- `project.name` (string)
- `project.version` (string, SemVer)
- `project.description` (string)
- `project.license` (string, SPDX recommended)
- `project.authors` (list[string])

#### `build.*`
- `build.targets` (list[string])
- `build.profile` (string, one of env.profile)
- `build.opt_level` (int, 0..3)
- `build.obfuscate` (bool)
- `build.features` (list[string])
- `build.mode` (string, reserved; future: `static|dynamic|pie`)

#### `deps` / `dev_deps`
推荐使用块：
- `deps { <name> = <dep_spec>; }`
- `dev_deps { <name> = <dep_spec>; }`

`dep_spec`（map）规范字段：
- `version` (string) 语义化版本表达式（MVP：精确版本）
- `registry` (string) 仓库名
- `git` (string) git url（可选）
- `rev` (string) 固定 commit（可选）
- `path` (string) 本地路径（可选）
- `features` (list[string])（可选）

#### `signing.*`
- `signing.enabled` (bool)
- `signing.key_id` (string)
- `signing.timestamp_url` (string, optional)

#### `license.*`
- `license.server_url` (string, http/https)
- `license.mode` (string, `online|offline|hybrid`)
- `license.product_id` (string, optional)

#### `binding.*`（只读硬件信号）
- `binding.enabled` (bool)
- `binding.signals` (list[string])（例如：`disk_serial`, `mac_addr`, `tpm_id`）
- `binding.policy` (string, `strict|relaxed`)

### 3. 依赖文件规范键（`.npkg/.nprtpkg`）
- `registry.default` (string)
- `registry.mirrors` (list[string])
- `packages { <name> = <locked_pkg>; }`（锁文件用）

`locked_pkg`（map）规范字段（锁文件必须齐全）：
- `version` (string)
- `source` (map) 例如 `{ kind: "registry", registry: "central", name: "foo" }`
- `download_url` (string)
- `integrity` (string) 例如 `sha256:<hex>`
- `signatures` (list[map])（可选，证书链/签名）

### 4. 未知键策略（建议）
- 默认：未知键 **warning**（便于前向兼容）
- `--strict-config`：未知键 **error**
