## `npkg` 仓库协议（Registry Protocol）v0

目标：支持中央仓库与私有仓库；满足依赖解析、下载缓存、版本锁定、完整性校验、（可选）签名链验证。\n
本协议只定义**包分发与元数据**，不涉及任何越权能力。

### 1. 基础约定
- 协议版本：`v0`（路径前缀体现：`/v0/...`）
- 编码：UTF-8
- 传输：HTTP/HTTPS（建议强制 HTTPS）
- 内容校验：所有包内容必须提供 `sha256`（十六进制）\n

### 2. 实体与命名
- **PackageName**：`[a-z0-9_]+`（建议小写）
- **Version**：SemVer（`MAJOR.MINOR.PATCH`）
- **Dist**：可下载的包工件（artifact），例如 `.npkg` 压缩包

### 3. API 端点（最小集合）
#### 3.1 服务能力
- `GET /v0/ping` → `200 OK`
- `GET /v0/capabilities` → 支持的特性（签名、上传、私有访问控制等）

#### 3.2 包索引与版本查询
- `GET /v0/packages/{name}`\n
返回：该包的版本列表与每个版本的摘要（不含大字段）。\n

- `GET /v0/packages/{name}/{version}`\n
返回：该版本的完整元数据（含 dist 信息、依赖、features）。

#### 3.3 下载 dist
- `GET /v0/dist/{name}/{version}/{artifact}`\n
返回：二进制包内容。\n
要求：响应头或元数据中提供 `sha256`，客户端校验后才写入缓存。

#### 3.4 上传（可选，私有仓库常用）
- `POST /v0/publish`\n
鉴权：Bearer token（仓库自定义）。\n
主体：`multipart/form-data`，包含 `metadata` 与 `artifact`。\n

### 4. 元数据模型（逻辑字段）
> 具体序列化格式不强制；仓库可用 JSON 对外，但 `npkg` 客户端需实现自己的解析器（工具链独立）。\n
建议仓库返回 `application/nprt-meta`（自定义）或 `application/json`（兼容性）。

必需字段：
- `name` (string)
- `version` (string)
- `summary` (string, optional)
- `license` (string, optional)
- `deps` (map[name] -> semver_range)\n
- `features` (map[feature] -> list[name]) (optional)\n
- `dist` (list[artifact])\n

artifact 字段：
- `filename` (string)\n
- `target` (string | "any")（例如 `x86_64-windows-msvc` 或 `any`）\n
- `sha256` (string)\n
- `size` (int)\n
- `url` (string)\n
- `signatures` (list, optional)\n

### 5. 签名与信任（可选）
若启用签名：
- 仓库为 dist 提供 `sig` 与 `cert_chain`（或 key id）。
- 客户端策略：\n
  - 默认只校验 `sha256`\n
  - `--require-signature`：必须校验签名且链可信\n
  - `--trust-store <path>`：自定义信任根\n

### 6. 锁文件（Lockfile）规范（逻辑）
锁文件必须包含：
- 解析后版本（精确）
- dist URL（或仓库 + 路径）
- `sha256`
- 源信息（registry/git/path）
- 可选签名链信息（用于离线验证）

锁文件仍使用 NPRT 自定义配置语法（见 `docs/30-config-format.md`），建议键空间：
- `packages.<name> = { version: "...", source: {...}, download_url: "...", integrity: "sha256:...", signatures: [...] };`

### 7. 客户端缓存布局（建议）
- 缓存根：`%NPRT_CACHE%/npkg/`\n
- `index/`：包索引缓存（按仓库域名分桶）\n
- `dist/`：按 `sha256` 内容寻址存储（CAS）\n

### 8. 安全与合规
- 禁止执行仓库返回的脚本作为安装步骤（MVP 只做纯下载+构建）。\n
- 任何“构建脚本”必须在用户显式启用并在沙箱中运行（后续阶段再设计）。
