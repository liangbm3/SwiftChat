#include "user.hpp"

json User::toJson() const
{
    return json{
        {"id", id_},
        {"username", username_},
        {"password", password_},
        {"is_online", is_online_},
        {"last_active_time", last_active_time_}};
}

User User::fromJson(const json &j)
{
    User user;
    user.id_ = j.value("id", "");
    user.username_ = j.at("username").get<std::string>();
    user.password_ = j.at("password").get<std::string>();
    user.is_online_ = j.at("is_online").get<bool>();
    user.last_active_time_ = j.value("last_active_time", 0);
    return user;
}
