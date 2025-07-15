#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "database_connection.hpp"

// 消息数据访问类
class MessageRepository
{
public:
    explicit MessageRepository(DatabaseConnection *db_conn);

    // 消息操作
    bool saveMessage(const std::string &room_name, const std::string &username,
                     const std::string &content, int64_t timestamp);//保存消息
    std::vector<nlohmann::json> getMessages(const std::string &room_name, int64_t since = 0);// 获取消息
    bool saveMessageById(const std::string &room_id, const std::string &user_id,
                         const std::string &content, int64_t timestamp);// 根据ID保存消息
    std::vector<nlohmann::json> getMessagesById(const std::string &room_id, int limit = 50,
                                                int64_t before_timestamp = 0);// 根据ID获取消息

private:
    DatabaseConnection *db_conn_;
};
