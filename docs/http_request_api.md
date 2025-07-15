# HTTP 请求类 (HttpRequest) API 文档

## 1\. 概述 (Overview)

`http::HttpRequest` 类是一个用于解析和表示HTTP请求的核心工具。它被设计为一个**不可变 (Immutable)** 的数据对象：一旦通过静态工厂方法 `parse()` 创建成功，其内部状态便无法从外部修改。

这确保了在处理请求的整个生命周期中，请求数据的一致性和线程安全性。

### 核心特性

  * **封装与安全**: 所有请求数据均为私有成员，只能通过公共的 `get` 方法访问。
  * **健壮的解析**: `parse` 方法能处理常见的HTTP格式，并对格式错误的请求进行安全处理。
  * **明确的错误处理**: 解析函数返回 `std::optional<HttpRequest>`，强制调用者必须处理解析失败的可能性。
  * **头部大小写不敏感**: 查找HTTP头部时，键名（key）不区分大小写，符合HTTP规范。
  * **自动解析**: URL查询参数和Cookies会被自动解析并存入易于访问的映射中。

## 2\. API 详解

### 2.1 创建请求对象 (Creating a Request Object)

`HttpRequest` 对象**只能**通过静态方法 `HttpRequest::parse()` 来创建。其构造函数是私有的，以保证所有实例都是经过完整解析的。

-----

#### `static std::optional<HttpRequest> parse(const std::string& raw_request)`

  * **描述**:
    解析一个原始的、基于字符串的HTTP请求，并返回一个`HttpRequest`对象。这是与该类交互的唯一入口点。
  * **参数**:
      * `raw_request` (`const std::string&`): 完整的HTTP请求文本，应包含请求行、所有头部，以及一个由`\r\n\r\n`分隔的请求体（如果存在）。
  * **返回值**: `std::optional<HttpRequest>`
      * 如果解析**成功**，返回一个包含`HttpRequest`实例的`optional`对象。
      * 如果解析**失败**（例如，请求格式不正确、缺少关键部分），返回`std::nullopt`。**调用者必须检查返回值是否存在！**

-----

### 2.2 获取基本请求信息 (Accessing Basic Request Information)

-----

#### `const std::string& getMethod() const`

  * **描述**: 获取HTTP请求方法。
  * **返回值**: `const std::string&` - 例如 `"GET"`, `"POST"`。

-----

#### `const std::string& getPath() const`

  * **描述**: 获取请求的路径部分（不包含查询参数）。
  * **返回值**: `const std::string&` - 例如 `"/api/users/123"`。

-----

#### `const std::string& getVersion() const`

  * **描述**: 获取HTTP协议版本。
  * **返回值**: `const std::string&` - 例如 `"HTTP/1.1"`。

-----

#### `const std::string& getBody() const`

  * **描述**: 获取HTTP请求体。
  * **返回值**: `const std::string&` - 如果请求没有请求体，则返回空字符串。

-----

### 2.3 处理请求头 (Working with Headers)

-----

#### `bool hasHeader(const std::string& key) const`

  * **描述**: 检查是否存在指定的HTTP头部。**查找时忽略键的大小写**。
  * **参数**:
      * `key` (`const std::string&`): 头部的名称，例如 `"Content-Type"`。
  * **返回值**: `bool` - `true` 表示存在，`false` 表示不存在。

-----

#### `std::optional<std::string_view> getHeaderValue(const std::string& key) const`

  * **描述**: 获取指定HTTP头部的值。**查找时忽略键的大小写**。
  * **参数**:
      * `key` (`const std::string&`): 头部的名称。
  * **返回值**: `std::optional<std::string_view>`
      * 如果头部存在，返回一个包含其值的`optional`对象。使用 `string_view` 是为了避免不必要的内存拷贝，提升效率。
      * 如果头部不存在，返回`std::nullopt`。

-----

### 2.4 处理URL查询参数 (Working with URL Query Parameters)

-----

#### `bool hasQueryParam(const std::string& key) const`

  * **描述**: 检查URL中是否存在指定的查询参数。
  * **参数**:
      * `key` (`const std::string&`): 查询参数的名称。
  * **返回值**: `bool` - `true` 表示存在，`false` 表示不存在。

-----

#### `std::optional<std::string_view> getQueryParam(const std::string& key) const`

  * **描述**: 获取指定URL查询参数的值。值是经过URL解码的。
  * **参数**:
      * `key` (`const std::string&`): 查询参数的名称。
  * **返回值**: `std::optional<std::string_view>` - 如果参数存在，返回其值；否则返回 `std::nullopt`。

-----

### 2.5 处理Cookies

-----

#### `bool hasCookie(const std::string& key) const`

  * **描述**: 检查是否存在指定的Cookie。
  * **参数**:
      * `key` (`const std::string&`): Cookie的名称。
  * **返回值**: `bool` - `true` 表示存在，`false` 表示不存在。

-----

#### `std::optional<std::string_view> getCookieValue(const std::string& key) const`

  * **描述**: 获取指定Cookie的值。
  * **参数**:
      * `key` (`const std::string&`): Cookie的名称。
  * **返回值**: `std::optional<std::string_view>` - 如果Cookie存在，返回其值；否则返回 `std::nullopt`。

-----

### 3\. 综合使用示例 (Comprehensive Usage Example)

```cpp
#include "http/http_request.hpp"
#include <iostream>

void processRequest(const std::string& raw_request_string) {
    // 步骤 1: 调用静态方法 parse() 来创建请求对象
    auto request_opt = http::HttpRequest::parse(raw_request_string);

    // 步骤 2: 必须检查 optional 是否有值
    if (!request_opt) {
        std::cerr << "HTTP请求解析失败！" << std::endl;
        // 在实际应用中，这里应该向客户端返回 400 Bad Request
        return;
    }

    // 步骤 3: 从 optional 中安全地获取请求对象
    const http::HttpRequest& request = *request_opt;

    // 步骤 4: 使用 getter 方法访问请求的各个部分
    std::cout << "收到请求: " << request.getMethod() << " " << request.getPath() << std::endl;

    // 检查并获取 Header (大小写不敏感)
    if (request.hasHeader("User-Agent")) {
        auto user_agent = request.getHeaderValue("user-agent").value(); // .value()在确定有值时使用
        std::cout << "客户端: " << user_agent << std::endl;
    }

    // 检查并获取 Query Param
    if (request.hasQueryParam("user_id")) {
        auto user_id = request.getQueryParam("user_id").value();
        std::cout << "查询用户ID: " << user_id << std::endl;
    }

    // 检查并获取 Cookie
    auto session_id_opt = request.getCookieValue("session_id");
    if (session_id_opt) {
        std::cout << "会话ID: " << *session_id_opt << std::endl; // 也可以用 * 来获取值
    } else {
        std::cout << "请求中不包含会话ID Cookie。" << std::endl;
    }
    
    // 获取请求体
    if (!request.getBody().empty()) {
        std::cout << "请求体内容: " << request.getBody() << std::endl;
    }
}

int main() {
    const std::string sample_request =
        "POST /api/messages?room_id=123 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: MyClient/1.0\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "Cookie: session_id=xyz-abc-123\r\n"
        "\r\n"
        "{\"message\":\"Hello, world!\"}";

    processRequest(sample_request);

    return 0;
}
```