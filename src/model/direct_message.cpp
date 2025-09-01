#include "direct_message.hpp"

json DirectMessage::toJson() const {
  json j;
  j["id"] = id_;
  j["sender_id"] = sender_id_;
  j["receiver_id"] = receiver_id_;
  j["content"] = content_;
  j["created_at"] = created_at_;

  return j;
}

DirectMessage DirectMessage::fromJson(const json &j) {
  DirectMessage directMessage;

  if (j.contains("id") && j["id"].is_number_integer()) 
    directMessage.id_ = j["id"];

  if (j.contains("sender_id") && j["sender_id"].is_number_integer())
    directMessage.sender_id_ = j["sender_id"];

  if (j.contains("receiver_id") && j["receiver_id"].is_number_integer())
    directMessage.receiver_id_ = j["receiver_id"];

  if (j.contains("content") && j["content"].is_string())
    directMessage.content_ = j["content"];

  if (j.contains("created_at") && j["created_at"].is_string())
    directMessage.created_at_ = j["created_at"];

  return directMessage;
}
