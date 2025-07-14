#include "../../src/http/http_server.hpp"
#include "../../src/http/http_request.hpp"
#include "../../src/http/http_response.hpp"
#include "../../src/utils/logger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

using namespace http;

// 简单的断言宏
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "ASSERTION FAILED: " << #a << " != " << #b \
                  << " (" << (a) << " != " << (b) << ")" \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    } \
} while(0)

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        std::cerr << "ASSERTION FAILED: " << #condition \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    } \
} while(0)

#define ASSERT_FALSE(condition) do { \
    if (condition) { \
        std::cerr << "ASSERTION FAILED: " << #condition << " should be false" \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    } \
} while(0)

class HttpServerTester {
private:
    const int test_port = 18081;  // 使用不同的端口避免冲突
    
    // 发送HTTP请求的辅助函数
    std::string sendHttpRequest(const std::string& request) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return "";
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(test_port);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        // 设置超时
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return "";
        }
        
        send(sock, request.c_str(), request.length(), 0);
        
        char buffer[4096] = {0};
        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        close(sock);
        
        if (bytes_received > 0) {
            return std::string(buffer, bytes_received);
        }
        return "";
    }
    
    // 等待端口可用
    bool waitForPort(int max_attempts = 10) {
        for (int i = 0; i < max_attempts; ++i) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                continue;
            }
            
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(test_port);
            server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            
            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
                close(sock);
                return true;  // 端口可用
            }
            close(sock);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return false;
    }

