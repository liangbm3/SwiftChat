# HTTP 响应类 (HttpResponse) API 文档

## 1\. 概述 (Overview)

`http::HttpResponse` 类是一个用于轻松构建和序列化HTTP响应的强大工具。它遵循**流式接口 (Fluent Interface)** 和**工厂模式 (Factory Pattern)**，旨在让创建HTTP响应的过程既简单、清晰，又富有表现力。

### 核心设计理念

  * **构建器模式 (Builder Pattern)**: `HttpResponse` 对象被设计为构建器。您可以从一个默认的响应开始，或者使用一个静态工厂方法（如 `Ok()`, `NotFound()`）创建一个特定类型的响应，然后通过链式调用 `with...` 方法来逐步添加或修改头部和响应体。
  * **封装与安全**: 响应对象的状态（状态码、头部、响应体）被封装为私有成员。只能通过提供的公共方法进行修改，这保证了对象状态的有效性（例如，`Content-Length` 会被自动管理）。
  * **便利性**: 为常见的HTTP响应（如404, 500等）提供了静态工厂方法，并内置了对 `nlohmann::json` 的无缝支持。

## 2\. API 详解

### 2.1 创建响应对象 (Creating Response Objects)

创建响应的第一步通常是调用默认构造函数或一个静态工厂方法。

-----

#### `HttpResponse()`

  * **描述**: 默认构造函数。创建一个基础的、状态为 `200 OK` 的空响应。它会包含 `Server`, `Date`, `Connection` 等默认头部。
  * **示例**: `http::HttpResponse resp;`

-----

#### `static HttpResponse Ok(const std::string& body = "OK")`

  * **描述**: 创建一个 `200 OK` 响应。
  * **参数**: `body` (`const std::string&`, 可选): 响应体内容，默认为 `"OK"`。
  * **示例**: `auto resp = http::HttpResponse::Ok("操作成功");`

-----

#### `static HttpResponse Created(const std::string& body = "Created")`

  * **描述**: 创建一个 `201 Created` 响应。通常用于资源创建成功的场景。响应体会被自动包装成JSON格式 `{"message": "..."}`。
  * **示例**: `auto resp = http::HttpResponse::Created("用户创建成功");`

-----

#### `static HttpResponse NoContent()`

  * **描述**: 创建一个 `204 No Content` 响应。根据HTTP规范，此响应**不包含**任何响应体和 `Content-Length` 头部。
  * **示例**: `auto resp = http::HttpResponse::NoContent();`

-----

#### `static HttpResponse BadRequest(const std::string& error_message = "Bad Request")`

  * **描述**: 创建一个 `400 Bad Request` 响应。响应体会被自动包装成JSON格式 `{"error": "..."}`。
  * **示例**: `auto resp = http::HttpResponse::BadRequest("请求参数无效");`

-----

*... 其他静态工厂方法 (`Unauthorized`, `Forbidden`, `NotFound`, `InternalError` 等) 的用法与 `BadRequest` 类似。*

-----

### 2.2 构建与定制 (流式接口)

这些方法返回 `HttpResponse&`，允许进行方法链式调用。

-----

#### `HttpResponse& withStatus(int code)`

  * **描述**: 设置或修改响应的HTTP状态码。
  * **参数**: `code` (`int`): 一个标准的HTTP状态码，如 `200`, `404`。
  * **返回值**: `HttpResponse&` - 对象自身的引用，用于链式调用。
  * **示例**: `resp.withStatus(202);`

-----

#### `HttpResponse& withHeader(const std::string& key, const std::string& value)`

  * **描述**: 添加或覆盖一个HTTP响应头。
  * **参数**:
      * `key` (`const std::string&`): 头部的名称，例如 `"Content-Type"`。
      * `value` (`const std::string&`): 头部的值。
  * **返回值**: `HttpResponse&` - 对象自身的引用。
  * **示例**: `resp.withHeader("X-Custom-Info", "some-value");`

-----

#### `HttpResponse& withBody(const std::string& body_content, const std::string& content_type = "text/plain")`

  * **描述**: 设置响应体为纯文本或二进制内容。
  * **参数**:
      * `body_content` (`const std::string&`): 响应体的内容。
      * `content_type` (`const std::string&`, 可选): 该内容的MIME类型，默认为 `"text/plain"`。
  * **返回值**: `HttpResponse&` - 对象自身的引用。
  * **示例**: `resp.withBody("<html>...</html>", "text/html");`

-----

#### `HttpResponse& withJsonBody(const nlohmann::json& json_body)`

  * **描述**: 将一个 `nlohmann::json` 对象设置为响应体。该方法会自动将JSON对象序列化为字符串，并设置 `Content-Type` 为 `application/json; charset=utf-8`。
  * **参数**: `json_body` (`const nlohmann::json&`): 要发送的JSON对象。
  * **返回值**: `HttpResponse&` - 对象自身的引用。

-----

### 2.3 序列化 (Serialization)

-----

#### `std::string toString() const`

  * **描述**: 将构建好的 `HttpResponse` 对象序列化为一个完整的、符合HTTP规范的字符串。这是构建响应的最后一步，得到的字符串可以直接发送给客户端。该方法会自动处理 `Content-Length` 和特殊状态码（如`204`）的响应体。
  * **返回值**: `std::string` - 可直接发送的HTTP响应报文。

-----

### 3\. 综合使用示例

```cpp
#include "http/http_response.hpp"
#include <iostream>

// 示例1: 创建一个简单的 404 Not Found 响应
void sendNotFound() {
    auto response = http::HttpResponse::NotFound("您请求的资源不存在。");
    std::cout << response.toString() << std::endl;
}

// 示例2: 创建一个成功的JSON数据响应
void sendUserData() {
    nlohmann::json user_data = {
        {"user_id", "user_123"},
        {"username", "alice"},
        {"email", "alice@example.com"}
    };
    
    // 从一个200 OK响应开始，然后用JSON设置body
    auto response = http::HttpResponse::Ok().withJsonBody(user_data);
    std::cout << response.toString() << std::endl;
}

// 示例3: 使用方法链创建一个自定义的 201 Created 响应
void sendCreationResponse() {
    nlohmann::json response_body = { {"message", "资源创建成功"}, {"id", 5} };

    http::HttpResponse response;
    response.withStatus(201)
            .withHeader("Location", "/api/items/5")
            .withHeader("X-Custom-Trace-ID", "trace-xyz")
            .withJsonBody(response_body);

    std::cout << response.toString() << std::endl;
}
```