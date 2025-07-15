#include <gtest/gtest.h>
#include "http/http_request.hpp" // 确保路径正确

TEST(HttpRequestTest, ParseBasicGetRequest) {
    const std::string raw_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "\r\n";

    auto request_opt = http::HttpRequest::parse(raw_request);

    ASSERT_TRUE(request_opt.has_value());
    const auto& req = *request_opt;

    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getPath(), "/index.html");
    EXPECT_EQ(req.getVersion(), "HTTP/1.1");
    EXPECT_TRUE(req.getBody().empty());
    EXPECT_TRUE(req.hasHeader("Host"));
    EXPECT_EQ(req.getHeaderValue("Host").value(), "www.example.com");
}

TEST(HttpRequestTest, ParseRequestWithHeaders) {
    const std::string raw_request = 
        "GET /api/users HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "User-Agent: MyTestClient/1.0\r\n"
        "accept: application/json\r\n" // 小写 accept
        "\r\n";
    
    auto request_opt = http::HttpRequest::parse(raw_request);

    ASSERT_TRUE(request_opt.has_value());
    const auto& req = *request_opt;

    // 测试大小写不敏感
    EXPECT_TRUE(req.hasHeader("Host"));
    EXPECT_TRUE(req.hasHeader("host"));
    EXPECT_TRUE(req.hasHeader("HOST"));
    
    EXPECT_EQ(req.getHeaderValue("user-agent").value(), "MyTestClient/1.0");
    EXPECT_EQ(req.getHeaderValue("Accept").value(), "application/json"); // 用大写 Accept 查询
    EXPECT_FALSE(req.hasHeader("Connection"));
}

TEST(HttpRequestTest, ParseRequestWithQueryParams) {
    const std::string raw_request = 
        "GET /search?q=c%2B%2B%20projects&page=2 HTTP/1.1\r\n"
        "Host: www.google.com\r\n"
        "\r\n";

    auto request_opt = http::HttpRequest::parse(raw_request);
    
    ASSERT_TRUE(request_opt.has_value());
    const auto& req = *request_opt;

    EXPECT_EQ(req.getPath(), "/search"); // 路径应被正确分离
    EXPECT_TRUE(req.hasQueryParam("q"));
    EXPECT_TRUE(req.hasQueryParam("page"));
    EXPECT_FALSE(req.hasQueryParam("limit"));

    EXPECT_EQ(req.getQueryParam("q").value(), "c++ projects"); // 验证URL解码
    EXPECT_EQ(req.getQueryParam("page").value(), "2");
}

TEST(HttpRequestTest, ParseRequestWithCookies) {
    const std::string raw_request = 
        "GET /profile HTTP/1.1\r\n"
        "Host: my.site.com\r\n"
        "Cookie: session_id=abc123xyz; theme=dark; tracking=false\r\n"
        "\r\n";

    auto request_opt = http::HttpRequest::parse(raw_request);
    
    ASSERT_TRUE(request_opt.has_value());
    const auto& req = *request_opt;

    EXPECT_TRUE(req.hasCookie("session_id"));
    EXPECT_TRUE(req.hasCookie("theme"));
    EXPECT_TRUE(req.hasCookie("tracking"));

    EXPECT_EQ(req.getCookieValue("session_id").value(), "abc123xyz");
    EXPECT_EQ(req.getCookieValue("theme").value(), "dark");
    EXPECT_FALSE(req.hasCookie("lang"));
}

TEST(HttpRequestTest, ParsePostRequestWithBody) {
    const std::string body = "{\"username\":\"test\",\"password\":\"12345\"}";
    const std::string raw_request = 
        "POST /login HTTP/1.1\r\n"
        "Host: auth.example.com\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.length()) + "\r\n"
        "\r\n" +
        body;

    auto request_opt = http::HttpRequest::parse(raw_request);

    ASSERT_TRUE(request_opt.has_value());
    const auto& req = *request_opt;

    EXPECT_EQ(req.getMethod(), "POST");
    EXPECT_EQ(req.getPath(), "/login");
    ASSERT_TRUE(req.hasHeader("Content-Length"));
    EXPECT_EQ(std::stoul(req.getHeaderValue("Content-Length").value().data()), body.length());
    EXPECT_EQ(req.getBody(), body);
}

TEST(HttpRequestTest, HandleMalformedRequests) {
    // 1. 空请求
    EXPECT_FALSE(http::HttpRequest::parse("").has_value());

    // 2. 请求行不完整
    EXPECT_FALSE(http::HttpRequest::parse("GET / HTTP/1.1").has_value()); // 缺少结尾的\r\n
    EXPECT_FALSE(http::HttpRequest::parse("GET / \r\n\r\n").has_value()); // 缺少版本

    // 3. Content-Length 值无效
    const std::string invalid_cl_request = 
        "POST /data HTTP/1.1\r\n"
        "Host: local\r\n"
        "Content-Length: not-a-number\r\n"
        "\r\n"
        "some data";
    EXPECT_FALSE(http::HttpRequest::parse(invalid_cl_request).has_value());
}