public:
    bool testBasicConstruction() {
        std::cout << "Testing basic HttpServer construction..." << std::endl;
        
        try {
            HttpServer server(test_port);
            // 如果构造成功，测试通过
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Construction failed with exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testInvalidPort() {
        std::cout << "Testing invalid port handling..." << std::endl;
        
        // 先创建一个服务器占用测试端口
        try {
            HttpServer server1(test_port);
            
            // 然后尝试在同一端口创建另一个服务器
            try {
                HttpServer server2(test_port);
                std::cout << "Expected exception for already used port, but construction succeeded" << std::endl;
                return false; // 应该抛出异常因为端口已被占用
            } catch (const std::exception& e) {
                // 预期行为 - 端口已被占用
                std::cout << "Got expected exception for port already in use: " << e.what() << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cout << "Unexpected exception in first server creation: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testServerStartStop() {
        std::cout << "Testing server start/stop..." << std::endl;
        
        try {
            HttpServer server(test_port);
            
            // 启动服务器（在后台线程中）
            std::thread server_thread([&server]() {
                try {
                    server.run();
                } catch (const std::exception& e) {
                    std::cerr << "Server thread exception: " << e.what() << std::endl;
                }
            });
            
            // 等待服务器启动
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 检查端口是否可连接
            bool port_available = waitForPort(5);
            
            // 停止服务器
            server.stop();
            
            if (server_thread.joinable()) {
                // 使用超时来避免无限等待
                auto start_time = std::chrono::steady_clock::now();
                bool joined = false;
                while (!joined && std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count() < 2000) {
                    if (server_thread.joinable()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    } else {
                        joined = true;
                        break;
                    }
                }
                if (server_thread.joinable()) {
                    server_thread.join();
                }
            }
            
            ASSERT_TRUE(port_available);
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Server start/stop test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testBasicHttpRequest() {
        std::cout << "Testing basic HTTP request handling..." << std::endl;
        
        try {
            HttpServer server(test_port);
            
            // 添加一个简单的路由
            server.addHandler("/test", "GET", [](const HttpRequest& req) -> HttpResponse {
                return HttpResponse(200, "Test Response");
            });
            
            // 启动服务器
            std::thread server_thread([&server]() {
                try {
                    server.run();
                } catch (const std::exception& e) {
                    std::cerr << "Server thread exception: " << e.what() << std::endl;
                }
            });
            
            // 等待服务器启动
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // 发送请求
            std::string request = "GET /test HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
            std::string response = sendHttpRequest(request);
            
            // 停止服务器
            server.stop();
            if (server_thread.joinable()) {
                // 给服务器线程一些时间来停止
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                server_thread.join();
            }
            
            // 检查响应
            if (response.empty()) {
                std::cerr << "No response received from server" << std::endl;
                return false;
            }
            
            ASSERT_TRUE(response.find("200 OK") != std::string::npos);
            ASSERT_TRUE(response.find("Test Response") != std::string::npos);
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Basic HTTP request test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testRouteHandling() {
        std::cout << "Testing route handling..." << std::endl;
        
        try {
            HttpServer server(test_port);
            
            // 添加多个路由
            server.addHandler("/hello", "GET", [](const HttpRequest& req) -> HttpResponse {
                return HttpResponse(200, "Hello World");
            });
            
            server.addHandler("/data", "POST", [](const HttpRequest& req) -> HttpResponse {
                return HttpResponse(201, "Data Created");
            });
            
            // 启动服务器
            std::thread server_thread([&server]() {
                try {
                    server.run();
                } catch (const std::exception& e) {
                    std::cerr << "Server thread exception: " << e.what() << std::endl;
                }
            });
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // 测试GET请求
            std::string get_request = "GET /hello HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
            std::string get_response = sendHttpRequest(get_request);
            
            // 测试POST请求
            std::string post_request = "POST /data HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            std::string post_response = sendHttpRequest(post_request);
            
            // 测试404
            std::string not_found_request = "GET /notfound HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
            std::string not_found_response = sendHttpRequest(not_found_request);
            
            server.stop();
            if (server_thread.joinable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                server_thread.join();
            }
            
            // 验证响应
            if (!get_response.empty()) {
                ASSERT_TRUE(get_response.find("200 OK") != std::string::npos);
                ASSERT_TRUE(get_response.find("Hello World") != std::string::npos);
            } else {
                std::cerr << "No GET response received" << std::endl;
                return false;
            }
            
            if (!post_response.empty()) {
                ASSERT_TRUE(post_response.find("201 Created") != std::string::npos);
                ASSERT_TRUE(post_response.find("Data Created") != std::string::npos);
            } else {
                std::cerr << "No POST response received" << std::endl;
                return false;
            }
            
            if (!not_found_response.empty()) {
                ASSERT_TRUE(not_found_response.find("404 Not Found") != std::string::npos);
            } else {
                std::cerr << "No 404 response received" << std::endl;
                return false;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Route handling test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testStaticFileServing() {
        std::cout << "Testing static file serving..." << std::endl;
        
        try {
            // 创建测试文件
            std::string test_dir = "/tmp/test_static";
            system(("mkdir -p " + test_dir).c_str());
            
            std::ofstream test_file(test_dir + "/test.html");
            test_file << "<html><body>Test Static File</body></html>";
            test_file.close();
            
            HttpServer server(test_port);
            server.setStaticDirectory(test_dir);
            
            // 启动服务器
            std::thread server_thread([&server]() {
                try {
                    server.run();
                } catch (const std::exception& e) {
                    std::cerr << "Server thread exception: " << e.what() << std::endl;
                }
            });
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // 请求静态文件
            std::string request = "GET /test.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
            std::string response = sendHttpRequest(request);
            
            server.stop();
            if (server_thread.joinable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                server_thread.join();
            }
            
            // 清理测试文件
            system(("rm -rf " + test_dir).c_str());
            
            // 验证响应
            if (response.empty()) {
                std::cerr << "No response received for static file" << std::endl;
                return false;
            }
            
            ASSERT_TRUE(response.find("200 OK") != std::string::npos);
            ASSERT_TRUE(response.find("Test Static File") != std::string::npos);
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Static file serving test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testErrorHandling() {
        std::cout << "Testing error handling..." << std::endl;
        
        try {
            HttpServer server(test_port);
            
            // 添加一个会抛出异常的路由
            server.addHandler("/error", "GET", [](const HttpRequest& req) -> HttpResponse {
                throw std::runtime_error("Test error");
                return HttpResponse(200, "Should not reach here");
            });
            
            // 启动服务器
            std::thread server_thread([&server]() {
                try {
                    server.run();
                } catch (const std::exception& e) {
                    std::cerr << "Server thread exception: " << e.what() << std::endl;
                }
            });
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // 发送请求到错误路由
            std::string request = "GET /error HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
            std::string response = sendHttpRequest(request);
            
            server.stop();
            if (server_thread.joinable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                server_thread.join();
            }
            
            // 应该返回500错误
            if (response.empty()) {
                std::cerr << "No response received for error test" << std::endl;
                return false;
            }
            
            ASSERT_TRUE(response.find("500") != std::string::npos);
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error handling test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    void runAllTests() {
        std::cout << "Starting HttpServer tests...\n" << std::endl;
        
        int passed = 0;
        int total = 0;
        
        std::cout << "Test 1: "; ++total; if (testBasicConstruction()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 2: "; ++total; if (testInvalidPort()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 3: "; ++total; if (testServerStartStop()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 4: "; ++total; if (testBasicHttpRequest()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 5: "; ++total; if (testRouteHandling()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 6: "; ++total; if (testStaticFileServing()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 7: "; ++total; if (testErrorHandling()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "HttpServer Tests Results: " << passed << "/" << total << " tests passed" << std::endl;
        
        if (passed == total) {
            std::cout << "✓ All HttpServer tests PASSED!" << std::endl;
        } else {
            std::cout << "✗ Some HttpServer tests FAILED!" << std::endl;
        }
        std::cout << "========================================\n" << std::endl;
    }
};

int main() {
    // 设置日志级别
    utils::Logger::setGlobalLevel(utils::LogLevel::WARN); // 减少测试时的日志输出
    
    HttpServerTester tester;
    tester.runAllTests();
    
    return 0;
}
