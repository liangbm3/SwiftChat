#include "websocket/websocket_server.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    // 设置日志级别
    utils::Logger::setGlobalLevel(utils::LogLevel::INFO);
    
    try {
        WebSocketServer server;
        
        // 设置消息处理回调
        server.setOnMessage([&server](auto hdl, auto msg) {
            std::string payload = msg->get_payload();
            LOG_INFO << "Received message: " << payload;
            
            try {
                // 尝试解析JSON消息
                json request = json::parse(payload);
                
                json response;
                response["type"] = "response";
                response["timestamp"] = std::time(nullptr);
                
                if (request.contains("type")) {
                    std::string type = request["type"];
                    
                    if (type == "echo") {
                        response["data"] = request["data"];
                        response["message"] = "Echo response";
                    } else if (type == "broadcast") {
                        std::string message = request["message"];
                        server.broadcast("Broadcast: " + message);
                        response["message"] = "Message broadcasted";
                    } else {
                        response["error"] = "Unknown message type";
                    }
                } else {
                    response["error"] = "Missing message type";
                }
                
                // 发送响应
                server.send(hdl, response.dump());
                
            } catch (const json::exception& e) {
                // 如果不是JSON，当作普通文本处理
                server.send(hdl, "Echo: " + payload);
            }
        });
        
        // 设置连接打开回调
        server.setOnOpen([&server](auto hdl) {
            LOG_INFO << "New client connected";
            
            json welcome;
            welcome["type"] = "welcome";
            welcome["message"] = "Welcome to SwiftChat WebSocket Server!";
            welcome["timestamp"] = std::time(nullptr);
            
            server.send(hdl, welcome.dump());
        });
        
        // 设置连接关闭回调
        server.setOnClose([](auto hdl) {
            LOG_INFO << "Client disconnected";
        });
        
        // 启动服务器
        int port = 8080;
        std::cout << "Starting WebSocket server on port " << port << std::endl;
        std::cout << "You can test it with a WebSocket client or browser console:" << std::endl;
        std::cout << "  const ws = new WebSocket('ws://localhost:" << port << "');" << std::endl;
        std::cout << "  ws.onmessage = e => console.log('Received:', e.data);" << std::endl;
        std::cout << "  ws.send(JSON.stringify({type: 'echo', data: 'Hello World'}));" << std::endl;
        std::cout << "Press Ctrl+C to stop the server." << std::endl;
        
        server.start(port);
        
        // 保持服务器运行
        std::cout << "Server is running. Press Enter to stop..." << std::endl;
        std::cin.get();
        
        server.stop();
        std::cout << "Server stopped." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
