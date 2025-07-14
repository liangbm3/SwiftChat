#include "../../src/websocket/websocket_server.hpp"
#include "../../src/utils/logger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

// 简单的断言宏
#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        std::cerr << "ASSERTION FAILED: " << #condition \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    } \
} while(0)

class WebSocketServerTester {
private:
    const int test_port = 18082;  // 使用不同的端口避免冲突

public:
    bool testBasicConstruction() {
        std::cout << "Testing basic WebSocketServer construction..." << std::endl;
        
        try {
            WebSocketServer server;
            // 如果构造成功，测试通过
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Construction failed with exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testServerStartStop() {
        std::cout << "Testing WebSocket server start/stop..." << std::endl;
        
        try {
            WebSocketServer server;
            
            // 启动服务器
            server.start(test_port);
            
            // 等待服务器启动
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 停止服务器
            server.stop();
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Server start/stop test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testCallbackSetters() {
        std::cout << "Testing callback setters..." << std::endl;
        
        try {
            WebSocketServer server;
            
            bool message_callback_called = false;
            bool open_callback_called = false;
            bool close_callback_called = false;
            
            // 设置回调函数
            server.setOnMessage([&message_callback_called](auto hdl, auto msg) {
                message_callback_called = true;
            });
            
            server.setOnOpen([&open_callback_called](auto hdl) {
                open_callback_called = true;
            });
            
            server.setOnClose([&close_callback_called](auto hdl) {
                close_callback_called = true;
            });
            
            // 如果设置成功，返回true
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Callback setters test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testBroadcast() {
        std::cout << "Testing broadcast functionality..." << std::endl;
        
        try {
            WebSocketServer server;
            
            // 启动服务器
            server.start(test_port);
            
            // 等待服务器启动
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 测试广播（即使没有连接的客户端，也不应该崩溃）
            server.broadcast("Test broadcast message");
            
            // 停止服务器
            server.stop();
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Broadcast test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    void runAllTests() {
        std::cout << "Starting WebSocketServer tests...\n" << std::endl;
        
        int passed = 0;
        int total = 0;
        
        std::cout << "Test 1: "; ++total; if (testBasicConstruction()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 2: "; ++total; if (testServerStartStop()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 3: "; ++total; if (testCallbackSetters()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        std::cout << "Test 4: "; ++total; if (testBroadcast()) { ++passed; std::cout << "PASSED"; } else { std::cout << "FAILED"; } std::cout << std::endl;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "WebSocketServer Tests Results: " << passed << "/" << total << " tests passed" << std::endl;
        
        if (passed == total) {
            std::cout << "✓ All WebSocketServer tests PASSED!" << std::endl;
        } else {
            std::cout << "✗ Some WebSocketServer tests FAILED!" << std::endl;
        }
        std::cout << "========================================\n" << std::endl;
    }
};

int main() {
    // 设置日志级别
    utils::Logger::setGlobalLevel(utils::LogLevel::WARN); // 减少测试时的日志输出
    
    WebSocketServerTester tester;
    tester.runAllTests();
    
    return 0;
}
