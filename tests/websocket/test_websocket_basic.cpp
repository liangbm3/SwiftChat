#include "websocket/websocket_server.hpp" // 确保路径正确
#include "utils/logger.hpp" // 日志记录工具

int main()
{
    try
    {
        WebSocketServer ws_server;
        ws_server.run(9002); // 启动WebSocket服务器，监听9002端口
        LOG_INFO << "WebSocket server started on port 9002";

        // 阻塞主线程，直到服务器停止
        std::this_thread::sleep_for(std::chrono::hours(24));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "WebSocket server failed to start: " << e.what();
        return EXIT_FAILURE;
    }
}