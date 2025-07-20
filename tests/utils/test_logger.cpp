#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <cstdio> // for std::remove
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
        // 确保文件日志被关闭
        Logger::closeFileLogger();
    }

    void TearDown() override {
        // 测试结束后清理
        Logger::closeFileLogger();
        // 重置日志级别
        Logger::setGlobalLevel(LogLevel::DEBUG);
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
    EXPECT_TRUE(contains(output, "[INFO ]")); // 注意这里有空格
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

// 测试文件日志功能
TEST_F(LoggerTest, FileLogging) {
    const std::string test_log_file = "/tmp/test_logger.log";
    
    // 清理可能存在的测试文件
    std::remove(test_log_file.c_str());
    
    // 初始化文件日志
    EXPECT_TRUE(Logger::initFileLogger(test_log_file));
    EXPECT_TRUE(Logger::isFileLoggingEnabled());
    
    // 写入一些日志
    LOG_INFO << "这是文件日志测试消息";
    LOG_ERROR << "这是错误日志消息";
    LOG_DEBUG << "这是调试消息";
    
    // 关闭文件日志
    Logger::closeFileLogger();
    EXPECT_FALSE(Logger::isFileLoggingEnabled());
    
    // 检查文件内容
    std::ifstream log_file(test_log_file);
    ASSERT_TRUE(log_file.is_open());
    
    std::string file_content;
    std::string line;
    while (std::getline(log_file, line)) {
        file_content += line + "\n";
    }
    log_file.close();
    
    // 验证日志内容（文件中应该没有ANSI颜色代码）
    EXPECT_TRUE(contains(file_content, "这是文件日志测试消息"));
    EXPECT_TRUE(contains(file_content, "这是错误日志消息"));
    EXPECT_TRUE(contains(file_content, "这是调试消息"));
    EXPECT_FALSE(contains(file_content, "\033[")); // 不应该包含ANSI转义序列
    
    // 清理测试文件
    std::remove(test_log_file.c_str());
}

TEST_F(LoggerTest, FileLoggingWithInvalidPath) {
    const std::string invalid_path = "/nonexistent/directory/test.log";
    
    // 尝试打开无效路径的文件
    EXPECT_FALSE(Logger::initFileLogger(invalid_path));
    EXPECT_FALSE(Logger::isFileLoggingEnabled());
}

TEST_F(LoggerTest, FileLoggingAppendMode) {
    const std::string test_log_file = "/tmp/test_append.log";
    
    // 清理可能存在的测试文件
    std::remove(test_log_file.c_str());
    
    // 第一次写入
    EXPECT_TRUE(Logger::initFileLogger(test_log_file));
    LOG_INFO << "第一条消息";
    Logger::closeFileLogger();
    
    // 第二次写入（应该追加，不覆盖）
    EXPECT_TRUE(Logger::initFileLogger(test_log_file));
    LOG_INFO << "第二条消息";
    Logger::closeFileLogger();
    
    // 检查文件内容
    std::ifstream log_file(test_log_file);
    ASSERT_TRUE(log_file.is_open());
    
    std::string file_content;
    std::string line;
    while (std::getline(log_file, line)) {
        file_content += line + "\n";
    }
    log_file.close();
    
    // 验证两条消息都存在
    EXPECT_TRUE(contains(file_content, "第一条消息"));
    EXPECT_TRUE(contains(file_content, "第二条消息"));
    
    // 清理测试文件
    std::remove(test_log_file.c_str());
}

TEST_F(LoggerTest, FileLoggingThreadSafety) {
    const std::string test_log_file = "/tmp/test_thread_safe.log";
    
    // 清理可能存在的测试文件
    std::remove(test_log_file.c_str());
    
    // 初始化文件日志
    EXPECT_TRUE(Logger::initFileLogger(test_log_file));
    
    const int thread_count = 5;
    const int messages_per_thread = 20;
    std::vector<std::thread> threads;
    
    // 启动多个线程同时写文件日志
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([i, messages_per_thread]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                LOG_INFO << "线程" << i << "文件消息" << j;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    Logger::closeFileLogger();
    
    // 检查文件内容
    std::ifstream log_file(test_log_file);
    ASSERT_TRUE(log_file.is_open());
    
    std::string file_content;
    std::string line;
    while (std::getline(log_file, line)) {
        file_content += line + "\n";
    }
    log_file.close();
    
    // 计算消息数量
    int message_count = 0;
    size_t pos = 0;
    while ((pos = file_content.find("文件消息", pos)) != std::string::npos) {
        message_count++;
        pos++;
    }
    
    EXPECT_EQ(message_count, thread_count * messages_per_thread);
    
    // 清理测试文件
    std::remove(test_log_file.c_str());
}

TEST_F(LoggerTest, AnsiCodeStripping) {
    const std::string test_log_file = "/tmp/test_ansi.log";
    
    // 清理可能存在的测试文件
    std::remove(test_log_file.c_str());
    
    // 初始化文件日志
    EXPECT_TRUE(Logger::initFileLogger(test_log_file));
    
    // 写入包含各种级别的日志（会有不同颜色）
    LOG_DEBUG << "调试消息";
    LOG_INFO << "信息消息";
    LOG_WARN << "警告消息";
    LOG_ERROR << "错误消息";
    LOG_FATAL << "致命错误消息";
    
    Logger::closeFileLogger();
    
    // 检查文件内容
    std::ifstream log_file(test_log_file);
    ASSERT_TRUE(log_file.is_open());
    
    std::string file_content;
    std::string line;
    while (std::getline(log_file, line)) {
        file_content += line + "\n";
    }
    log_file.close();
    
    // 验证消息存在但没有ANSI代码
    EXPECT_TRUE(contains(file_content, "调试消息"));
    EXPECT_TRUE(contains(file_content, "信息消息"));
    EXPECT_TRUE(contains(file_content, "警告消息"));
    EXPECT_TRUE(contains(file_content, "错误消息"));
    EXPECT_TRUE(contains(file_content, "致命错误消息"));
    
    // 确保没有ANSI转义序列
    EXPECT_FALSE(contains(file_content, "\033["));
    EXPECT_FALSE(contains(file_content, "\033[0m"));
    EXPECT_FALSE(contains(file_content, "\033[31m"));
    EXPECT_FALSE(contains(file_content, "\033[32m"));
    
    // 清理测试文件
    std::remove(test_log_file.c_str());
}

TEST_F(LoggerTest, FileLoggingWithLogLevelFilter) {
    const std::string test_log_file = "/tmp/test_level_filter.log";
    
    // 清理可能存在的测试文件
    std::remove(test_log_file.c_str());
    
    // 设置日志级别为WARN
    Logger::setGlobalLevel(LogLevel::WARN);
    
    // 初始化文件日志
    EXPECT_TRUE(Logger::initFileLogger(test_log_file));
    
    // 写入不同级别的日志
    LOG_DEBUG << "这条调试消息不应该出现";
    LOG_INFO << "这条信息消息不应该出现";
    LOG_WARN << "这条警告消息应该出现";
    LOG_ERROR << "这条错误消息应该出现";
    
    Logger::closeFileLogger();
    
    // 检查文件内容
    std::ifstream log_file(test_log_file);
    ASSERT_TRUE(log_file.is_open());
    
    std::string file_content;
    std::string line;
    while (std::getline(log_file, line)) {
        file_content += line + "\n";
    }
    log_file.close();
    
    // 验证只有WARN及以上级别的消息被记录
    EXPECT_FALSE(contains(file_content, "这条调试消息不应该出现"));
    EXPECT_FALSE(contains(file_content, "这条信息消息不应该出现"));
    EXPECT_TRUE(contains(file_content, "这条警告消息应该出现"));
    EXPECT_TRUE(contains(file_content, "这条错误消息应该出现"));
    
    // 清理测试文件
    std::remove(test_log_file.c_str());
}

// Google Test main 函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
