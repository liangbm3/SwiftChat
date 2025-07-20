# SwiftChat 文档索引

## 🚀 快速开始

- [**API 完整文档**](./API_COMPLETE.md) - **推荐** 📖 完整的HTTP和WebSocket API参考

## 📋 API 文档

### 主要API
- [API 概览](./README.md) - API文档总览和快速导航
- [WebSocket API](./websocket_api.md) - 实时通信协议文档

### 底层组件 API
- [HTTP Server API](./http_server_api.md) - HTTP服务器核心API
- [HTTP Request API](./http_request_api.md) - HTTP请求处理类
- [HTTP Response API](./http_reponse_api.md) - HTTP响应构建类
- [Database Manager API](./database_manager_api.md) - 数据库管理器

## 🗄️ 数据库文档

- [数据库设计](./database.md) - 数据库架构和表结构

## 📁 文档组织

```
docs/
├── README.md                    # API概览 (本文档)
├── API_COMPLETE.md             # 🌟 完整API文档 (推荐)
├── websocket_api.md            # WebSocket API详细说明
├── database.md                 # 数据库设计文档
├── http_server_api.md          # HTTP服务器底层API
├── http_request_api.md         # HTTP请求类API
├── http_reponse_api.md         # HTTP响应类API
└── database_manager_api.md     # 数据库管理器API
```

## 📖 推荐阅读顺序

### 对于API使用者
1. [API 概览](./README.md) - 了解整体架构
2. [**API 完整文档**](./API_COMPLETE.md) - 详细的API参考
3. [WebSocket API](./websocket_api.md) - 实时通信功能

### 对于开发者
1. [数据库设计](./database.md) - 了解数据结构
2. [HTTP Server API](./http_server_api.md) - 了解服务器架构
3. [Database Manager API](./database_manager_api.md) - 了解数据访问层

## 🔄 文档更新记录

- **2025-07-20**: 创建完整API文档，统一响应格式，删除过期文档
- **2025-07-20**: 重构ServerService，将系统API移动到独立服务
- **2025-07-20**: 更新WebSocket API以支持统一响应格式

---

**维护状态**: ✅ 活跃维护  
**最后更新**: 2025-07-20  
**文档版本**: 1.0.0
