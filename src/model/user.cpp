#include "user.hpp"

json User::toJson() const
{
    return json{
        {"id", id_},
        {"username", username_},
        {"password", password_}};
}

User User::fromJson(const json &j)
{
    User user;
    user.id_ = j.value("id", "");
    user.username_ = j.at("username").get<std::string>();
    user.password_ = j.at("password").get<std::string>();
    return user;
}
