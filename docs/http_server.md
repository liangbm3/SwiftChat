# HTTP 服务器设计文档

## 1. 概述

HTTP 服务器的实现位于 `src/http/` 目录下，提供了一个高性能、多线程的Web服务器框架。该服务器支持路由管理、中间件、静态文件服务和路径参数等现代Web服务器的核心功能。

服务器采用事件驱动架构，主线程负责接受连接，工作线程池处理具体的HTTP请求。所有网络操作都使用POSIX socket API实现，确保跨平台兼容性。

## 2. 特性

### 2.1 核心特性

- **多线程处理**: 使用线程池处理并发连接，提高服务器性能
- **路由系统**: 支持动态路由注册和路径参数提取
- **中间件支持**: 提供请求预处理和后处理机制
- **静态文件服务**: 自动处理静态资源文件的请求
- **CORS支持**: 内置跨域资源共享支持
- **安全性**: 防止路径遍历攻击，支持请求超时控制

### 2.2 MIME类型支持

服务器内置常见文件类型的MIME映射：

| 扩展名 | MIME类型 |
|--------|----------|
| .html | text/html |
| .css | text/css |
| .js | application/javascript |
| .json | application/json |
| .png | image/png |
| .jpg/.jpeg | image/jpeg |
| .gif | image/gif |
| .svg | image/svg+xml |
| .ico | image/x-icon |
| .txt | text/plain |

### 2.3 安全特性

#### 2.3.1 路径遍历保护

- 检查请求路径中的".."字符串
- 拒绝包含目录遍历尝试的请求
- 返回403 Forbidden响应

#### 2.3.2 超时控制

- 客户端socket设置30秒超时
- 防止慢速攻击和资源泄露

#### 2.3.3 CORS支持

- 自动处理OPTIONS预检请求
- 添加必要的CORS响应头
- 支持跨域资源访问


### 2.4 性能特点

#### 2.4.1 并发处理

- 使用线程池避免频繁创建/销毁线程的开销
- 主线程专注于连接接受，工作线程处理请求
- 支持高并发连接处理

#### 2.4.2 内存管理

- 使用栈上缓冲区接收数据，减少动态分配
- 智能指针管理资源生命周期
- 及时关闭连接释放资源

#### 2.4.3 网络优化

- 设置SO_REUSEADDR避免端口重用问题
- 合理的接收缓冲区大小(8KB)
- 支持HTTP keep-alive（通过适当的响应头）

### 2.5 错误处理

服务器提供完善的错误处理机制：

- **网络错误**: 连接失败、接收超时等
- **解析错误**: HTTP请求格式错误
- **路由错误**: 未找到匹配的路由
- **文件错误**: 静态文件不存在或权限问题
- **内部错误**: 处理函数异常

所有错误都会记录到日志系统，并返回适当的HTTP状态码给客户端。



## 3. 公共接口

### 3.1 构造函数和析构函数

#### `HttpServer(int port, size_t thread_count)`

创建HTTP服务器实例。

**参数:**
- `port`: 监听端口号
- `thread_count`: 线程池大小，默认为硬件并发数

**功能:**
- 创建并配置服务器socket
- 绑定到指定端口
- 初始化线程池
- 设置socket选项（SO_REUSEADDR）

**异常:**
- `std::runtime_error`: socket创建、绑定或监听失败时抛出

#### `~HttpServer()`

析构函数，自动停止服务器并清理资源。

### 3.2 路由管理

#### `void addHandler(const Route &route)`

注册新的路由处理器。

**参数:**
- `route`: 路由配置，包含路径、方法、处理函数和认证设置

### 3.3 中间件

#### `void setMiddleware(Middleware middleware)`

设置全局中间件函数。

**参数:**
- `middleware`: 中间件函数，用于请求预处理

### 3.4 静态文件服务

#### `void setStaticDirectory(const std::string &dir)`

设置静态文件服务目录。

**参数:**
- `dir`: 静态文件根目录路径

**功能:**
- 自动处理GET请求的静态文件
- 支持常见MIME类型识别
- 提供基础的安全检查（防目录遍历）

### 3.5 服务器控制

#### `void run()`

启动服务器，开始监听和处理请求。

**功能:**
- 进入主事件循环
- 接受客户端连接
- 将请求分发到线程池处理
- 阻塞调用，直到服务器停止

#### `void stop()`

停止服务器运行。

**功能:**
- 设置停止标志
- 关闭服务器socket
- 中断accept()调用

### 3.6 测试接口

#### `HttpResponse routeRequest(const HttpRequest &request)`

执行路由分发逻辑，主要用于测试。

**参数:**
- `request`: HTTP请求对象

**返回值:**
- `HttpResponse`: 处理后的响应对象

#### `HttpResponse serveStaticFile(const std::string &path)`

处理静态文件请求。

**参数:**
- `path`: 请求的文件路径

**返回值:**
- `HttpResponse`: 文件响应或错误响应

