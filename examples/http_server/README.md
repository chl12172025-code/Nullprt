# HTTP Server 示例

展示如何使用未来 `std::net` API 搭建 Web 服务。

## 本示例新增点
- HTTPS 配置示例（证书/密钥路径）
- WebSocket 升级示例
- 中间件链示例（request_id/auth/logging/router）

## 性能基准数据（示例）
| case | qps | p95_latency_ms |
|---|---:|---:|
| HTTP keepalive | 18200 | 6.8 |
| HTTPS | 12100 | 9.4 |
| WebSocket echo | 30500 msg/s | 4.1 |

## 安全注意事项
- 示例证书路径仅用于演示，生产环境必须使用真实证书管理与轮换。
- 不要在公开网络暴露未加鉴权的 WebSocket 端点。

## FAQ
- Q: 为什么启用 HTTPS 后吞吐下降？
  - A: TLS 握手与加解密会带来额外开销。

## 扩展练习
- 增加限流中间件并比较前后基准。
- 增加 `wss://` 证书热更新演示。
