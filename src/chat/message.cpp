#include "message.hpp"

json Message::toJson() const
{
    json j;
    j["id"] = id_;
    j["room_id"] = room_id_;
    j["user_id"] = user_id_;
    j["content"] = content_;
    j["timestamp"] = timestamp_;
    
    // 如果有发送者信息，则包含发送者信息
    if (hasSenderInfo())
    {
        j["sender"] = sender_.toJson();
    }
    
    return j;
}

Message Message::fromJson(const json &j)
{
    Message message;
    
    if (j.contains("id") && j["id"].is_number_integer())
        message.id_ = j["id"];
    
    if (j.contains("room_id") && j["room_id"].is_string())
        message.room_id_ = j["room_id"];
    
    if (j.contains("user_id") && j["user_id"].is_string())
        message.user_id_ = j["user_id"];
    
    if (j.contains("content") && j["content"].is_string())
        message.content_ = j["content"];
    
    if (j.contains("timestamp") && j["timestamp"].is_number_integer())
        message.timestamp_ = j["timestamp"];
    
    // 如果包含发送者信息，则解析发送者信息
    if (j.contains("sender") && j["sender"].is_object())
    {
        message.sender_ = User::fromJson(j["sender"]);
    }
    
    return message;
}
