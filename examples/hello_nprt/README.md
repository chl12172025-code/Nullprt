# Hello World

最基础的 Nullprt 程序结构示例。  
使用 `NPRTcfg.nprtcfg` 与 `src/main.nprt` 作为项目入口。

## 性能基准数据（示例）
| case | avg_us | p95_us |
|---|---:|---:|
| cold start | 140 | 170 |
| warm start | 38 | 45 |

## 安全注意事项
- 仅在可信编译环境构建。
- 示例配置不包含生产级安全硬化。

## FAQ
- Q: hello_nprt 与 hello_world 的区别？
  - A: hello_nprt 是最简入口，hello_world 额外展示多文件/测试/基准。

## 扩展练习
- 增加命令行参数解析并输出版本信息。
- 引入一个简单模块并拆分文件结构。
