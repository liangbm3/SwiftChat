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
    return Room(
        j.at("id").get<std::string>(),
        j.at("name").get<std::string>(),
        j.value("description", ""), // 描述可能为空，使用默认值
        j.at("creator_id").get<std::string>(),
        j.at("created_at").get<int64_t>()
    );
}
