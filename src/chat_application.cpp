#include "chat_application.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ChatApplication::ChatApplication(const std::string &static_dir)
    : static_dir_(static_dir), http_server_(nullptr), db_manager_(std::make_shared<DatabaseManager>("chat.db"))
{
}

void ChatApplication::start(int port)
{
    LOG_INFO << "Creating HTTP server on port " << port;
    http_server_ = std::make_unique<http::HttpServer>(port);
    LOG_INFO << "Setting up routes";
    setupRoutes();
    LOG_INFO << "Starting HTTP server";
    http_server_->run();
}

void ChatApplication::stop()
{
    if (http_server_)
    {
        LOG_INFO << "Stopping HTTP server";
        http_server_->stop();
        http_server_.reset();
    }
}

void ChatApplication::setupRoutes()
{
    // 静态文件处理请求
    LOG_INFO << "Setting up static file route";
    http_server_->addHandler("/", "GET", [this](const http::HttpRequest &request) -> http::HttpResponse
                             { return serveStaticFile("/login.html"); });
    // 处理注册请求
    http_server_->addHandler("/register", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                             {
                                 try
                                 {
                                     json data = json::parse(request.body);
                                     if (!data.contains("username") || !data.contains("password"))
                                     {
                                         return http::HttpResponse(400, "Missing username or password");
                                     }
                                     std::string username = data["username"];
                                     std::string password = data["password"];
                                     if (db_manager_->userExists(username))
                                     {
                                         LOG_WARN << "User already exists: " << username;
                                         return http::HttpResponse(409, "User already exists");
                                     }
                                     if (db_manager_->createUser(username, password))
                                     {
                                         LOG_INFO << "User created successfully: " << username;
                                         return http::HttpResponse(201, "User created successfully");
                                     }
                                     else
                                     {
                                         LOG_ERROR << "Failed to create user in database: " << username;
                                         return http::HttpResponse(500, "{\"error\":\"Internal server error\"}");
                                     }
                                 }
                                 catch (const json::exception &e)
                                 {
                                     LOG_ERROR << "JSON parsing error: " << e.what();
                                     return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
                                 } });

    // 处理登录请求
    http_server_->addHandler("/login", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                             {
        try
        {
            json data=json::parse(request.body);
            if(!data.contains("username") || !data.contains("password"))
            {
                LOG_ERROR<<"Missing username or password in login request";
                return http::HttpResponse(400, "{\"error\":\"Missing username or password\"}");
            }
            std::string username = data["username"];
            std::string password = data["password"];

            if(db_manager_->validateUser(username, password))
            {
                LOG_INFO << "User logged in successfully: " << username;
                db_manager_->setUserOnlineStatus(username, true);
                db_manager_->updateUserLastActiveTime(username);
                json response_data = {
                    {"status", "success"},
                    {"message", "Login successful"},
                    {"username", username}
                };
                return http::HttpResponse(200, response_data.dump());
            }
            else
            {
                LOG_WARN << "Invalid login attempt for user: " << username;
                return http::HttpResponse(401, "{\"error\":\"Invalid username or password\"}");
            }
        }
        catch(const json::exception& e)
        {
           LOG_ERROR<<"JSON parsing error: " << e.what();
           return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
        } });

    // 创建聊天室
    http_server_->addHandler("/create_room", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                             {
        try
        {
            json data=json::parse(request.body);
            if(!data.contains("room_name") || !data.contains("creator"))
            {
                LOG_ERROR<<"Missing room_name or creator in create_room request";
                return http::HttpResponse(400, "{\"error\":\"Missing room_name or creator\"}");
            }
            std::string room_name = data["room_name"];
            std::string creator = data["creator"];

            if(db_manager_->createRoom(room_name, creator))
            {
                if(db_manager_->addRoomMember(room_name, creator))
                {
                    LOG_INFO << "Room created successfully: " << room_name << " by " << creator;
                    return http::HttpResponse(201, "{\"status\":\"success\",\"message\":\"Room created successfully\"}");
                }
                else
                {
                    LOG_ERROR << "Failed to add creator to room: " << room_name;
                    return http::HttpResponse(500, "{\"error\":\"Failed to add creator to room\"}");
                }
            }
            else
            {
                LOG_ERROR << "Failed to create room in database: " << room_name;
                return http::HttpResponse(500, "{\"error\":\"Internal server error\"}");
            }
        }
        catch(const json::exception& e)
        {
           LOG_ERROR<<"JSON parsing error: " << e.what();
           return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
        } });

    // 加入聊天室
    http_server_->addHandler("/join_room", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                             {
        try
        {
            json data=json::parse(request.body);
            if(!data.contains("room_name") || !data.contains("username"))
            {
                LOG_ERROR<<"Missing room_name or username in join_room request";
                return http::HttpResponse(400, "{\"error\":\"Missing room_name or username\"}");
            }
            std::string room_name = data["room_name"];
            std::string username = data["username"];

            if(db_manager_->addRoomMember(room_name, username))
            {
                LOG_INFO << "User " << username << " joined room: " << room_name;
                return http::HttpResponse(200, "{\"status\":\"success\",\"message\":\"Joined room successfully\"}");
            }
            else
            {
                LOG_ERROR << "Failed to add user to room: " << room_name;
                return http::HttpResponse(500, "{\"error\":\"Failed to join room\"}");
            }
        }
        catch(const json::exception& e)
        {
           LOG_ERROR<<"JSON parsing error: " << e.what();
           return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
        } });
    // 获取聊天室列表
    http_server_->addHandler("/rooms", "GET", [this](const http::HttpRequest &request) -> http::HttpResponse
                             {
        auto rooms = db_manager_->getRooms();
        json response_data = json::array();
        for (const auto &room : rooms)
        {
            json room_data =
            {
                {"name", room},
                {"members", db_manager_->getRoomMembers(room)}
            };
            response_data.push_back(room_data);
        }
        return http::HttpResponse(200, response_data.dump()); });

    // 处理发送消息请求
    http_server_->addHandler("/send_message", "POST", [this](const http::HttpRequest &request) -> http::HttpResponse
                             {
        try
        {
            json data=json::parse(request.body);
            if(!data.contains("room_name") || ! data.contains("username") || !data.contains("content"))
            {
                LOG_ERROR<<"Missing room_name, username or content in send_message request";
                return http::HttpResponse(400, "{\"error\":\"Missing room_name, username or content\"}");
            }
            std::string room_name = data["room_name"];
            std::string username = data["username"];
            std::string content = data["content"];  
            int64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count();

            if(db_manager_->saveMessage(room_name, username, content, timestamp))
            {
                LOG_INFO << "Message sent to room " << room_name << " by user " << username;
                return http::HttpResponse(200, "{\"status\":\"success\",\"message\":\"Message sent successfully\"}");
            }
            else
            {
                LOG_ERROR << "Failed to send message to room " << room_name;
                return http::HttpResponse(500, "{\"error\":\"Failed to send message\"}");
            }
        }
        catch(const json::exception& e)
        {
           LOG_ERROR<<"JSON parsing error: " << e.what();
           return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
        } });
    // 获取新消息
    http_server_->addHandler("/get_messages", "GET", [this](const http::HttpRequest &request) -> http::HttpResponse {
        try
        {
            json data=json::parse(request.body);
            if(!data.contains("room_name") || !data.contains("since"))
            {
                LOG_ERROR<<"Missing room_name or since in get_messages request";
                return http::HttpResponse(400, "{\"error\":\"Missing room_name or since\"}");
            }
            std::string room_name = data["room_name"];
            int64_t since = data["since"];
            auto messages = db_manager_->getMessages(room_name, since);
            //更新用户最后活跃的时间
            if(data.contains("username"))
            {
                std::string username = data["username"];
                db_manager_->updateUserLastActiveTime(username);
            }
            return http::HttpResponse(200, json(messages).dump());
        }
        catch(const json::exception& e)
        {
           LOG_ERROR<<"JSON parsing error: " << e.what();
           return http::HttpResponse(400, "{\"error\":\"Invalid JSON format\"}");
        }
        
    });
}

