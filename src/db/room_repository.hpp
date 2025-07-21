#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "database_connection.hpp"
#include "../model/room.hpp"

// 房间数据访问类
class RoomRepository
{
public:
    explicit RoomRepository(DatabaseConnection* db_conn);

    // 房间基本操作
    std::optional<Room> createRoom(const std::string &name, const std::string &description, const std::string &creator_id);
    bool deleteRoom(const std::string &room_id);// 根据ID删除房间
    bool roomExists(const std::string &room_id);// 根据ID检查房间是否存在
    bool updateRoom(const std::string &room_id, const std::string &name, const std::string &description);// 更新房间
    
    // 房间查询
    std::vector<std::string> getRooms();// 获取所有房间（仅名称）
    std::vector<Room> getAllRooms();// 获取所有房间的详细信息
    std::optional<Room> getRoomById(const std::string &room_id) const;// 根据ID获取房间信息
    std::optional<std::string> getRoomIdByName(const std::string &room_name) const;// 根据房间名获取房间ID
    bool isRoomCreator(const std::string &room_id, const std::string &user_id);// 检查是否为房间创建者
    
    // 房间成员管理
    std::vector<nlohmann::json> getRoomMembers(const std::string &room_id) const;// 获取房间成员
    std::vector<Room> getUserJoinedRooms(const std::string &user_id) const;// 获取用户已加入的房间列表
    bool addRoomMember(const std::string &room_id, const std::string &user_id);// 根据ID添加房间成员
    bool removeRoomMember(const std::string &room_id, const std::string &user_id);// 根据ID移除房间成员
    
    // 工具方法
    std::string generateRoomId();

private:
    DatabaseConnection* db_conn_;
};
