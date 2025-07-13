#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <cstdio>
#include "../src/utils/logger.hpp"

using namespace utils;

// 重定向stdout和stderr用于测试
class OutputCapture {
private:
    std::streambuf* old_cout;
    std::streambuf* old_cerr;
    std::ostringstream captured_cout;
    std::ostringstream captured_cerr;

public:
    OutputCapture() {
        old_cout = std::cout.rdbuf();
        old_cerr = std::cerr.rdbuf();
        std::cout.rdbuf(captured_cout.rdbuf());
        std::cerr.rdbuf(captured_cerr.rdbuf());
    }

    ~OutputCapture() {
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
    }

    std::string getCout() const { return captured_cout.str(); }
    std::string getCerr() const { return captured_cerr.str(); }
    
    void clear() {
        captured_cout.str("");
        captured_cout.clear();
        captured_cerr.str("");
        captured_cerr.clear();
    }
};

// 测试辅助函数
void printTestResult(const std::string& testName, bool passed) {
    // 直接写入原始的stdout，避免被OutputCapture捕获
    fprintf(stdout, "[%s] %s\n", passed ? "PASS" : "FAIL", testName.c_str());
    fflush(stdout);
}

// 检查字符串是否包含子字符串
bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

// 测试基本日志级别
void testLogLevels() {
    printf("\n=== 测试日志级别 ===\n");
    
    // 设置为DEBUG级别，所有日志都应该显示
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    {
        OutputCapture capture;
        LOG_DEBUG << "这是DEBUG消息";
        LOG_INFO << "这是INFO消息";
        LOG_WARN << "这是WARN消息";
        LOG_ERROR << "这是ERROR消息";
        LOG_FATAL << "这是FATAL消息";
        
        std::string output = capture.getCout();
        
        bool passed = contains(output, "DEBUG") && 
                     contains(output, "INFO") && 
                     contains(output, "WARN") && 
                     contains(output, "ERROR") && 
                     contains(output, "FATAL");
        printTestResult("所有级别日志输出", passed);
    }
    
    // 测试日志级别过滤
    Logger::setGlobalLevel(LogLevel::WARN);
    
    {
        OutputCapture capture;
        LOG_DEBUG << "这不应该显示";
        LOG_INFO << "这也不应该显示";
        LOG_WARN << "这应该显示";
        LOG_ERROR << "这也应该显示";
        
        std::string output = capture.getCout();
        
        bool passed = !contains(output, "这不应该显示") && 
                     !contains(output, "这也不应该显示") && 
                     contains(output, "这应该显示") && 
                     contains(output, "这也应该显示");
        printTestResult("日志级别过滤", passed);
    }
}

// 测试日志格式
void testLogFormat() {
    printf("\n=== 测试日志格式 ===\n");
    
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    {
        OutputCapture capture;
        LOG_INFO << "格式测试消息";
        
        std::string output = capture.getCout();
        
        // 检查是否包含时间戳、级别、线程ID、文件名、函数名等信息
        bool passed = contains(output, "[INFO ]") && // 注意这里有空格
                     contains(output, ".cpp") && 
                     contains(output, "格式测试消息");
        printTestResult("日志格式检查", passed);
    }
}

// 测试错误日志同时输出到stderr
void testErrorOutput() {
    printf("\n=== 测试错误输出 ===\n");
    
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    {
        OutputCapture capture;
        LOG_ERROR << "这是错误消息";
        
        std::string cout_output = capture.getCout();
        std::string cerr_output = capture.getCerr();
        
        bool passed = contains(cout_output, "这是错误消息") && 
                     contains(cerr_output, "这是错误消息");
        printTestResult("错误消息同时输出到stdout和stderr", passed);
    }
    
    {
        OutputCapture capture;
        LOG_FATAL << "这是致命错误";
        
        std::string cout_output = capture.getCout();
        std::string cerr_output = capture.getCerr();
        
        bool passed = contains(cout_output, "这是致命错误") && 
                     contains(cerr_output, "这是致命错误");
        printTestResult("致命错误同时输出到stdout和stderr", passed);
    }
    
    {
        OutputCapture capture;
        LOG_INFO << "这是普通信息";
        
        std::string cout_output = capture.getCout();
        std::string cerr_output = capture.getCerr();
        
        bool passed = contains(cout_output, "这是普通信息") && 
                     cerr_output.empty();
        printTestResult("普通信息只输出到stdout", passed);
    }
}

