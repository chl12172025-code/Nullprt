## 配置与包依赖格式（自定义语法 + EBNF）

本章定义 NPRT 的统一配置格式：项目配置（`.nprtcfg/.nullprtconfig`）与包依赖/锁定（`.npkg/.nprtpkg`）。格式必须独立，不复用 JSON/TOML/YAML/XML。

### 1. 文件命名约定
- 项目根配置：`NPRTcfg.nprtcfg` 或 `NullprtConfig.nullprtconfig`（二选一存在即可；若都存在，以前者优先）。
- 包依赖：`deps.npkg`（建议）或 `deps.nprtpkg`。
- 锁文件：`deps.lock.npkg`（建议）或 `deps.lock.nprtpkg`（保持同语法）。

### 2. 语法设计原则
- **可读性**：类 INI/声明式块结构，但关键字、分隔符与现有格式不同以避免混淆。
- **可流式解析**：单遍 tokenizer + 递归下降即可实现。
- **可升级**：文件头包含 `config_version`，解析器支持 N-1 版本。
- **良好错误信息**：行列号、期望 token、上下文片段。

### 3. 词法约定
- 注释：`//` 行注释；`/* ... */` 块注释。
- 标识符：`[A-Za-z_][A-Za-z0-9_]*`
- 字符串：`"..."`，支持 `\\n \\t \\\\ \\\"`。
- 整数：十进制；可选 `0x` 十六进制（建议用于位掩码）。

### 4. EBNF（统一语法）
> 说明：同一套语法可用于项目配置与依赖文件，仅关键字集合不同。

```text
document        = header, { statement } ;

header          = "nprtcfg", ws, version, ";" ;
version         = int, ".", int, ".", int ;

statement       = kv_stmt | block_stmt | feature_block ;

kv_stmt         = key, ws, "=", ws, value, ";" ;
key             = ident, { ".", ident } ;

block_stmt      = ident, ws, "{", { statement }, "}" ;

feature_block   = "when", ws, "(", cond_expr, ")", ws, "{", { statement }, "}" ;

cond_expr       = cond_term, { ws, ("and" | "or"), ws, cond_term } ;
cond_term       = "not", ws, cond_term
               | "(", cond_expr, ")"
               | cond_atom ;
cond_atom       = ident, ws, ("==" | "!="), ws, value ;

value           = string
               | int
               | bool
               | list
               | map
               | ident_ref ;

bool            = "true" | "false" ;
list            = "[", [ value, { ws, ",", ws, value } ], "]" ;
map             = "{", [ map_entry, { ws, ",", ws, map_entry } ], "}" ;
map_entry       = ident, ws, ":", ws, value ;
ident_ref       = ident, { "::", ident } ;

ident           = ALPHA, { ALPHA | DIGIT | "_" } ;
int             = DIGIT, { DIGIT } ;
string          = "\"", { char }, "\"" ;
ws              = { " " | "\\t" | "\\r" | "\\n" | comment } ;
comment         = line_comment | block_comment ;
line_comment    = "//", { not_newline }, ( "\\n" | EOF ) ;
block_comment   = "/*", { not_block_end }, "*/" ;
```

### 5. 项目配置字段语义映射（必须支持）
以下字段名是**规范字段**（关键字），可以通过 `key.path` 组织层级。

#### 元信息
- `project.name`：string
- `project.version`：string（SemVer）
- `project.description`：string
- `project.license`：string（SPDX 标识符建议）
- `project.authors`：list[string]（或 `author { name/email }` 块）

#### 依赖
- `deps { ... }`：运行时依赖声明块
- `dev_deps { ... }`：开发依赖声明块

依赖条目建议形式（示例语义，不绑定最终写法）：
- `deps.foo = { version: "1.2.3", registry: "central", features: ["x"] };`
- 支持：git/url/path 三类来源（MVP 可先 central+git）。

#### 构建与编译
- `build.targets`：list[string]（target triples）
- `build.opt_level`：int（0-3 或 0-2，需定义）
- `build.profile`：ident（`dev/release`）
- `build.obfuscate`：bool（发行版可用）
- `build.features`：list[string]
- `build.cond`：使用 `when(...) { ... }` 进行条件配置

#### 签名与授权
- `signing.enabled`：bool
- `signing.key_id`：string（引用本地密钥别名，不直接存私钥）
- `license.server_url`：string（HTTP/HTTPS）
- `license.mode`：ident（`online/offline/hybrid`）

#### 硬件绑定策略（合规）
- `binding.enabled`：bool
- `binding.signals`：list[ident]（例如 `disk_serial`, `mac_addr`, `tpm_id`；只读）
- `binding.policy`：ident（例如 `strict/relaxed`）

### 6. 依赖/锁文件（`.npkg/.nprtpkg`）字段语义
#### 依赖清单
- `registry.default`：string（默认仓库）
- `registry.mirrors`：list[string]
- `packages { name = { version: "...", integrity: "...", source: ... }; }`

#### 锁定文件（可重现）
- 固定解析后的版本、下载 URL、内容哈希（`sha256`）、可选签名链。

### 7. 示例（占位，待最终语法定稿后补全）
> 由于语法尚未与关键字风格最终绑定，本章先提供 EBNF 与字段语义；等 `cfg` 解析器实现时补充端到端样例文件。

### 本章决策清单
- 是否允许尾随逗号与尾随分号？
- `when(cond)` 的可用原子：`target_os/arch/profile/feature` 的标准 key 集合。
- 字符串编码：是否强制 UTF-8？
