# 📈 SwiftChat 更新日志

所有重要的项目变更都将记录在这个文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [未发布]

### 计划中的功能
- [ ] 文件上传和分享
- [ ] 消息加密
- [ ] 用户在线状态
- [ ] 消息已读状态
- [ ] 群组管理功能

## [1.0.0] - 2025-07-22

### ✨ 新增功能
- 完整的 JWT 用户认证系统
- 实时 WebSocket 聊天功能
- 多房间聊天支持
- SQLite 数据库集成
- RESTful API 接口

### 🏗️ 架构特性
- 模块化设计，易于扩展
- 多线程池处理并发请求
- 完善的错误处理机制
- 详细的日志系统

### 🧪 测试覆盖
- 单元测试覆盖核心功能
- API 集成测试
- 性能压力测试

### 📚 文档完善
- 详细的 API 文档
- 数据库设计说明
- 部署和配置指南

### 🔧 技术栈
- **后端**: C++17, CMake
- **数据库**: SQLite3
- **WebSocket**: websocketpp
- **JSON**: nlohmann/json
- **JWT**: jwt-cpp
- **测试**: Google Test
- **加密**: OpenSSL

---
