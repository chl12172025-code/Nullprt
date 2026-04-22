## nprt-pkg（npkg client prototype）

目标：实现 `spec/npkg/registry-protocol.md` 的最小客户端能力：\n
- ping/capabilities\n
- 查询包版本信息\n
- 下载 dist + sha256 校验\n
- CAS 缓存布局\n
- 生成 `deps.lock.npkg`\n

实现语言：当前原型用 C（便于 Stage0/Stage1 期间快速落地），后续迁移到 NPRT。\n
