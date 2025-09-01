#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class Message {
 private:
  int64_t id_;             // 消息ID (BIGINT AUTO_INCREMENT)
  int64_t room_id_;        // 房间ID (BIGINT)
  int64_t sender_id_;      // 发送者用户ID (BIGINT)
  std::string content_;    // 消息内容
  std::string created_at_; // 时间戳 (TIMESTAMP)
  std::string user_name_;  // 发送者姓名

 public:
  // 构造函数
  Message() : id_(0), room_id_(0), sender_id_(0) {}  // 默认构造函数
  Message(int64_t id, int64_t room_id, int64_t sender_id,
          const std::string &content, const std::string &created_at,
          const std::string &user_name)
      : id_(id),
        room_id_(room_id),
        sender_id_(sender_id),
        content_(content),
        created_at_(created_at),
        user_name_(user_name) {}

  // Getter方法
  int64_t getId() const { return id_; }
  int64_t getRoomId() const { return room_id_; }
  int64_t getSenderId() const { return sender_id_; }
  const std::string &getContent() const { return content_; }
  const std::string &getCreatedAt() const { return created_at_; }
  const std::string &getUserName() const { return user_name_; }

  // Setter方法
  void setId(int64_t id) { id_ = id; }
  void setRoomId(int64_t room_id) { room_id_ = room_id; }
  void setSenderId(int64_t sender_id) { sender_id_ = sender_id; }
  void setContent(const std::string &content) { content_ = content; }
  void setCreatedAt(const std::string &created_at) { created_at_ = created_at; }
  void setUserName(const std::string &user_name) { user_name_ = user_name; }

  // JSON转换
  json toJson() const;
  static Message fromJson(const json &j);
};