// 测试多线程安全性
void testThreadSafety() {
    printf("\n=== 测试多线程安全性 ===\n");
    
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    {
        OutputCapture capture;
        
        const int thread_count = 10;
        const int messages_per_thread = 10;
        std::vector<std::thread> threads;
        
        // 启动多个线程同时写日志
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([i, messages_per_thread]() {
                for (int j = 0; j < messages_per_thread; ++j) {
                    LOG_INFO << "线程" << i << "消息" << j;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        std::string output = capture.getCout();
        
        // 检查是否有预期数量的日志消息
        int message_count = 0;
        size_t pos = 0;
        while ((pos = output.find("线程", pos)) != std::string::npos) {
            message_count++;
            pos++;
        }
        
        bool passed = message_count == thread_count * messages_per_thread;
        printTestResult("多线程日志输出完整性", passed);
    }
}

// 测试流式操作
void testStreamOperations() {
    printf("\n=== 测试流式操作 ===\n");
    
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    {
        OutputCapture capture;
        
        int number = 42;
        double pi = 3.14159;
        std::string text = "测试文本";
        
        LOG_INFO << "数字: " << number << ", 浮点数: " << pi << ", 文本: " << text;
        
        std::string output = capture.getCout();
        
        bool passed = contains(output, "数字: 42") && 
                     contains(output, "浮点数: 3.14159") && 
                     contains(output, "文本: 测试文本");
        printTestResult("流式操作支持不同数据类型", passed);
    }
    
    {
        OutputCapture capture;
        LOG_DEBUG << "这是一个" << "连续的" << "消息" << 123;
        
        std::string output = capture.getCout();
        bool passed = contains(output, "这是一个连续的消息123");
        printTestResult("连续流式操作", passed);
    }
}

// 测试日志级别设置
void testLogLevelSetting() {
    printf("\n=== 测试日志级别设置 ===\n");
    
    // 测试各种级别设置
    LogLevel levels[] = {LogLevel::DEBUG, LogLevel::INFO, LogLevel::WARN, LogLevel::ERROR, LogLevel::FATAL};
    std::string level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    
    bool all_passed = true;
    
    for (int i = 0; i < 5; ++i) {
        Logger::setGlobalLevel(levels[i]);
        LogLevel current = Logger::getGlobalLevel();
        bool passed = current == levels[i];
        all_passed &= passed;
        printTestResult("设置日志级别为 " + level_names[i], passed);
    }
    
    printTestResult("所有日志级别设置", all_passed);
}

// 测试特殊字符和Unicode
void testSpecialCharacters() {
    printf("\n=== 测试特殊字符 ===\n");
    
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    {
        OutputCapture capture;
        LOG_INFO << "特殊字符: !@#$%^&*()_+-=[]{}|;':\",./<>?";
        
        std::string output = capture.getCout();
        bool passed = contains(output, "特殊字符: !@#$%^&*()_+-=[]{}|;':\",./<>?");
        printTestResult("特殊字符处理", passed);
    }
    
    {
        OutputCapture capture;
        LOG_INFO << "Unicode: 中文测试 🚀 📝 ✅";
        
        std::string output = capture.getCout();
        bool passed = contains(output, "Unicode: 中文测试 🚀 📝 ✅");
        printTestResult("Unicode字符处理", passed);
    }
    
    {
        OutputCapture capture;
        LOG_INFO << "换行符测试\n第二行\t制表符";
        
        std::string output = capture.getCout();
        bool passed = contains(output, "换行符测试") && contains(output, "第二行") && contains(output, "制表符");
        printTestResult("控制字符处理", passed);
    }
}

// 性能测试
void testPerformance() {
    printf("\n=== 性能测试 ===\n");
    
    Logger::setGlobalLevel(LogLevel::INFO);
    
    {
        OutputCapture capture;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        const int log_count = 1000;
        for (int i = 0; i < log_count; ++i) {
            LOG_INFO << "性能测试消息 " << i;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::string output = capture.getCout();
        int message_count = 0;
        size_t pos = 0;
        while ((pos = output.find("性能测试消息", pos)) != std::string::npos) {
            message_count++;
            pos++;
        }
        
        bool passed = message_count == log_count && duration.count() < 5000; // 5秒内完成
        printTestResult("性能测试 (" + std::to_string(log_count) + "条消息用时" + 
                       std::to_string(duration.count()) + "ms)", passed);
    }
    
    // 测试过滤性能
    Logger::setGlobalLevel(LogLevel::ERROR);
    
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        const int log_count = 10000;
        for (int i = 0; i < log_count; ++i) {
            LOG_DEBUG << "这些消息应该被过滤 " << i;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        bool passed = duration.count() < 1000; // 被过滤的日志应该很快
        printTestResult("日志过滤性能测试 (" + std::to_string(log_count) + "条被过滤消息用时" + 
                       std::to_string(duration.count()) + "ms)", passed);
    }
}

int main() {
    printf("开始运行 Logger 类测试用例...\n");
    printf("==========================================\n");
    
    try {
        testLogLevels();
        testLogFormat();
        testErrorOutput();
        testThreadSafety();
        testStreamOperations();
        testLogLevelSetting();
        testSpecialCharacters();
        testPerformance();
        
        printf("\n==========================================\n");
        printf("所有测试已完成！\n");
        printf("注意：请检查上面的测试结果，确保所有测试都通过。\n");
        
    } catch (const std::exception& e) {
        fprintf(stderr, "测试过程中发生异常: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
