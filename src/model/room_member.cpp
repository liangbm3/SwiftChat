#include "room_member.hpp"

json RoomMember::toJson() const {
  json j;
  j["room_id"] = room_id_;
  j["user_id"] = user_id_;
  j["joined_at"] = joined_at_;

  return j;
}

RoomMember RoomMember::fromJson(const json &j) {
  RoomMember roomMember;

  if (j.contains("room_id") && j["room_id"].is_number_integer())
    roomMember.room_id_ = j["room_id"];

  if (j.contains("user_id") && j["user_id"].is_number_integer())
    roomMember.user_id_ = j["user_id"];

  if (j.contains("joined_at") && j["joined_at"].is_number_integer())
    roomMember.joined_at_ = j["joined_at"];

  return roomMember;
}
