#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <cstdio>
#include "../../src/utils/logger.hpp"

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

// 检查字符串是否包含子字符串
bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

// Logger 测试类
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试开始前重置日志级别
        Logger::setGlobalLevel(LogLevel::DEBUG);
    }

    void TearDown() override {
        // 测试结束后清理
    }
};

// 测试基本日志级别
TEST_F(LoggerTest, AllLogLevelsOutput) {
    // 设置为DEBUG级别，所有日志都应该显示
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    OutputCapture capture;
    LOG_DEBUG << "这是DEBUG消息";
    LOG_INFO << "这是INFO消息";
    LOG_WARN << "这是WARN消息";
    LOG_ERROR << "这是ERROR消息";
    LOG_FATAL << "这是FATAL消息";
    
    std::string output = capture.getCout();
    
    EXPECT_TRUE(contains(output, "DEBUG"));
    EXPECT_TRUE(contains(output, "INFO"));
    EXPECT_TRUE(contains(output, "WARN"));
    EXPECT_TRUE(contains(output, "ERROR"));
    EXPECT_TRUE(contains(output, "FATAL"));
}

TEST_F(LoggerTest, LogLevelFiltering) {
    // 测试日志级别过滤
    Logger::setGlobalLevel(LogLevel::WARN);
    
    OutputCapture capture;
    LOG_DEBUG << "这不应该显示";
    LOG_INFO << "这也不应该显示";
    LOG_WARN << "这应该显示";
    LOG_ERROR << "这也应该显示";
    
    std::string output = capture.getCout();
    
    EXPECT_FALSE(contains(output, "这不应该显示"));
    EXPECT_FALSE(contains(output, "这也不应该显示"));
    EXPECT_TRUE(contains(output, "这应该显示"));
    EXPECT_TRUE(contains(output, "这也应该显示"));
}

// 测试日志格式
TEST_F(LoggerTest, LogFormat) {
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    OutputCapture capture;
    LOG_INFO << "格式测试消息";
    
    std::string output = capture.getCout();
    
    // 检查是否包含时间戳、级别、线程ID、文件名、函数名等信息
    EXPECT_TRUE(contains(output, "[INFO]")); // 注意这里有空格
    EXPECT_TRUE(contains(output, ".cpp"));
    EXPECT_TRUE(contains(output, "格式测试消息"));
}

// 测试错误日志同时输出到stderr
TEST_F(LoggerTest, ErrorOutputToStderr) {
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    OutputCapture capture;
    LOG_ERROR << "这是错误消息";
    
    std::string cout_output = capture.getCout();
    std::string cerr_output = capture.getCerr();
    
    EXPECT_TRUE(contains(cout_output, "这是错误消息"));
    EXPECT_TRUE(contains(cerr_output, "这是错误消息"));
}

TEST_F(LoggerTest, FatalOutputToStderr) {
    OutputCapture capture;
    LOG_FATAL << "这是致命错误";
    
    std::string cout_output = capture.getCout();
    std::string cerr_output = capture.getCerr();
    
    EXPECT_TRUE(contains(cout_output, "这是致命错误"));
    EXPECT_TRUE(contains(cerr_output, "这是致命错误"));
}

TEST_F(LoggerTest, InfoOutputOnlyToStdout) {
    OutputCapture capture;
    LOG_INFO << "这是普通信息";
    
    std::string cout_output = capture.getCout();
    std::string cerr_output = capture.getCerr();
    
    EXPECT_TRUE(contains(cout_output, "这是普通信息"));
    EXPECT_TRUE(cerr_output.empty());
}

// 测试多线程安全性
TEST_F(LoggerTest, ThreadSafety) {
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
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
    
    EXPECT_EQ(message_count, thread_count * messages_per_thread);
}

// 测试流式操作
TEST_F(LoggerTest, StreamOperationsWithDifferentTypes) {
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    OutputCapture capture;
    
    int number = 42;
    double pi = 3.14159;
    std::string text = "测试文本";
    
    LOG_INFO << "数字: " << number << ", 浮点数: " << pi << ", 文本: " << text;
    
    std::string output = capture.getCout();
    
    EXPECT_TRUE(contains(output, "数字: 42"));
    EXPECT_TRUE(contains(output, "浮点数: 3.14159"));
    EXPECT_TRUE(contains(output, "文本: 测试文本"));
}

TEST_F(LoggerTest, ContinuousStreamOperations) {
    OutputCapture capture;
    LOG_DEBUG << "这是一个" << "连续的" << "消息" << 123;
    
    std::string output = capture.getCout();
    EXPECT_TRUE(contains(output, "这是一个连续的消息123"));
}

// 测试日志级别设置
TEST_F(LoggerTest, LogLevelSetting) {
    // 测试各种级别设置
    LogLevel levels[] = {LogLevel::DEBUG, LogLevel::INFO, LogLevel::WARN, LogLevel::ERROR, LogLevel::FATAL};
    
    for (int i = 0; i < 5; ++i) {
        Logger::setGlobalLevel(levels[i]);
        LogLevel current = Logger::getGlobalLevel();
        EXPECT_EQ(current, levels[i]);
    }
}

// 测试特殊字符和Unicode
TEST_F(LoggerTest, SpecialCharacters) {
    Logger::setGlobalLevel(LogLevel::DEBUG);
    
    OutputCapture capture;
    LOG_INFO << "特殊字符: !@#$%^&*()_+-=[]{}|;':\",./<>?";
    
    std::string output = capture.getCout();
    EXPECT_TRUE(contains(output, "特殊字符: !@#$%^&*()_+-=[]{}|;':\",./<>?"));
}

TEST_F(LoggerTest, UnicodeCharacters) {
    OutputCapture capture;
    LOG_INFO << "Unicode: 中文测试 🚀 📝 ✅";
    
    std::string output = capture.getCout();
    EXPECT_TRUE(contains(output, "Unicode: 中文测试 🚀 📝 ✅"));
}

TEST_F(LoggerTest, ControlCharacters) {
    OutputCapture capture;
    LOG_INFO << "换行符测试\n第二行\t制表符";
    
    std::string output = capture.getCout();
    EXPECT_TRUE(contains(output, "换行符测试"));
    EXPECT_TRUE(contains(output, "第二行"));
    EXPECT_TRUE(contains(output, "制表符"));
}

// 性能测试
TEST_F(LoggerTest, Performance) {
    Logger::setGlobalLevel(LogLevel::INFO);
    
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
    
    EXPECT_EQ(message_count, log_count);
    EXPECT_LT(duration.count(), 5000); // 5秒内完成
}

TEST_F(LoggerTest, FilterPerformance) {
    // 测试过滤性能
    Logger::setGlobalLevel(LogLevel::ERROR);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    const int log_count = 10000;
    for (int i = 0; i < log_count; ++i) {
        LOG_DEBUG << "这些消息应该被过滤 " << i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000); // 被过滤的日志应该很快
}

// Google Test main 函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
