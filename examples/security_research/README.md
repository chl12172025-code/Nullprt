# security_research

仅用于授权安全研究与兼容性测试。  
需要开发者门控与运行时令牌。

## 本示例新增点
- 硬件模拟完整演示
- 虚拟机检测绕过示例
- 跨进程内存分析完整流程

## 性能基准数据（示例）
| case | duration_ms | notes |
|---|---:|---|
| hw simulation profile apply | 12.6 | dev-only |
| vm bypass check pipeline | 8.9 | dev-only |
| cross-process scan | 34.2 | dev-only |

## 安全注意事项
- 仅限授权环境，不得用于攻击第三方系统或绕过合法授权机制。
- 所有操作应审计记录并可追溯。

## FAQ
- Q: 为什么发行版无法运行这些流程？
  - A: 发行版默认关闭开发者研究能力（强制门控）。

## 扩展练习
- 增加多进程并发分析场景与结果聚合。
- 增加误报/漏报评估数据集回放。
