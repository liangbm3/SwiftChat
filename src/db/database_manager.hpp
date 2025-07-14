#pragma once

#include <string>
#include <memory>
#include <vector>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include "chat/user.hpp"

// 数据库管理类
class DatabaseManager
{
public:
    DatabaseManager(const std::string &db_path);
    ~DatabaseManager();

    // 用户操作
    bool createUser(const std::string &username, const std::string &password_hash);//创建用户
    bool validateUser(const std::string &username, const std::string &password_hash);//验证用户
    bool userExists(const std::string &username);//用户是否存在
    bool setUserOnlineStatus(const std::string &username, bool is_online);//设置用户在线状态
    bool updateUserLastActiveTime(const std::string &username);//更新用户最后活跃时间
    bool checkAndUpdateInactiveUsers(int64_t timeout_ms);//检查并更新不活跃用户
    std::vector<User> getAllUsers();//获取所有用户
    // 房间操作
    bool createRoom(const std::string &name, const std::string &creator);//创建房间
    bool deleteRoom(const std::string &name);//删除房间
    bool roomExists(const std::string &name);//房间是否存在
    std::vector<std::string> getRooms();//获取所有房间

    // 房间成员操作
    bool addRoomMember(const std::string &room_name, const std::string &username);//添加房间成员
    bool removeRoomMember(const std::string &room_name, const std::string &username);//移除房间成员
    std::vector<std::string> getRoomMembers(const std::string &room_name);//获取房间成员

    // 消息操作
    bool saveMessage(const std::string &room_name, const std::string &username,
                     const std::string &content, int64_t timestamp);//保存消息
    std::vector<nlohmann::json> getMessages(const std::string &room_name, int64_t since = 0);//获取消息

private:
    bool initializeTables();
    bool executeQuery(const std::string &query);

    sqlite3 *db_;                // 指向sqlite3 结构体的指针
    std::string db_path_;        // 数据库路径
    std::recursive_mutex mutex_; // 递归互斥锁
};
