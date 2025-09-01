#include "message.hpp"

json Message::toJson() const {
  json j;
  j["id"] = id_;
  j["room_id"] = room_id_;
  j["sender_id"] = sender_id_;
  j["content"] = content_;
  j["created_at"] = created_at_;
  j["user_name"] = user_name_;

  return j;
}

Message Message::fromJson(const json &j) {
  Message message;

  if (j.contains("id") && j["id"].is_number_integer()) message.id_ = j["id"];

  if (j.contains("room_id") && j["room_id"].is_number_integer())
    message.room_id_ = j["room_id"];

  if (j.contains("sender_id") && j["sender_id"].is_number_integer())
    message.sender_id_ = j["sender_id"];

  if (j.contains("content") && j["content"].is_string())
    message.content_ = j["content"];

  if (j.contains("created_at") && j["created_at"].is_string())
    message.created_at_ = j["created_at"];

  if (j.contains("user_name") && j["user_name"].is_string())
    message.user_name_ = j["user_name"];

  return message;
}
