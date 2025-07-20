#include "room_service.hpp"
#include "db/database_manager.hpp"
#include "http/http_server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utils/logger.hpp"
#include "utils/jwt_utils.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <optional>
#include <chrono>

using json = nlohmann::json;

RoomService::RoomService(DatabaseManager &db_manager) : db_manager_(db_manager) {}

void RoomService::registerRoutes(http::HttpServer &server)
{
    // 注册创建房间的路由
    server.addHandler({
        .path = "/api/v1/rooms",
        .method = "POST",
        .handler = [this](const http::HttpRequest &request) { return handleCreateRoom(request); },
        .use_auth_middleware = true
    });

    // 注册获取房间列表的路由
    server.addHandler({
        .path = "/api/v1/rooms",
        .method = "GET",
        .handler = [this](const http::HttpRequest &request) { return handleGetRooms(request); },
        .use_auth_middleware = false
    });

    // 注册加入房间的路由
    server.addHandler({
        .path = "/api/v1/rooms/join",
        .method = "POST",
        .handler = [this](const http::HttpRequest &request) { return handleJoinRoom(request); },
        .use_auth_middleware = true
    });

    // 注册退出房间的路由
    server.addHandler({
        .path = "/api/v1/rooms/leave",
        .method = "POST",
        .handler = [this](const http::HttpRequest &request) { return handleLeaveRoom(request); },
        .use_auth_middleware = true
    });

    // 注册更改房间描述的路由
    server.addHandler({
        .path = "/api/v1/rooms/{room_id}",
        .method = "PATCH",
        .handler = [this](const http::HttpRequest &request) { return handleUpdateRoomDescription(request); },
        .use_auth_middleware = true
    });

    // 注册删除房间的路由
    server.addHandler({
        .path = "/api/v1/rooms/{room_id}",
        .method = "DELETE",
        .handler = [this](const http::HttpRequest &request) { return handleDeleteRoom(request); },
        .use_auth_middleware = true
    });
}

std::optional<std::string> RoomService::getUserIdFromRequest(const http::HttpRequest &request)
{
    return JwtUtils::getUserIdFromRequest(request);
}

