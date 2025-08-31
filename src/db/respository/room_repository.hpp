#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../../model/room.hpp"
#include "../../model/user.hpp"
#include "../connection_pool.hpp"
#include "../mysql_statement.hpp"

namespace db {

// 房间数据访问类
class RoomRepository {
 public:
  explicit RoomRepository(ConnectionPool &pool);  // 构造函数，接受连接池引用

  // 房间基本操作
  std::optional<int64_t> createRoom(const std::string &name, int64_t creator_id);  // 创建房间，返回房间ID
  bool deleteRoom(int64_t room_id);        // 根据ID删除房间
  bool roomExists(int64_t room_id) const;  // 根据ID检查房间是否存在
  bool updateRoom(int64_t room_id, const std::string &name,
                  const std::string &description);  // 更新房间

  // 房间查询
  std::vector<std::string> getAllRoomNames() const;  // 获取所有房间名称
  std::vector<Room> getAllRooms() const;             // 获取所有房间详细信息
  std::optional<Room> getRoom(int64_t room_id) const;  // 根据ID获取房间详细信息
  std::optional<Room> getRoom(const std::string &room_name) const;  // 根据房间名获取房间详细信息
  std::optional<int64_t> getRoomIdByName(const std::string &room_name) const;  // 根据房间名获取房间ID
  bool isRoomCreator(int64_t user_id, int64_t room_id) const;  // 验证用户是否为房间创建者

  // 房间成员管理
  std::vector<User> getRoomMembers(int64_t room_id) const;  // 获取房间成员
  bool addRoomMember(int64_t room_id, int64_t user_id);     // 添加房间成员
  bool removeRoomMember(int64_t room_id, int64_t user_id);  // 移除房间成员

 private:
  ConnectionPool &pool_;
};

}  // namespace db
