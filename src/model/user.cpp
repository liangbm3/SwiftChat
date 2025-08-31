#include "user.hpp"

json User::toJson() const {
  return json{{"id", id_}, 
              {"username", username_}, 
              {"password", password_},
              {"status", status_},
              {"last_seen", last_seen_}};
}

User User::fromJson(const json &j) {
  User user;
  user.id_ = j.value("id", 0);
  user.username_ = j.at("username").get<std::string>();
  user.password_ = j.at("password").get<std::string>();
  user.status_ = j.value("status", 0);
  user.last_seen_ = j.value("last_seen", std::string(""));
  return user;
}
