#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <filesystem> // C++17, 用于文件系统操作
#include "http/http_server.hpp"

using namespace testing;
namespace fs = std::filesystem;

// --- API 路由和中间件测试 ---
class HttpServerTest : public ::testing::Test {
protected:
    // 在这个测试套件中，我们不需要真正的网络监听，
    // 因此构造函数传入一个任意端口即可。
    // 我们将直接调用其内部的路由方法进行测试。
    http::HttpServer server_{8080}; 
};

TEST_F(HttpServerTest, BasicRouting) {
    // 注册一个简单的处理器
    server_.addHandler("/hello", "GET", [](const http::HttpRequest& req) {
        return http::HttpResponse::Ok("Hello, World!");
    });

    // 构造一个请求
    auto request_opt = http::HttpRequest::parse("GET /hello HTTP/1.1\r\n\r\n");
    ASSERT_TRUE(request_opt.has_value());

    // 直接调用路由方法进行测试
    auto response = server_.routeRequest(*request_opt);
    auto response_str = response.toString();
    
    EXPECT_THAT(response_str, StartsWith("HTTP/1.1 200 OK"));
    EXPECT_THAT(response_str, EndsWith("Hello, World!"));
}

TEST_F(HttpServerTest, RouteNotFound) {
    auto request_opt = http::HttpRequest::parse("GET /not-found HTTP/1.1\r\n\r\n");
    ASSERT_TRUE(request_opt.has_value());

    auto response = server_.routeRequest(*request_opt);
    auto response_str = response.toString();

    EXPECT_THAT(response_str, StartsWith("HTTP/1.1 404 Not Found"));
}

TEST_F(HttpServerTest, MethodNotAllowed) {
    server_.addHandler("/resource", "POST", [](const http::HttpRequest& req) {
        return http::HttpResponse::Created();
    });

    auto request_opt = http::HttpRequest::parse("GET /resource HTTP/1.1\r\n\r\n");
    ASSERT_TRUE(request_opt.has_value());

    auto response = server_.routeRequest(*request_opt);
    auto response_str = response.toString();

    EXPECT_THAT(response_str, StartsWith("HTTP/1.1 400 Bad Request")); // 根据您的实现，可能是400或405
}

TEST_F(HttpServerTest, MiddlewareExecution) {
    // 添加一个中间件，它会给响应添加一个自定义Header
    server_.addMiddleware([](const http::HttpRequest& req, const http::HttpServer::RequestHandler& next) {
        auto response = next(req); // 先调用核心处理器
        response.withHeader("X-Middleware-Applied", "true"); // 再修改响应
        return response;
    });

    server_.addHandler("/mw-test", "GET", [](const http::HttpRequest& req) {
        return http::HttpResponse::Ok("Handler executed");
    });
    
    auto request_opt = http::HttpRequest::parse("GET /mw-test HTTP/1.1\r\n\r\n");
    ASSERT_TRUE(request_opt.has_value());

    auto response = server_.routeRequest(*request_opt);
    auto response_str = response.toString();
    
    EXPECT_THAT(response_str, StartsWith("HTTP/1.1 200 OK"));
    EXPECT_THAT(response_str, HasSubstr("X-Middleware-Applied: true\r\n"));
    EXPECT_THAT(response_str, EndsWith("Handler executed"));
}

// --- 静态文件服务测试 ---
class StaticFileTest : public ::testing::Test {
protected:
    http::HttpServer server_{8081};
    fs::path static_dir_ = "./test_static_temp";

    void SetUp() override {
        // 创建临时静态文件目录和文件
        fs::create_directory(static_dir_);
        server_.setStaticDirectory(static_dir_.string());

        std::ofstream test_file(static_dir_ / "index.html");
        test_file << "<html><body>Hello Static</body></html>";
        test_file.close();
    }

    void TearDown() override {
        // 清理临时文件和目录
        fs::remove_all(static_dir_);
    }
};

TEST_F(StaticFileTest, ServesExistingFile) {
    auto request_opt = http::HttpRequest::parse("GET /index.html HTTP/1.1\r\n\r\n");
    ASSERT_TRUE(request_opt.has_value());

    // 直接调用静态文件服务方法
    auto response = server_.serveStaticFile(request_opt->getPath());
    auto response_str = response.toString();

    EXPECT_THAT(response_str, StartsWith("HTTP/1.1 200 OK"));
    EXPECT_THAT(response_str, HasSubstr("Content-Type: text/html\r\n"));
    EXPECT_THAT(response_str, EndsWith("<html><body>Hello Static</body></html>"));
}

TEST_F(StaticFileTest, ReturnsNotFoundForMissingFile) {
    auto request_opt = http::HttpRequest::parse("GET /missing.css HTTP/1.1\r\n\r\n");
    ASSERT_TRUE(request_opt.has_value());

    auto response = server_.serveStaticFile(request_opt->getPath());
    auto response_str = response.toString();
    
    EXPECT_THAT(response_str, StartsWith("HTTP/1.1 404 Not Found"));
}

TEST_F(StaticFileTest, PreventsPathTraversal) {
    auto request_opt = http::HttpRequest::parse("GET /../secret.txt HTTP/1.1\r\n\r\n");
    ASSERT_TRUE(request_opt.has_value());
    
    auto response = server_.serveStaticFile(request_opt->getPath());
    auto response_str = response.toString();

    EXPECT_THAT(response_str, StartsWith("HTTP/1.1 403 Forbidden"));
}