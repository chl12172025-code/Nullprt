## `deps.lock.npkg`（锁文件）规范 v0

锁文件用于可重现构建：在任意平台/时间点使用相同 lockfile，都应解析到相同版本与相同内容哈希的 dist。

### 1. 格式
- 语法：NPRT 配置语法（见 `docs/30-config-format.md`）\n
- 文件头：`nprtcfg 0.1.0;`（锁文件格式版本可以与配置版本对齐或独立）

### 2. 必需键空间
- `lock.format_version` (string) 例如 `"0"`\n
- `lock.generated_by` (string) 例如 `"npkg 0.1.0"`\n
- `registry.default` (string)\n
- `packages { ... }`\n

### 3. `packages` 条目结构（必须字段）
每个包：
- `version`：精确版本\n
- `source`：map\n
  - `kind`：`"registry" | "git" | "path"`\n
  - `registry` / `url` / `path`：按 kind 必填\n
- `download_url`：string\n
- `integrity`：`"sha256:<hex>"`\n

可选字段：
- `signatures`：list[map]（签名与证书链）\n
- `deps`：map（可选，为加速构建图；源信息也能推导）

### 4. 示例
```text
nprtcfg 0.1.0;

lock.format_version = "0";
lock.generated_by = "npkg 0.1.0";

registry.default = "central";

packages {
  libc = {
    version: "0.2.0",
    source: { kind: "registry", registry: "central" },
    download_url: "https://repo.nullprt.example/pkg/libc/0.2.0/libc.npkg",
    integrity: "sha256:aaaaaaaa"
  };
}
```

### 5. 校验规则
- `integrity` 必须与下载内容的 `sha256` 匹配，否则构建失败。\n
- 若启用 `--require-signature`：`signatures` 必须存在且链可信，否则失败。
