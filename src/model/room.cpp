#include "room.hpp"

json Room::toJson() const
{
    return json{
        {"id", id_},
        {"name", name_},
        {"description", description_},
        {"creator_id", creator_id_},
        {"created_at", created_at_}
    };
}

Room Room::fromJson(const json &j)
{
    Room room;
    
    if (j.contains("id") && j["id"].is_string())
        room.id_ = j["id"];
    
    if (j.contains("name") && j["name"].is_string())
        room.name_ = j["name"];
    
    if (j.contains("description") && j["description"].is_string())
        room.description_ = j["description"];
    
    if (j.contains("creator_id") && j["creator_id"].is_string())
        room.creator_id_ = j["creator_id"];
    
    if (j.contains("created_at") && j["created_at"].is_number_integer())
        room.created_at_ = j["created_at"];
    
    return room;
}