http::HttpResponse RoomService::handleCreateRoom(const http::HttpRequest &request)
{
    auto user_id_opt = getUserIdFromRequest(request);//获取创建者ID
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "Invalid or missing JWT token"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }
    
    std::string user_id = *user_id_opt;

    json request_body;
    try
    {
        request_body = json::parse(request.getBody());
        std::string room_name = request_body.at("name").get<std::string>();//尝试获取房间名
        std::string room_description = request_body.value("description", ""); // 获取房间描述，如果没有则默认为空
        
        //调用DB接口创建房间
        auto room_opt = db_manager_.createRoom(room_name, room_description, user_id);
        if (!room_opt)
        {
            LOG_ERROR << "Failed to create room for user: " << user_id;
            json error_response = {
                {"success", false},
                {"message", "Failed to create room"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        //创建者自动加入房间
        std::string room_id = room_opt->getId();
        db_manager_.addRoomMember(room_id, user_id);

        json success_response = {
            {"success", true},
            {"message", "Room created successfully"},
            {"data", room_opt->toJson()}
        };
        return http::HttpResponse::Created().withJsonBody(success_response);
    }
    catch (const json::parse_error &e)
    {
        LOG_ERROR << "Failed to parse JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Invalid JSON format"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const json::out_of_range &e)
    {
        LOG_ERROR << "Missing required fields in JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Missing required fields"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Unexpected error while creating room: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Unexpected error occurred"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse RoomService::handleJoinRoom(const http::HttpRequest &request)
{
    // 获取当前用户的ID
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "Invalid or missing JWT token"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }

    std::string user_id = *user_id_opt;

    try
    {
        // 添加调试信息：请求体内容
        std::string request_body = request.getBody();
        LOG_INFO << "Join room request body: " << request_body;
        
        if (request_body.empty()) {
            LOG_ERROR << "Request body is empty";
            json error_response = {
                {"success", false},
                {"message", "Empty request body"},
                {"error", "Request body is required"}
            };
            return http::HttpResponse::BadRequest().withJsonBody(error_response);
        }
        
        // 从请求体中获取房间ID
        auto json_body = json::parse(request_body);
        LOG_INFO << "Parsed JSON: " << json_body.dump();
        
        std::string room_id = json_body.value("room_id", "");
        LOG_INFO << "Room ID from request: '" << room_id << "'";
        
        if (room_id.empty()) {
            LOG_ERROR << "Room ID is empty or missing";
            json error_response = {
                {"success", false},
                {"message", "Room ID is required"},
                {"error", "Missing room_id field"}
            };
            return http::HttpResponse::BadRequest().withJsonBody(error_response);
        }
        
        // 检查房间是否存在
        if (!db_manager_.roomExists(room_id)) {
            LOG_ERROR << "Room not found: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Room not found"},
                {"error", "Invalid room ID"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        //检查房间和用户是否存在
        bool room_exists = db_manager_.roomExists(room_id);
        bool user_exists = db_manager_.userExists(user_id);
        
        LOG_INFO << "Room exists check - Room ID: " << room_id << ", exists: " << room_exists;
        LOG_INFO << "User exists check - User ID: " << user_id << ", exists: " << user_exists;
        
        if(!room_exists)
        {
            LOG_ERROR << "Room does not exist. Room ID: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Room does not exist"},
                {"error", "Invalid room ID"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }
        
        if(!user_exists)
        {
            LOG_ERROR << "User does not exist. User ID: " << user_id;
            json error_response = {
                {"success", false},
                {"message", "User does not exist"},
                {"error", "Invalid user ID"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        //将用户加入房间
        if (!db_manager_.addRoomMember(room_id, user_id))
        {
            LOG_ERROR << "Failed to add user to room. Room ID: " << room_id << ", User ID: " << user_id;
            json error_response = {
                {"success", false},
                {"message", "Failed to join room"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        // 成功加入房间
        LOG_INFO << "User " << user_id << " successfully joined room " << room_id;
        
        // 获取当前时间戳
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        
        json response_data = {
            {"success", true},
            {"message", "Room joined successfully"},
            {"data", {
                {"room_id", room_id},
                {"user_id", user_id},
                {"joined_at", timestamp}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch(const json::parse_error &e)
    {
        LOG_ERROR << "Failed to parse JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Invalid JSON format"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const json::out_of_range &e)
    {
        LOG_ERROR << "Missing required fields in JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Missing required fields"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to join room for user: " << user_id << ", Error: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to join room"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse RoomService::handleGetRooms(const http::HttpRequest &request)
{
    try
    {
        // 获取查询参数
        int limit = 50; // 默认限制
        int offset = 0; // 默认偏移量

        if (auto limit_opt = request.getQueryParam("limit"))
        {
            try
            {
                std::string limit_str(limit_opt->data(), limit_opt->size());
                limit = std::stoi(limit_str);
                if (limit <= 0 || limit > 100) // 限制在1到100之间
                {
                    LOG_WARN << "Invalid limit value: " << limit << ". Using default value of 50.";
                    limit = 50;
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "Invalid limit parameter. Using default value of 50.";
                limit = 50;
            }
        }

        if (auto offset_opt = request.getQueryParam("offset"))
        {
            try
            {
                std::string offset_str(offset_opt->data(), offset_opt->size());
                offset = std::stoi(offset_str);
                if (offset < 0)
                {
                    LOG_WARN << "Invalid offset value: " << offset << ". Using default value of 0.";
                    offset = 0;
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "Invalid offset parameter. Using default value of 0.";
                offset = 0;
            }
        }

        // 从数据库获取所有房间
        auto all_rooms = db_manager_.getAllRooms();
        
        // 在服务层实现分页逻辑
        size_t total_count = all_rooms.size();
        size_t start_index = std::min(static_cast<size_t>(offset), total_count);
        size_t end_index = std::min(start_index + static_cast<size_t>(limit), total_count);
        
        std::vector<Room> rooms;
        if (start_index < total_count) {
            rooms.assign(all_rooms.begin() + start_index, all_rooms.begin() + end_index);
        }
        
        // 将Room对象转换为JSON数组，并添加成员数量
        json rooms_json = json::array();
        for (const auto& room : rooms) {
            json room_json = room.toJson();
            
            // 获取房间成员数量
            auto members = db_manager_.getRoomMembers(room.getId());
            room_json["member_count"] = members.size();
            
            rooms_json.push_back(room_json);
        }
        
        // 构造标准的JSON响应格式
        json json_response = {
            {"success", true},
            {"message", "Rooms retrieved successfully"},
            {"data", {
                {"rooms", rooms_json},
                {"count", rooms.size()},
                {"total", total_count},
                {"limit", limit},
                {"offset", offset}
            }}
        };
        
        return http::HttpResponse::Ok().withJsonBody(json_response);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to get rooms: " << e.what();
        
        json error_response = {
            {"success", false},
            {"message", "Failed to get rooms"},
            {"error", e.what()}
        };
        
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse RoomService::handleLeaveRoom(const http::HttpRequest &request)
{
    // 获取当前用户的ID
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "Invalid or missing JWT token"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }

    std::string user_id = *user_id_opt;

    try
    {
        // 从请求体中获取房间ID
        auto json_body = json::parse(request.getBody());
        std::string room_id = json_body.at("room_id").get<std::string>();
        
        // 检查房间是否存在
        if (!db_manager_.roomExists(room_id))
        {
            LOG_ERROR << "Room not found: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Room not found"},
                {"error", "Invalid room ID"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        // 退出房间
        if (!db_manager_.removeRoomMember(room_id, user_id))
        {
            LOG_ERROR << "Failed to remove user from room. Room ID: " << room_id << ", User ID: " << user_id;
            json error_response = {
                {"success", false},
                {"message", "Failed to leave room"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        // 成功退出房间
        LOG_INFO << "User " << user_id << " successfully left room " << room_id;
        json response_data = {
            {"success", true},
            {"message", "Room left successfully"},
            {"data", {
                {"room_id", room_id},
                {"user_id", user_id}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch(const json::parse_error &e)
    {
        LOG_ERROR << "Failed to parse JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Invalid JSON format"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const json::out_of_range &e)
    {
        LOG_ERROR << "Missing required fields in JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Missing required fields"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to leave room for user: " << user_id << ", Error: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to leave room"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse RoomService::handleDeleteRoom(const http::HttpRequest &request)
{
    // 获取当前用户的ID
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "Invalid or missing JWT token"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }

    std::string user_id = *user_id_opt;

    try
    {
        // 从路径参数中获取房间ID
        auto room_id_opt = request.getPathParam("room_id");
        if (!room_id_opt)
        {
            LOG_ERROR << "Missing room_id path parameter";
            json error_response = {
                {"success", false},
                {"message", "Room ID is required"},
                {"error", "Missing room_id path parameter"}
            };
            return http::HttpResponse::BadRequest().withJsonBody(error_response);
        }
        
        std::string room_id = std::string(*room_id_opt);
        
        // 检查房间是否存在
        if (!db_manager_.roomExists(room_id))
        {
            LOG_ERROR << "Room not found: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Room not found"},
                {"error", "Invalid room ID"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        // 验证用户是否为房间创建者
        auto room_info = db_manager_.getRoomById(room_id);
        if (!room_info || room_info->getCreatorId() != user_id)
        {
            LOG_ERROR << "User " << user_id << " is not the creator of room " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Access denied"},
                {"error", "Only the room creator can delete the room"}
            };
            return http::HttpResponse::Forbidden().withJsonBody(error_response);
        }

        // 删除房间
        if (!db_manager_.deleteRoom(room_id))
        {
            LOG_ERROR << "Failed to delete room: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Failed to delete room"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        // 成功删除房间
        LOG_INFO << "Room " << room_id << " successfully deleted by user " << user_id;
        json response_data = {
            {"success", true},
            {"message", "Room deleted successfully"},
            {"data", {
                {"room_id", room_id},
                {"deleted_by", user_id}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to delete room, Error: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to delete room"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}

http::HttpResponse RoomService::handleUpdateRoomDescription(const http::HttpRequest &request)
{
    // 获取当前用户的ID
    auto user_id_opt = getUserIdFromRequest(request);
    if (!user_id_opt)
    {
        LOG_ERROR << "Failed to get user ID from request.";
        json error_response = {
            {"success", false},
            {"message", "Authentication required"},
            {"error", "Invalid or missing JWT token"}
        };
        return http::HttpResponse::Unauthorized().withJsonBody(error_response);
    }

    std::string user_id = *user_id_opt;

    try
    {
        // 从路径参数中获取房间ID
        auto room_id_opt = request.getPathParam("room_id");
        if (!room_id_opt)
        {
            LOG_ERROR << "Missing room_id path parameter";
            json error_response = {
                {"success", false},
                {"message", "Room ID is required"},
                {"error", "Missing room_id path parameter"}
            };
            return http::HttpResponse::BadRequest().withJsonBody(error_response);
        }
        
        std::string room_id = std::string(*room_id_opt);
        
        // 从请求体中获取新描述
        auto json_body = json::parse(request.getBody());
        std::string new_description = json_body.at("description").get<std::string>();
        
        // 检查房间是否存在
        if (!db_manager_.roomExists(room_id))
        {
            LOG_ERROR << "Room not found: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Room not found"},
                {"error", "Invalid room ID"}
            };
            return http::HttpResponse::NotFound().withJsonBody(error_response);
        }

        // 验证用户是否为房间创建者
        auto room_info = db_manager_.getRoomById(room_id);
        if (!room_info || room_info->getCreatorId() != user_id)
        {
            LOG_ERROR << "User " << user_id << " is not the creator of room " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Access denied"},
                {"error", "Only the room creator can update the room description"}
            };
            return http::HttpResponse::Forbidden().withJsonBody(error_response);
        }

        // 获取当前房间信息以保留房间名
        auto current_room_info = db_manager_.getRoomById(room_id);
        if (!current_room_info)
        {
            LOG_ERROR << "Failed to get current room info: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Failed to get room information"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }
        
        std::string current_room_name = current_room_info->getName();

        // 更新房间描述
        if (!db_manager_.updateRoom(room_id, current_room_name, new_description))
        {
            LOG_ERROR << "Failed to update room description for room: " << room_id;
            json error_response = {
                {"success", false},
                {"message", "Failed to update room description"},
                {"error", "Database operation failed"}
            };
            return http::HttpResponse::InternalError().withJsonBody(error_response);
        }

        // 成功更新房间描述
        LOG_INFO << "Room " << room_id << " description updated by user " << user_id;
        json response_data = {
            {"success", true},
            {"message", "Room description updated successfully"},
            {"data", {
                {"room_id", room_id},
                {"new_description", new_description},
                {"updated_by", user_id}
            }}
        };
        return http::HttpResponse::Ok().withJsonBody(response_data);
    }
    catch(const json::parse_error &e)
    {
        LOG_ERROR << "Failed to parse JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Invalid JSON format"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const json::out_of_range &e)
    {
        LOG_ERROR << "Missing required fields in JSON body: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Missing required fields (description is required)"},
            {"error", e.what()}
        };
        return http::HttpResponse::BadRequest().withJsonBody(error_response);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Failed to update room description, Error: " << e.what();
        json error_response = {
            {"success", false},
            {"message", "Failed to update room description"},
            {"error", e.what()}
        };
        return http::HttpResponse::InternalError().withJsonBody(error_response);
    }
}