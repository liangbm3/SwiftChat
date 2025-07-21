<div align="center">

# SwiftChat 💬

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/liangbm3/SwiftChat)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)](https://www.linux.org/)

**一个基于 C++ 的高性能实时聊天应用，支持多房间聊天、用户认证和消息持久化。**


**🌐 在线体验**: [https://demo.swiftchat.example.com](https://demo.swiftchat.example.com) *(演示站点)*

</div>

## 📋 目录

- [✨ 功能特性](#-功能特性)
- [📋 系统要求](#-系统要求)  
- [🚀 快速开始](#-快速开始)
- [📚 相关文档](#-相关文档)
- [🏗️ 项目架构](#️-项目架构)
- [🔧 配置选项](#-配置选项)
- [🧪 运行测试](#-运行测试)
- [🔒 安全特性](#-安全特性)
- [🚀 部署指南](#-部署指南)
- [📈 更新日志](#-更新日志)
- [🙏 致谢](#-致谢)
- [📄 许可证](#-许可证)

## ✨ 功能特性

- 🔐 **JWT 用户认证** - 安全的用户登录和注册系统
- 💬 **实时聊天** - 基于 WebSocket 的即时消息传递
- 🏠 **多房间支持** - 创建、加入、管理多个聊天室
- 💾 **消息持久化** - 使用 SQLite 数据库保存聊天记录
- 🔄 **自动重连** - 网络中断时自动重新连接
- ⚡ **高性能** - 多线程架构，支持大量并发连接
- 🛡️ **线程安全** - 完善的并发控制和错误处理
- 📡 **RESTful API** - 完整的 REST API 接口
- 🔧 **中间件支持** - 可扩展的认证中间件
- 📊 **日志系统** - 完善的日志记录和调试支持
- ⚙️ **配置灵活** - 支持环境变量和命令行参数配置

## 📋 系统要求

### 编译环境
- **操作系统**: Linux (推荐 Ubuntu 18.04+)
- **编译器**: GCC 7.0+ 或 Clang 6.0+ (支持 C++17)
- **CMake**: 3.10+

### 运行时依赖
- **SQLite3**: 用于数据持久化
- **OpenSSL**: 用于 JWT 签名和验证
- **pthread**: POSIX 线程库

### 第三方库
- **websocketpp**: WebSocket 服务器实现
- **nlohmann/json**: JSON 解析库  
- **jwt-cpp**: JWT 处理库

### 开发和测试工具
- **Google Test**: 单元测试框架 (可选，用于测试)
- **CMake**: 构建系统
- **Git**: 版本控制系统

## 🚀 快速开始

### 方法一：一键设置（推荐）

```bash
# 克隆项目（包含 submodules）
git clone --recursive https://github.com/liangbm3/SwiftChat.git
cd SwiftChat

# 设置 JWT 密钥
export JWT_SECRET="your_super_secret_jwt_key_here"

# 创建构建目录并编译
mkdir build && cd build
cmake ..
make -j$(nproc)

# 启动服务器 (构建后自动安装到 bin 目录)
cd bin
./SwiftChat
```

### 方法二：手动设置

#### 1. 克隆项目

```bash
git clone https://github.com/liangbm3/SwiftChat.git
cd SwiftChat
```

#### 2. 安装系统依赖

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake libsqlite3-dev libssl-dev
```

**CentOS/RHEL:**
```bash
sudo yum install gcc-c++ cmake sqlite-devel openssl-devel
```

#### 3. 初始化第三方库

```bash
# 初始化 Git submodules
git submodule update --init --recursive
```

#### 4. 设置环境变量

```bash
# 设置 JWT 密钥（必需）
export JWT_SECRET="your_super_secret_jwt_key_here"

# 可选：设置其他环境变量
export LOG_LEVEL=INFO
export DB_PATH=./chat.db
```

#### 5. 编译项目

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

#### 6. 运行应用

```bash
# 进入 bin 目录
cd bin

# 启动服务器
./SwiftChat

# 或者指定端口
./SwiftChat --http-port 8080 --ws-port 8081
```

#### 7. 访问应用

打开浏览器访问: `http://localhost:8080`

## 📚 相关文档

- [API 文档](docs/API.md) - RESTful API 详细说明
- [数据库设计](docs/database.md) - 数据库结构和设计
- [HTTP 服务器](docs/http_server.md) - HTTP 服务器实现详解
- [数据模型](docs/model.md) - 数据模型


## 🏗️ 项目架构

```
SwiftChat/
├── src/                    # 源代码
│   ├── main.cpp           # 程序入口
│   ├── http/              # HTTP 服务器
│   │   ├── http_server.cpp/hpp
│   │   ├── http_request.cpp/hpp
│   │   └── http_response.cpp/hpp
│   ├── websocket/         # WebSocket 服务器
│   │   └── websocket_server.cpp/hpp
│   ├── db/                # 数据库层
│   │   ├── database_manager.cpp/hpp
│   │   ├── database_connection.cpp/hpp
│   │   ├── user_repository.cpp/hpp
│   │   ├── room_repository.cpp/hpp
│   │   └── message_repository.cpp/hpp
│   ├── service/           # 业务逻辑层
│   │   ├── auth_service.cpp/hpp
│   │   ├── user_service.cpp/hpp
│   │   ├── room_service.cpp/hpp
│   │   ├── message_service.cpp/hpp
│   │   └── server_service.cpp/hpp
│   ├── middleware/        # 中间件
│   │   └── auth_middleware.cpp/hpp
│   ├── model/             # 数据模型
│   │   ├── user.cpp/hpp
│   │   ├── room.cpp/hpp
│   │   └── message.cpp/hpp
│   └── utils/             # 工具类
│       ├── logger.cpp/hpp
│       ├── jwt_utils.cpp/hpp
│       ├── thread_pool.cpp/hpp
│       └── timer.cpp/hpp
├── static/                # 前端资源
│   ├── index.html         # 主页面
│   └── test.html          # 测试页面
├── tests/                 # 单元测试
│   ├── db/                # 数据库测试
│   ├── http/              # HTTP 测试
│   ├── model/             # 模型测试
│   └── utils/             # 工具测试
├── docs/                  # 文档
├── scripts/               # 测试脚本
├── third_party/           # 第三方库
└── CMakeLists.txt         # 构建配置
```

## 🔧 配置选项

### 环境变量

| 变量名 | 描述 | 默认值 | 必需 |
|--------|------|--------|------|
| `JWT_SECRET` | JWT 签名密钥 | 无 | ✅ |
| `DB_PATH` | 数据库文件路径 | `./chat.db` | ❌ |
| `LOG_LEVEL` | 日志级别 (DEBUG/INFO/WARN/ERROR) | `INFO` | ❌ |

### 命令行参数

```bash
./SwiftChat [选项]

选项:
  --http-port PORT     HTTP 服务器端口 (默认: 8080)
  --ws-port PORT       WebSocket 服务器端口 (默认: 8081)
  --db-path PATH       数据库文件路径 (默认: ./chat.db)
  --static-dir DIR     静态文件目录 (默认: ./static)
  --help              显示帮助信息
  --version           显示版本信息
```

## 🧪 运行测试

### 单元测试

```bash
# 进入构建目录
cd build/bin

# 编译所有测试 (需要回到上级目录)
cd ..
make -j$(nproc)

# 运行所有测试
ctest --verbose

# 运行特定测试
./tests/test_database_manager
./tests/test_http_server
./tests/test_http_request
./tests/test_http_response
./tests/test_user
./tests/test_room
./tests/test_message
```

### 测试脚本


```bash
# 运行 API 测试
python3 scripts/api_test.py

# 运行端到端测试
python3 scripts/e2e_test.py

# HTTP 接口测试
k6 run --env BASE_URL=http://localhost:8080 scripts/http_test.js

# WebSocket 连接测试
k6 run \
  -e BASE_URL=localhost:8080 \
  -e WS_URL=localhost:8081 \
  scripts/websocket_test.js
```



## 🔒 安全特性

### JWT 认证
- 使用 RS256 算法签名
- Token 自动过期机制
- 安全的密钥管理

### 数据安全
- SQL 注入防护
- 输入验证和清理
- 安全的密码存储

### 网络安全
- CORS 策略配置
- 请求频率限制
- 安全头部设置

### 最佳实践建议
```bash
# 生成强密钥
export JWT_SECRET=$(openssl rand -base64 64)

# 设置文件权限
chmod 600 chat.db

# 使用防火墙
sudo ufw allow 8080
sudo ufw allow 8081
```

## 🚀 部署指南

### 生产环境部署

```bash
# 1. 编译 Release 版本
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 2. 设置生产环境变量
export JWT_SECRET=$(openssl rand -base64 64)
export LOG_LEVEL=INFO
export DB_PATH=/var/lib/swiftchat/chat.db

# 3. 创建数据目录
sudo mkdir -p /var/lib/swiftchat
sudo chown $USER:$USER /var/lib/swiftchat

# 4. 进入 bin 目录并启动服务
cd bin
./SwiftChat --http-port 80 --ws-port 443
```

### 使用 systemd 管理服务

创建服务文件 `/etc/systemd/system/swiftchat.service`:

```ini
[Unit]
Description=SwiftChat Server
After=network.target

[Service]
Type=simple
User=swiftchat
WorkingDirectory=/opt/swiftchat/bin
Environment=JWT_SECRET=your_jwt_secret_here
Environment=LOG_LEVEL=INFO
Environment=DB_PATH=/var/lib/swiftchat/chat.db
ExecStart=/opt/swiftchat/bin/SwiftChat --http-port 8080 --ws-port 8081
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

启动服务:
```bash
sudo systemctl enable swiftchat
sudo systemctl start swiftchat
sudo systemctl status swiftchat
```

## 📈 更新日志

查看完整的 [更新日志](CHANGELOG.md) 了解项目的所有变更记录。

### 最新版本 v1.0.0 (2025-01-21)
- ✨ 完整的实时聊天功能
- 🔐 JWT 用户认证系统  
- 🏠 多房间聊天支持
- 💾 SQLite 数据库集成
- 📡 RESTful API 接口

### 即将推出
- 文件上传和分享
- 消息加密功能
- 用户在线状态


## 🙏 致谢

感谢以下开源项目的支持:

- [websocketpp](https://github.com/zaphoyd/websocketpp) - WebSocket 服务器库
- [nlohmann/json](https://github.com/nlohmann/json) - JSON 解析库
- [jwt-cpp](https://github.com/Thalhammer/jwt-cpp) - JWT 处理库
- [Google Test](https://github.com/google/googletest) - 测试框架


## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件


**⭐ 如果这个项目对您有帮助，请点一个 Star！**
