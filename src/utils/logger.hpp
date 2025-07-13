#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <thread>
#include <iomanip>
#include <chrono>
#include <mutex>

namespace utils
{

    // ANSI escape codes for colors
    struct Color
    {
        static constexpr const char *RESET = "\033[0m";
        static constexpr const char *RED = "\033[31m";
        static constexpr const char *GREEN = "\033[32m";
        static constexpr const char *YELLOW = "\033[33m";
        static constexpr const char *BLUE = "\033[34m";
        static constexpr const char *MAGENTA = "\033[35m";
        static constexpr const char *CYAN = "\033[36m";
        static constexpr const char *BOLD = "\033[1m";
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
            LogStream(LogLevel level, const char *file, const char *function, int line)
                : level_(level), file_(getFileName(file)), function_(function), line_(line)
            {
                if (level_ >= Logger::getGlobalLevel())
                {
                    // 添加时间戳
                    auto now = std::chrono::system_clock::now();
                    auto now_time_t = std::chrono::system_clock::to_time_t(now);
                    auto now_tm = *std::localtime(&now_time_t);
                    auto now_ms = std::chrono::duration_cast<std::chrono::microseconds>(
                                      now.time_since_epoch()) %
                                  1000000;

                    char buffer[80];
                    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &now_tm);

                    stream_ << Color::CYAN << "[" << buffer << "."
                            << std::setfill('0') << std::setw(6) << now_ms.count() << "] "
                            << getLevelColor(level_) << Color::BOLD << "[" << getLevelStr(level_) << "] "
                            << Color::MAGENTA << "[" << std::this_thread::get_id() << "] "
                            << Color::BLUE << "[" << file_ << ":" << line_ << "] "
                            << Color::CYAN << "[" << function_ << "] "
                            << getLevelColor(level_);
                }
            }

            ~LogStream()
            {
                if (level_ >= Logger::getGlobalLevel())
                {
                    stream_ << Color::RESET << std::endl;
                    
                    // 使用锁确保线程安全的输出
                    static std::mutex output_mutex;
                    std::lock_guard<std::mutex> lock(output_mutex);
                    
                    std::cout << stream_.str();
                    std::cout.flush();

                    // 如果是错误或致命错误，也输出到 stderr
                    if (level_ >= LogLevel::ERROR)
                    {
                        std::cerr << stream_.str();
                        std::cerr.flush();
                    }
                }
            }

            template <typename T>
            LogStream &operator<<(const T &val)
            {
                if (level_ >= Logger::getGlobalLevel())
                {
                    stream_ << val;
                }
                return *this;
            }

        private:
            std::ostringstream stream_;
            LogLevel level_;
            const char *file_;
            const char *function_;
            int line_;

            static const char *getFileName(const char *filePath)
            {
                const char *fileName = filePath;
                for (const char *p = filePath; *p; ++p)
                {
                    if (*p == '/' || *p == '\\')
                    {
                        fileName = p + 1;
                    }
                }
                return fileName;
            }

            static const char *getLevelStr(LogLevel level)
            {
                switch (level)
                {
                case LogLevel::DEBUG:
                    return "DEBUG";
                case LogLevel::INFO:
                    return "INFO ";
                case LogLevel::WARN:
                    return "WARN ";
                case LogLevel::ERROR:
                    return "ERROR";
                case LogLevel::FATAL:
                    return "FATAL";
                default:
                    return "UNKNOWN";
                }
            }

            static const char *getLevelColor(LogLevel level)
            {
                switch (level)
                {
                case LogLevel::DEBUG:
                    return Color::RESET;
                case LogLevel::INFO:
                    return Color::GREEN;
                case LogLevel::WARN:
                    return Color::YELLOW;
                case LogLevel::ERROR:
                    return Color::RED;
                case LogLevel::FATAL:
                    return Color::RED;
                default:
                    return Color::RESET;
                }
            }
        };

        static void setGlobalLevel(LogLevel level)
        {
            globalLevel_ = level;
        }

        static LogLevel getGlobalLevel()
        {
            return globalLevel_;
        }

        // 添加一个获取锁的方法以支持批量日志输出
        static std::mutex& getOutputMutex()
        {
            static std::mutex output_mutex;
            return output_mutex;
        }

        static LogStream Debug(const char *file, const char *function, int line)
        {
            return LogStream(LogLevel::DEBUG, file, function, line);
        }

        static LogStream Info(const char *file, const char *function, int line)
        {
            return LogStream(LogLevel::INFO, file, function, line);
        }

        static LogStream Warn(const char *file, const char *function, int line)
        {
            return LogStream(LogLevel::WARN, file, function, line);
        }

        static LogStream Error(const char *file, const char *function, int line)
        {
            return LogStream(LogLevel::ERROR, file, function, line);
        }

        static LogStream Fatal(const char *file, const char *function, int line)
        {
            return LogStream(LogLevel::FATAL, file, function, line);
        }

    private:
        static LogLevel globalLevel_;
    };

    // 在 .cpp 文件中初始化
    inline LogLevel Logger::globalLevel_ = LogLevel::INFO;

} // namespace utils

// 定义便捷宏
#define LOG_DEBUG utils::Logger::Debug(__FILE__, __FUNCTION__, __LINE__)
#define LOG_INFO utils::Logger::Info(__FILE__, __FUNCTION__, __LINE__)
#define LOG_WARN utils::Logger::Warn(__FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR utils::Logger::Error(__FILE__, __FUNCTION__, __LINE__)
#define LOG_FATAL utils::Logger::Fatal(__FILE__, __FUNCTION__, __LINE__)
