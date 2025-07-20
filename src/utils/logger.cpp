#include "logger.hpp"
#include <regex>

namespace utils
{
    // 静态成员变量定义
    LogLevel Logger::globalLevel_ = LogLevel::INFO;
    std::mutex Logger::output_mutex_;
    std::ofstream Logger::file_stream_;
    bool Logger::file_logging_enabled_ = false;

    // LogStream 构造函数
    Logger::LogStream::LogStream(LogLevel level, const char* file, const char* function, int line)
        : level_(level), file_(getFileName(file)), function_(function), line_(line), should_log_(level >= Logger::getGlobalLevel())
    {
        if (should_log_)
        {
            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            auto now_tm = *std::localtime(&now_time_t);
            auto now_ms = std::chrono::duration_cast<std::chrono::microseconds>(
                              now.time_since_epoch()) % 1000000;

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

    // LogStream 析构函数
    Logger::LogStream::~LogStream()
    {
        if (should_log_)
        {
            stream_ << Color::RESET << std::endl;
            std::string full_message = stream_.str();

            // 使用集中的锁来确保线程安全的输出
            std::lock_guard<std::mutex> lock(Logger::output_mutex_);

            // 控制台输出
            writeToConsole(full_message, level_);

            // 文件输出
            if (file_logging_enabled_ && file_stream_.is_open())
            {
                writeToFile(full_message);
            }
        }
    }

    // LogStream 移动构造函数
    Logger::LogStream::LogStream(LogStream&& other) noexcept
        : stream_(std::move(other.stream_)), level_(other.level_), 
          file_(std::move(other.file_)), function_(std::move(other.function_)), 
          line_(other.line_), should_log_(other.should_log_)
    {
        other.should_log_ = false; // 防止原对象析构时重复输出
    }

    // LogStream 移动赋值运算符
    Logger::LogStream& Logger::LogStream::operator=(LogStream&& other) noexcept
    {
        if (this != &other)
        {
            stream_ = std::move(other.stream_);
            level_ = other.level_;
            file_ = std::move(other.file_);
            function_ = std::move(other.function_);
            line_ = other.line_;
            should_log_ = other.should_log_;
            other.should_log_ = false;
        }
        return *this;
    }

    // LogStream 私有辅助方法
    const char* Logger::LogStream::getFileName(const char* filePath)
    {
        const char* fileName = filePath;
        for (const char* p = filePath; *p; ++p)
        {
            if (*p == '/' || *p == '\\')
            {
                fileName = p + 1;
            }
        }
        return fileName;
    }

    std::string Logger::LogStream::stripAnsiCodes(const std::string& input)
    {
        return Logger::stripAnsiCodesInternal(input);
    }

    const char* Logger::LogStream::getLevelStr(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARN:    return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
        }
    }

    const char* Logger::LogStream::getLevelColor(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:   return Color::RESET;
        case LogLevel::INFO:    return Color::GREEN;
        case LogLevel::WARN:    return Color::YELLOW;
        case LogLevel::ERROR:   return Color::RED;
        case LogLevel::FATAL:   return Color::RED;
        default:                return Color::RESET;
        }
    }

    // Logger 公共方法
    void Logger::setGlobalLevel(LogLevel level)
    {
        globalLevel_ = level;
    }

    LogLevel Logger::getGlobalLevel()
    {
        return globalLevel_;
    }

    bool Logger::initFileLogger(const std::string& filename)
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        
        if (file_stream_.is_open())
        {
            file_stream_.close();
        }
        
        // 以追加模式打开文件
        file_stream_.open(filename, std::ios::out | std::ios::app);
        file_logging_enabled_ = file_stream_.is_open();
        
        if (!file_logging_enabled_)
        {
            std::cerr << "错误：打开日志文件失败: " << filename << std::endl;
        }
        
        return file_logging_enabled_;
    }

    void Logger::closeFileLogger()
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        if (file_stream_.is_open())
        {
            file_stream_.close();
        }
        file_logging_enabled_ = false;
    }

    bool Logger::isFileLoggingEnabled()
    {
        return file_logging_enabled_;
    }

    // Logger 静态工厂方法
    Logger::LogStream Logger::Debug(const char* file, const char* function, int line)
    {
        return LogStream(LogLevel::DEBUG, file, function, line);
    }

    Logger::LogStream Logger::Info(const char* file, const char* function, int line)
    {
        return LogStream(LogLevel::INFO, file, function, line);
    }

    Logger::LogStream Logger::Warn(const char* file, const char* function, int line)
    {
        return LogStream(LogLevel::WARN, file, function, line);
    }

    Logger::LogStream Logger::Error(const char* file, const char* function, int line)
    {
        return LogStream(LogLevel::ERROR, file, function, line);
    }

    Logger::LogStream Logger::Fatal(const char* file, const char* function, int line)
    {
        return LogStream(LogLevel::FATAL, file, function, line);
    }

    // Logger 私有辅助方法
    void Logger::writeToConsole(const std::string& message, LogLevel level)
    {
        std::cout << message;
        std::cout.flush();

        // 对于高级别日志，同时输出到 stderr
        if (level >= LogLevel::ERROR)
        {
            std::cerr << message;
            std::cerr.flush();
        }
    }

    void Logger::writeToFile(const std::string& message)
    {
        // 写入文件前剥离颜色代码
        std::string clean_message = stripAnsiCodesInternal(message);
        file_stream_ << clean_message;
        file_stream_.flush();
    }

    // 内部辅助函数，用于剥离ANSI代码
    std::string Logger::stripAnsiCodesInternal(const std::string& input)
    {
        std::string result;
        result.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); ++i)
        {
            if (input[i] == '\033' && i + 1 < input.size() && input[i + 1] == '[')
            {
                // 跳过ANSI转义序列
                i += 2; // 跳过 '\033['
                while (i < input.size() && input[i] != 'm')
                {
                    ++i;
                }
                // i现在指向'm'或者超出范围，循环会递增i
            }
            else
            {
                result += input[i];
            }
        }
        return result;
    }

} // namespace utils