http::HttpResponse ChatApplication::serveStaticFile(const std::string &path)
{
    std::string full_path = static_dir_ + path;

    struct stat st;
    if (stat(full_path.c_str(), &st) == -1)
    {
        if (errno == ENOENT)
        {
            LOG_WARN << "File not found: " << full_path;
            return http::HttpResponse(404, "File not found");
        }
        LOG_ERROR << "Failed to check file: " << full_path << ", error: " << strerror(errno);
        return http::HttpResponse(500, "Internal server error");
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file)
    {
        LOG_ERROR << "Failed to open file: " << full_path;
        return http::HttpResponse(500, "Failed to open file");
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // 根据文件扩展名设置 Content-Type
    std::string content_type = "text/plain";
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        std::string ext = path.substr(dot_pos);
        if (ext == ".html")
        {
            content_type = "text/html";
        }
        else if (ext == ".css")
        {
            content_type = "text/css";
        }
        else if (ext == ".js")
        {
            content_type = "application/javascript";
        }
        else if (ext == ".json")
        {
            content_type = "application/json";
        }
        else if (ext == ".png")
        {
            content_type = "image/png";
        }
        else if (ext == ".jpg" || ext == ".jpeg")
        {
            content_type = "image/jpeg";
        }
    }

    http::HttpResponse response(200, content);
    response.headers["Content-Type"] = content_type;
    return response;
}