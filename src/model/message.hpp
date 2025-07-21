#pragma once
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Message
{
private:
    int64_t id_;           // 消息ID
    std::string room_id_;  // 房间ID
    std::string user_id_;  // 发送者用户ID
    std::string content_;  // 消息内容
    int64_t timestamp_;    // 时间戳
    std::string user_name_; // 发送者姓名

public:
    // 构造函数
    Message() : id_(0), timestamp_(0) {} // 默认构造函数
    Message(int64_t id, const std::string &room_id, const std::string &user_id,
            const std::string &content, int64_t timestamp, const std::string &user_name)
        : id_(id), room_id_(room_id), user_id_(user_id), content_(content), 
          timestamp_(timestamp), user_name_(user_name) {}

    // Getter方法
    int64_t getId() const { return id_; }
    const std::string &getRoomId() const { return room_id_; }
    const std::string &getUserId() const { return user_id_; }
    const std::string &getContent() const { return content_; }
    int64_t getTimestamp() const { return timestamp_; }
    const std::string &getUserName() const { return user_name_; }

    // Setter方法
    void setId(int64_t id) { id_ = id; }
    void setRoomId(const std::string &room_id) { room_id_ = room_id; }
    void setUserId(const std::string &user_id) { user_id_ = user_id; }
    void setContent(const std::string &content) { content_ = content; }
    void setTimestamp(int64_t timestamp) { timestamp_ = timestamp; }
    void setUserName(const std::string &user_name) { user_name_ = user_name; }

    // JSON转换
    json toJson() const;
    static Message fromJson(const json &j);
};
