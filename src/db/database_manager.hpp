#pragma once

#include <string>
#include <memory>
#include "database_connection.hpp"
#include "user_repository.hpp"
#include "room_repository.hpp"
#include "message_repository.hpp"

// 重构后的数据库管理类 - 作为各个仓库的组合
class DatabaseManager
{
public:
    explicit DatabaseManager(const std::string &db_path);
    ~DatabaseManager() = default;

    // 检查数据库连接状态
    bool isConnected() const;

    // 用户操作代理
    bool createUser(const std::string &username, const std::string &password_hash);
    bool validateUser(const std::string &username, const std::string &password_hash);
    bool userExists(const std::string &user_id);
    bool setUserOnlineStatus(const std::string &user_id, bool is_online);
    bool updateUserLastActiveTime(const std::string &user_id);
    bool checkAndUpdateInactiveUsers(int64_t timeout_ms);
    std::vector<User> getAllUsers();
    std::optional<User> getUserById(const std::string &user_id) const;
    std::optional<User> getUserByUsername(const std::string &username) const;
    std::string generateUserId();

    // 房间操作代理
    std::optional<nlohmann::json> createRoom(const std::string &name, const std::string &creator_id);
    bool deleteRoom(const std::string &room_id);
    bool roomExists(const std::string &room_id);
    std::vector<std::string> getRooms();
    std::optional<nlohmann::json> getRoomById(const std::string &room_id) const;
    std::string generateRoomId();
    bool updateRoom(const std::string &room_id, const std::string &name, const std::string &description);
    bool isRoomCreator(const std::string &room_id, const std::string &user_id);

    // 房间成员操作代理
    std::vector<nlohmann::json> getRoomMembers(const std::string &room_id) const;
    bool addRoomMember(const std::string &room_id, const std::string &user_id);
    bool removeRoomMember(const std::string &room_id, const std::string &user_id);

    // 消息操作代理
    bool saveMessage(const std::string &room_id, const std::string &user_id,
                     const std::string &content, int64_t timestamp);
    std::vector<nlohmann::json> getMessages(const std::string &room_id, int limit = 50,
                                            int64_t before_timestamp = 0);

    // 获取各个仓库的直接访问（如果需要更复杂的操作）
    UserRepository* getUserRepository() { return user_repo_.get(); }
    RoomRepository* getRoomRepository() { return room_repo_.get(); }
    MessageRepository* getMessageRepository() { return message_repo_.get(); }

private:
    std::unique_ptr<DatabaseConnection> db_conn_;// 数据库连接
    std::unique_ptr<UserRepository> user_repo_;// 用户仓库
    std::unique_ptr<RoomRepository> room_repo_;// 房间仓库
    std::unique_ptr<MessageRepository> message_repo_;// 消息仓库
};
