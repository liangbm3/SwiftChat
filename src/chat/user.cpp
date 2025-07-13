#include "user.hpp"

json User::toJson() const
{
    return json{
        {"username", username},
        {"password", password},
        {"is_online", is_online}};
}

User User::fromJson(const json &j)
{
    User user;
    user.username = j.at("username").get<std::string>();
    user.password = j.at("password").get<std::string>();
    user.is_online = j.at("is_online").get<bool>();
    return user;
}
