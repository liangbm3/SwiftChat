#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class RoomMember {
 private:
  int64_t room_id_;       // 房间ID (BIGINT)
  int64_t user_id_;       // 用户ID (BIGINT)
  int64_t joined_at_;     // 加入时间戳

 public:
  // 构造函数
  RoomMember() : room_id_(0), user_id_(0), joined_at_(0) {}  // 默认构造函数
  RoomMember(int64_t room_id, int64_t user_id, int64_t joined_at)
      : room_id_(room_id),
        user_id_(user_id),
        joined_at_(joined_at) {}

  // Getter方法
  int64_t getRoomId() const { return room_id_; }
  int64_t getUserId() const { return user_id_; }
  int64_t getJoinedAt() const { return joined_at_; }

  // Setter方法
  void setRoomId(int64_t room_id) { room_id_ = room_id; }
  void setUserId(int64_t user_id) { user_id_ = user_id; }
  void setJoinedAt(int64_t joined_at) { joined_at_ = joined_at; }

  // JSON转换
  json toJson() const;
  static RoomMember fromJson(const json &j);
};
