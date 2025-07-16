# HTTP 服务器 (HttpServer) API 文档

## 1\. 概述 (Overview)

`http::HttpServer` 是整个Web服务的核心引擎。它负责处理底层的网络通信（TCP监听、接受连接），并内置了一个强大而灵活的**多线程请求处理**、**路由分发**和**中间件**系统。

通过使用该类，您可以将业务逻辑与复杂的网络编程解耦，专注于实现具体的API功能和静态文件服务。

### 核心概念

#### a) 请求处理器 (RequestHandler)

这是API路由的最终目的地。它是一个函数（或lambda表达式），接收一个`HttpRequest`对象，并必须返回一个`HttpResponse`对象。

```cpp
using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;
```

**职责**: 实现具体的业务逻辑，例如从数据库查询用户信息、创建新的聊天室等。

#### b) 中间件 (Middleware)

中间件是一种强大的设计模式，它允许您在请求到达最终的`RequestHandler`之前，或者在响应返回给客户端之前，对请求和响应进行一系列的“加工处理”。

```cpp
using Middleware = std::function<HttpResponse(const HttpRequest&, const RequestHandler&)>;
```

**职责**: 处理一些通用的、与具体业务无关的横切关注点，例如：

  * 记录每个请求的日志。
  * 验证用户的认证信息（如JWT Token）。
  * 为所有响应添加CORS（跨域资源共享）头。
  * 捕获异常并统一格式化错误响应。

中间件形成一个**调用链**。每个中间件都可以决定是“放行”请求到下一个中间件（通过调用`next`函数），还是直接“短路”并返回一个响应（例如，认证失败时返回`401`）。

## 2\. API 详解

### 2.1 初始化与配置

-----

#### `HttpServer(int port, size_t thread_count = std::thread::hardware_concurrency())`

  * **描述**: 构造函数。初始化服务器，设置监听端口和线程池中的工作线程数量。在这一步，服务器会完成`socket`, `bind`, `listen`等所有网络准备工作。如果失败，会抛出`std::runtime_error`。
  * **参数**:
      * `port` (`int`): 服务器要监听的TCP端口号。
      * `thread_count` (`size_t`, 可选): 线程池的工作线程数，默认为硬件支持的并发线程数。

-----

#### `void setStaticDirectory(const std::string& dir)`

  * **描述**: 设置用于提供静态文件服务的根目录。
  * **参数**:
      * `dir` (`const std::string&`): 本地文件系统中的路径，例如 `"./public"`。

-----

### 2.2 路由与中间件注册

-----

#### `void addHandler(const std::string& path, const std::string& method, RequestHandler handler)`

  * **描述**: 为一个特定的\*\*路径（Path）**和**HTTP方法（Method）\*\*组合注册一个请求处理器。
  * **参数**:
      * `path` (`const std::string&`): API的路径，例如 `"/api/users"`。**需要精确匹配**。
      * `method` (`const std::string&`): HTTP方法，例如 `"GET"`, `"POST"`。
      * `handler` (`RequestHandler`): 符合`RequestHandler`签名的函数或lambda表达式。

-----

#### `void addMiddleware(Middleware middleware)`

  * **描述**: 添加一个中间件到处理链中。中间件将按照**添加的顺序**依次执行。
  * **参数**:
      * `middleware` (`Middleware`): 符合`Middleware`签名的函数或lambda表达式。

-----

### 2.3 运行与停止

-----

#### `void run()`

  * **描述**: 启动服务器的主事件循环。这是一个**阻塞调用**，它会持续监听和接受新的客户端连接，直到`stop()`被调用。通常这是`main`函数的最后一步。

-----

#### `void stop()`

  * **描述**: 优雅地停止服务器。它会设置一个标志位来终止事件循环，并关闭服务器的监听套接字以中断阻塞的`accept()`调用。此方法是线程安全的。

-----

### 3\. 综合使用示例

这是一个模拟的 `main.cpp`，展示了如何配置和运行一个包含API、中间件和静态文件服务的完整服务器。

```cpp
#include "http/http_server.hpp"
#include "db/database_manager.hpp" // 假设这是您的数据库管理器
#include "utils/logger.hpp"      // 假设这是您的日志库

// --- 业务逻辑处理器 ---
// (在实际项目中，这些处理器可能会在不同的文件中)

HttpResponse handleUserRegister(const http::HttpRequest& req, DatabaseManager& db) {
    try {
        auto json_body = nlohmann::json::parse(req.getBody());
        std::string username = json_body.at("username");
        std::string password = json_body.at("password");

        // 密码应该在客户端加密，服务器端哈希后存储
        std::string password_hash = "hashed_" + password; // 简化处理

        if (db.createUser(username, password_hash)) {
            return http::HttpResponse::Created("User created successfully.");
        } else {
            return http::HttpResponse::BadRequest("Username already exists.");
        }
    } catch (const std::exception& e) {
        return http::HttpResponse::BadRequest(e.what());
    }
}

HttpResponse handleGetUsers(const http::HttpRequest& req, DatabaseManager& db) {
    auto users = db.getAllUsers();
    nlohmann::json json_response;
    for (const auto& user : users) {
        json_response.push_back({
            {"id", user.getId()},
            {"username", user.getUsername()},
            {"is_online", user.isOnline()}
        });
    }
    return http::HttpResponse::Ok().withJsonBody(json_response);
}


int main() {
    try {
        // 1. 初始化依赖项
        DatabaseManager db("swiftchat.db");

        // 2. 创建并配置服务器
        http::HttpServer server(8080);
        server.setStaticDirectory("./public"); // 设置静态文件目录

        // 3. 添加中间件 (按执行顺序列出)
        // 日志中间件
        server.addMiddleware([](const http::HttpRequest& req, const http::HttpServer::RequestHandler& next) {
            LOG_INFO << ">> Request Start: " << req.getMethod() << " " << req.getPath();
            auto response = next(req); // 调用下一个中间件或最终处理器
            LOG_INFO << "<< Request End";
            return response;
        });

        // 认证中间件 (示例)
        server.addMiddleware([&](const http::HttpRequest& req, const http::HttpServer::RequestHandler& next) {
            // 公开路径，无需认证
            if (req.getPath() == "/api/users/register") {
                return next(req);
            }
            
            // 简单检查认证头
            if (!req.hasHeader("Authorization")) {
                return http::HttpResponse::Unauthorized("Authorization header is missing.");
            }
            // ... 此处应有真正的Token验证逻辑 ...
            
            // 验证通过，放行
            return next(req);
        });

        // 4. 注册API路由
        // 使用 lambda 捕获 db 引用，将其注入到处理器中
        server.addHandler("/api/users/register", "POST", [&](const http::HttpRequest& req) {
            return handleUserRegister(req, db);
        });

        server.addHandler("/api/users", "GET", [&](const http::HttpRequest& req) {
            return handleGetUsers(req, db);
        });

        // 5. 启动服务器
        server.run();

    } catch (const std::exception& e) {
        LOG_ERROR << "Server startup failed: " << e.what();
        return 1;
    }

    return 0;
}
```