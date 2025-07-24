#include "http/http_server.hpp"
#include "utils/logger.hpp"
#include "websocket/websocket_server.hpp"
#include "service/auth_service.hpp"
#include "service/room_service.hpp"
#include "service/message_service.hpp"
#include "service/user_service.hpp"
#include "service/server_service.hpp"
#include "middleware/auth_middleware.hpp"
#include "db/database_manager.hpp"
#include <iostream>
#include <signal.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <memory>
#include <getopt.h>
#include <fstream>
#include <filesystem>
#include <locale>


std::atomic<bool> running(true);
std::unique_ptr<WebSocketServer> ws_server;

// 配置选项结构
struct ServerConfig {
    int http_port = 8080;
    int ws_port = 8081;
    std::string db_path = "./chat.db";
    std::string static_dir = "./static";
    std::string log_file = ""; // 将在运行时根据日期生成
    std::string log_dir = "./logs"; // 日志目录
    bool show_help = false;
    bool show_version = false;
};

void showHelp(const char* program_name) {
    std::cout << "SwiftChat Server v1.0.0\n\n";
    std::cout << "用法: " << program_name << " [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  --http-port PORT     HTTP 服务器端口 (默认: 8080)\n";
    std::cout << "  --ws-port PORT       WebSocket 服务器端口 (默认: 8081)\n";
    std::cout << "  --db-path PATH       数据库文件路径 (默认: ./chat.db)\n";
    std::cout << "  --static-dir DIR     静态文件目录 (默认: ./static)\n";
    std::cout << "  --log-dir DIR        日志文件目录 (默认: ./logs)\n";
    std::cout << "  --help               显示帮助信息\n";
    std::cout << "  --version            显示版本信息\n\n";
    std::cout << "注意: 日志文件将按日期命名 (如: swiftchat_2025-07-24.log)\n\n";
    std::cout << "示例:\n";
    std::cout << "  " << program_name << " --http-port 9000 --ws-port 9001\n";
    std::cout << "  " << program_name << " --db-path /var/lib/swiftchat/chat.db\n";
}

void showVersion() {
    std::cout << "SwiftChat Server v1.0.0\n";
    std::cout << "基于 C++17 构建的高性能实时聊天服务器\n";
}

ServerConfig parseCommandLine(int argc, char* argv[]) {
    ServerConfig config;
    
    static struct option long_options[] = {
        {"http-port", required_argument, 0, 'h'},
        {"ws-port", required_argument, 0, 'w'},
        {"db-path", required_argument, 0, 'd'},
        {"static-dir", required_argument, 0, 's'},
        {"log-dir", required_argument, 0, 'l'},
        {"help", no_argument, 0, '?'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "h:w:d:s:l:?v", long_options, nullptr)) != -1) {
        switch (c) {
            case 'h':
                config.http_port = std::atoi(optarg);
                break;
            case 'w':
                config.ws_port = std::atoi(optarg);
                break;
            case 'd':
                config.db_path = optarg;
                break;
            case 's':
                config.static_dir = optarg;
                break;
            case 'l':
                config.log_dir = optarg;
                break;
            case '?':
                config.show_help = true;
                break;
            case 'v':
                config.show_version = true;
                break;
            default:
                config.show_help = true;
                break;
        }
    }
    
    return config;
}

// 生成基于日期的日志文件名
std::string generateLogFileName(const std::string& log_dir) {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // 格式化日期字符串 (YYYY-MM-DD)
    char date_str[32];
    std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm);
    
    // 创建完整的日志文件路径
    std::filesystem::path log_path(log_dir);
    log_path /= std::string("swiftchat_") + date_str + ".log";
    
    return log_path.string();
}

void setupLogging(const std::string& log_dir) {
    // 生成基于日期的日志文件名
    std::string log_file = generateLogFileName(log_dir);
    
    // 创建日志目录
    std::filesystem::path log_path(log_file);
    std::filesystem::create_directories(log_path.parent_path());
    
    // 初始化文件日志记录器
    if (utils::Logger::initFileLogger(log_file)) {
        LOG_INFO << "日志系统已配置，输出到文件: " << log_file;
    } else {
        LOG_ERROR << "无法初始化文件日志记录器: " << log_file;
    }
    
    // 设置日志级别（可以根据环境变量设置）
    const char* log_level_env = std::getenv("LOG_LEVEL");
    if (log_level_env) {
        std::string level_str(log_level_env);
        if (level_str == "DEBUG") {
            utils::Logger::setGlobalLevel(utils::LogLevel::DEBUG);
        } else if (level_str == "INFO") {
            utils::Logger::setGlobalLevel(utils::LogLevel::INFO);
        } else if (level_str == "WARN") {
            utils::Logger::setGlobalLevel(utils::LogLevel::WARN);
        } else if (level_str == "ERROR") {
            utils::Logger::setGlobalLevel(utils::LogLevel::ERROR);
        } else if (level_str == "FATAL") {
            utils::Logger::setGlobalLevel(utils::LogLevel::FATAL);
        }
        LOG_INFO << "日志级别设置为: " << log_level_env;
    }
}

