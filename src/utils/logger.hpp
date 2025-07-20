#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <thread>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <fstream>

namespace utils
{
    // ANSI 颜色转义码
    struct Color
    {
        static constexpr const char* RESET = "\033[0m";
        static constexpr const char* RED = "\033[31m";
        static constexpr const char* GREEN = "\033[32m";
        static constexpr const char* YELLOW = "\033[33m";
        static constexpr const char* BLUE = "\033[34m";
        static constexpr const char* MAGENTA = "\033[35m";
        static constexpr const char* CYAN = "\033[36m";
        static constexpr const char* BOLD = "\033[1m";
    };

    enum class LogLevel
    {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3,
        FATAL = 4
    };

    class Logger
    {
    public:
        class LogStream
        {
        public:
            LogStream(LogLevel level, const char* file, const char* function, int line);
            ~LogStream();

            template <typename T>
            LogStream& operator<<(const T& val)
            {
                if (level_ >= Logger::getGlobalLevel())
                {
                    stream_ << val;
                }
                return *this;
            }

            // 禁用拷贝构造和赋值
            LogStream(const LogStream&) = delete;
            LogStream& operator=(const LogStream&) = delete;

            // 允许移动构造和赋值
            LogStream(LogStream&& other) noexcept;
            LogStream& operator=(LogStream&& other) noexcept;

        private:
            std::ostringstream stream_;
            LogLevel level_;
            std::string file_;
            std::string function_;
            int line_;
            bool should_log_;

            static const char* getFileName(const char* filePath);
            static std::string stripAnsiCodes(const std::string& input);
            static const char* getLevelStr(LogLevel level);
            static const char* getLevelColor(LogLevel level);
        };

        // 日志级别控制
        static void setGlobalLevel(LogLevel level);
        static LogLevel getGlobalLevel();

        // 文件日志控制
        static bool initFileLogger(const std::string& filename);
        static void closeFileLogger();
        static bool isFileLoggingEnabled();

        // 日志流创建方法
        static LogStream Debug(const char* file, const char* function, int line);
        static LogStream Info(const char* file, const char* function, int line);
        static LogStream Warn(const char* file, const char* function, int line);
        static LogStream Error(const char* file, const char* function, int line);
        static LogStream Fatal(const char* file, const char* function, int line);

    private:
        static LogLevel globalLevel_;
        static std::mutex output_mutex_;
        static std::ofstream file_stream_;
        static bool file_logging_enabled_;
        
        // 内部辅助方法
        static void writeToConsole(const std::string& message, LogLevel level);
        static void writeToFile(const std::string& message);
        static std::string stripAnsiCodesInternal(const std::string& input);
    };

} // namespace utils

// 便捷宏定义
#define LOG_DEBUG utils::Logger::Debug(__FILE__, __FUNCTION__, __LINE__)
#define LOG_INFO utils::Logger::Info(__FILE__, __FUNCTION__, __LINE__)
#define LOG_WARN utils::Logger::Warn(__FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR utils::Logger::Error(__FILE__, __FUNCTION__, __LINE__)
#define LOG_FATAL utils::Logger::Fatal(__FILE__, __FUNCTION__, __LINE__)