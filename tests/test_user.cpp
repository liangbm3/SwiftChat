#include <iostream>
#include <cassert>
#include <stdexcept>
#include "../src/chat/user.hpp"

// 测试辅助函数：打印测试结果
void printTestResult(const std::string& testName, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

// 测试User对象转JSON
void testUserToJson() {
    std::cout << "\n=== 测试 User::toJson() ===" << std::endl;
    
    // 测试1：基本转换
    {
        User user;
        user.username = "testuser";
        user.password = "testpass";
        user.is_online = true;
        
        json j = user.toJson();
        
        bool passed = (j["username"] == "testuser" && 
                      j["password"] == "testpass" && 
                      j["is_online"] == true);
        printTestResult("基本JSON转换", passed);
    }
    
    // 测试2：空字符串处理
    {
        User user;
        user.username = "";
        user.password = "";
        user.is_online = false;
        
        json j = user.toJson();
        
        bool passed = (j["username"] == "" && 
                      j["password"] == "" && 
                      j["is_online"] == false);
        printTestResult("空字符串处理", passed);
    }
    
    // 测试3：特殊字符处理
    {
        User user;
        user.username = "user@example.com";
        user.password = "pass!@#$%^&*()";
        user.is_online = true;
        
        json j = user.toJson();
        
        bool passed = (j["username"] == "user@example.com" && 
                      j["password"] == "pass!@#$%^&*()" && 
                      j["is_online"] == true);
        printTestResult("特殊字符处理", passed);
    }
}

// 测试JSON转User对象
void testJsonToUser() {
    std::cout << "\n=== 测试 User::fromJson() ===" << std::endl;
    
    // 测试1：基本转换
    {
        json j = {
            {"username", "testuser"},
            {"password", "testpass"},
            {"is_online", true}
        };
        
        User user = User::fromJson(j);
        
        bool passed = (user.username == "testuser" && 
                      user.password == "testpass" && 
                      user.is_online == true);
        printTestResult("基本JSON解析", passed);
    }
    
    // 测试2：false值处理
    {
        json j = {
            {"username", "offlineuser"},
            {"password", "password123"},
            {"is_online", false}
        };
        
        User user = User::fromJson(j);
        
        bool passed = (user.username == "offlineuser" && 
                      user.password == "password123" && 
                      user.is_online == false);
        printTestResult("离线用户解析", passed);
    }
    
    // 测试3：缺少字段异常处理
    {
        json j = {
            {"username", "incompleteuser"},
            {"password", "password123"}
            // 缺少 is_online 字段
        };
        
        bool passed = false;
        try {
            User user = User::fromJson(j);
        } catch (const std::exception& e) {
            passed = true; // 期望抛出异常
        }
        printTestResult("缺少字段异常处理", passed);
    }
    
    // 测试4：错误类型异常处理
    {
        json j = {
            {"username", "typeuser"},
            {"password", "password123"},
            {"is_online", "not_a_boolean"} // 错误类型
        };
        
        bool passed = false;
        try {
            User user = User::fromJson(j);
        } catch (const std::exception& e) {
            passed = true; // 期望抛出异常
        }
        printTestResult("错误类型异常处理", passed);
    }
}

// 测试往返转换（序列化后反序列化）
void testRoundTrip() {
    std::cout << "\n=== 测试往返转换 ===" << std::endl;
    
    // 测试1：往返转换保持数据一致性
    {
        User originalUser;
        originalUser.username = "roundtripuser";
        originalUser.password = "complexpass!@#";
        originalUser.is_online = true;
        
        // 转换为JSON再转回User
        json j = originalUser.toJson();
        User reconstructedUser = User::fromJson(j);
        
        bool passed = (originalUser.username == reconstructedUser.username && 
                      originalUser.password == reconstructedUser.password && 
                      originalUser.is_online == reconstructedUser.is_online);
        printTestResult("往返转换数据一致性", passed);
    }
    
    // 测试2：多次往返转换
    {
        User user;
        user.username = "multiround";
        user.password = "testpass";
        user.is_online = false;
        
        // 进行多次往返转换
        for (int i = 0; i < 5; i++) {
            json j = user.toJson();
            user = User::fromJson(j);
        }
        
        bool passed = (user.username == "multiround" && 
                      user.password == "testpass" && 
                      user.is_online == false);
        printTestResult("多次往返转换", passed);
    }
}

// 测试边界情况
void testEdgeCases() {
    std::cout << "\n=== 测试边界情况 ===" << std::endl;
    
    // 测试1：超长字符串
    {
        User user;
        user.username = std::string(1000, 'a'); // 1000个'a'
        user.password = std::string(1000, 'b'); // 1000个'b'
        user.is_online = true;
        
        json j = user.toJson();
        User reconstructedUser = User::fromJson(j);
        
        bool passed = (user.username == reconstructedUser.username && 
                      user.password == reconstructedUser.password && 
                      user.is_online == reconstructedUser.is_online);
        printTestResult("超长字符串处理", passed);
    }
    
    // 测试2：Unicode字符
    {
        User user;
        user.username = "用户名测试";
        user.password = "密码测试🔐";
        user.is_online = true;
        
        json j = user.toJson();
        User reconstructedUser = User::fromJson(j);
        
        bool passed = (user.username == reconstructedUser.username && 
                      user.password == reconstructedUser.password && 
                      user.is_online == reconstructedUser.is_online);
        printTestResult("Unicode字符处理", passed);
    }
}

int main() {
    std::cout << "开始运行 User 类测试用例..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testUserToJson();
        testJsonToUser();
        testRoundTrip();
        testEdgeCases();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "所有测试已完成！" << std::endl;
        std::cout << "注意：请检查上面的测试结果，确保所有测试都通过。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
