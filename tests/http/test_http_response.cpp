#include <gtest/gtest.h>
#include <gmock/gmock.h> // GTest的配套库，提供了更丰富的匹配器
#include "http/http_response.hpp" // 确保路径正确

using namespace testing;

TEST(HttpResponseTest, DefaultConstructorIs200OK) {
    http::HttpResponse resp;
    const auto resp_str = resp.toString();

    EXPECT_THAT(resp_str, StartsWith("HTTP/1.1 200 OK\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("Content-Length: 0\r\n"));
    EXPECT_THAT(resp_str, EndsWith("\r\n\r\n"));
}

TEST(HttpResponseTest, StaticFactoryForNotFound) {
    // 测试静态工厂方法是否正确设置状态码和默认的JSON body
    auto resp = http::HttpResponse::NotFound("Resource not available");
    const auto resp_str = resp.toString();

    const std::string expected_body = "{\"error\":\"Resource not available\"}";

    EXPECT_THAT(resp_str, StartsWith("HTTP/1.1 404 Not Found\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("Content-Type: application/json; charset=utf-8\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("Content-Length: " + std::to_string(expected_body.length()) + "\r\n"));
    EXPECT_THAT(resp_str, EndsWith("\r\n\r\n" + expected_body));
}

TEST(HttpResponseTest, FluentInterfaceChaining) {
    http::HttpResponse resp;

    // 使用链式调用来构建响应
    resp.withStatus(418) // I'm a teapot
        .withHeader("X-Custom-Header", "Hello C++")
        .withBody("I'm a teapot", "text/plain");

    const auto resp_str = resp.toString();

    EXPECT_THAT(resp_str, StartsWith("HTTP/1.1 418 Unknown\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("X-Custom-Header: Hello C++\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("Content-Type: text/plain\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("Content-Length: 13\r\n")); // "I'm a teapot" 的长度
    EXPECT_THAT(resp_str, EndsWith("\r\n\r\nI'm a teapot"));
}

TEST(HttpResponseTest, WithJsonBody) {
    nlohmann::json json_payload = {
        {"status", "success"},
        {"data", {1, "two", 3.0}}
    };

    // 使用 withJsonBody 设置响应体
    auto resp = http::HttpResponse::Ok().withJsonBody(json_payload);
    const auto resp_str = resp.toString();
    
    // nlohmann::json::dump() 会生成无空格的字符串
    const std::string expected_body = json_payload.dump();

    EXPECT_THAT(resp_str, StartsWith("HTTP/1.1 200 OK\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("Content-Type: application/json; charset=utf-8\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("Content-Length: " + std::to_string(expected_body.length()) + "\r\n"));
    EXPECT_THAT(resp_str, EndsWith("\r\n\r\n" + expected_body));
}

TEST(HttpResponseTest, HeaderOverwriting) {
    // 验证后设置的 header 会覆盖之前的同名 header
    auto resp = http::HttpResponse::Ok()
                  .withHeader("Cache-Control", "no-cache")
                  .withHeader("cache-control", "max-age=3600"); // key 是大小写不敏感的，但这里是标准 map，所以会区分

    const auto resp_str = resp.toString();

    // 注意：std::unordered_map 是大小写敏感的，所以这里会存在两个header
    // 如果您希望 header 的 key 也不敏感，需要像 HttpRequest 那样使用自定义比较器
    // 这里我们测试当前实现的行为
    EXPECT_THAT(resp_str, HasSubstr("Cache-Control: no-cache\r\n"));
    EXPECT_THAT(resp_str, HasSubstr("cache-control: max-age=3600\r\n"));

    // 如果我们用完全相同的 key, 则会覆盖
    auto resp2 = http::HttpResponse::Ok()
                   .withHeader("Cache-Control", "no-cache")
                   .withHeader("Cache-Control", "max-age=3600"); 

    const auto resp_str2 = resp2.toString();
    EXPECT_THAT(resp_str2, Not(HasSubstr("Cache-Control: no-cache\r\n")));
    EXPECT_THAT(resp_str2, HasSubstr("Cache-Control: max-age=3600\r\n"));
}

TEST(HttpResponseTest, CorrectContentLength) {
    // 1. 对于没有body的响应
    auto resp_no_body = http::HttpResponse::NoContent(); // 假设我们添加一个204工厂
    // 或者用现有的
    // auto resp_no_body = http::HttpResponse::Ok("").withStatus(204);
    // 实际上204响应不应该有body，我们这里测试一个body为空字符串的情况
    auto resp_empty_body = http::HttpResponse::Ok("");
    EXPECT_THAT(resp_empty_body.toString(), HasSubstr("Content-Length: 0\r\n"));

    // 2. 对于有body的响应
    std::string body = "Hello, World!";
    auto resp_with_body = http::HttpResponse::Ok(body);
    EXPECT_THAT(resp_with_body.toString(), HasSubstr("Content-Length: " + std::to_string(body.length()) + "\r\n"));
}