void signalHandler(int signal)
{
    LOG_INFO << "收到信号 " << signal << "，正在关闭服务器...";
    running = false;
}

int main(int argc, char *argv[])
{

    // 设置全局locale
    std::locale::global(std::locale("C"));

    // 解析命令行参数
    ServerConfig config = parseCommandLine(argc, argv);
    
    if (config.show_help) {
        showHelp(argv[0]);
        return 0;
    }
    
    if (config.show_version) {
        showVersion();
        return 0;
    }
    
    // 设置日志
    setupLogging(config.log_dir);
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try
    {
        LOG_INFO << "SwiftChat Server v1.0.0 启动中...";
        
        // 设置JWT密钥环境变量（如果未设置）
        if (!std::getenv("JWT_SECRET")) {
            setenv("JWT_SECRET", "your_secret_key_here", 1);
            LOG_WARN << "JWT_SECRET environment variable set to default value - 请在生产环境中设置安全密钥";
        }

        // 初始化数据库管理器
        DatabaseManager db_manager(config.db_path);
        LOG_INFO << "数据库管理器已初始化: " << config.db_path;

        // 创建HTTP服务器实例
        http::HttpServer server(config.http_port, 4); // 4个工作线程

        // 设置静态文件目录
        server.setStaticDirectory(config.static_dir);
        LOG_INFO << "静态文件目录: " << config.static_dir;

        // 设置中间件
        server.setMiddleware(middleware::auth);

        // 初始化服务
        AuthService auth_service(db_manager);
        RoomService room_service(db_manager);
        MessageService message_service(db_manager);
        UserService user_service(db_manager);
        ServerService server_service(db_manager);
        
        // 注册路由
        auth_service.registerRoutes(server);
        room_service.registerRoutes(server);
        message_service.registerRoutes(server);
        user_service.registerRoutes(server);
        server_service.registerRoutes(server);
        
        LOG_INFO << "所有服务已注册成功";

        // 创建并启动WebSocket服务器
        ws_server = std::make_unique<WebSocketServer>(db_manager);
        LOG_INFO << "WebSocket服务器已创建";

        // 启动信息
        std::cout << "SwiftChat Server v1.0.0 已启动" << std::endl;
        std::cout << "HTTP 服务器: http://localhost:" << config.http_port << std::endl;
        std::cout << "WebSocket 服务器: ws://localhost:" << config.ws_port << std::endl;
        std::cout << "访问 http://localhost:" << config.http_port << " 开始使用" << std::endl;
        std::cout << "按 Ctrl+C 退出服务器" << std::endl;

        // 在后台线程启动HTTP服务器
        std::thread server_thread([&server]()
                                  { 
                                      LOG_INFO << "HTTP服务器线程启动";
                                      server.run(); 
                                  });

        // 在后台线程启动WebSocket服务器
        std::thread websocket_thread([&]()
                                     {
                                         LOG_INFO << "WebSocket服务器线程启动";
                                         try {
                                             ws_server->run(config.ws_port);
                                         } catch (const std::exception& e) {
                                             LOG_ERROR << "WebSocket服务器启动失败: " << e.what();
                                         }
                                     });

        // 给服务器一些时间来启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        LOG_INFO << "HTTP服务器已启动在端口: " << config.http_port;
        LOG_INFO << "WebSocket服务器已启动在端口: " << config.ws_port;

        // 主循环
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 停止服务器
        LOG_INFO << "正在停止服务器...";
        
        // 停止WebSocket服务器
        if (ws_server) {
            ws_server->stop();
            LOG_INFO << "WebSocket服务器已停止";
        }
        
        // 停止HTTP服务器
        server.stop();
        LOG_INFO << "HTTP服务器已停止";

        // 等待服务器线程结束
        if (server_thread.joinable())
        {
            server_thread.join();
        }
        
        if (websocket_thread.joinable())
        {
            websocket_thread.join();
        }

        LOG_INFO << "所有服务器已关闭";
        
        // 关闭文件日志
        utils::Logger::closeFileLogger();
        
        std::cout << "服务器已安全关闭" << std::endl;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "服务器错误: " << e.what();
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
