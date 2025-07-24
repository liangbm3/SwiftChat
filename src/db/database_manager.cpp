#include "database_manager.hpp"

DatabaseManager::DatabaseManager(const std::string &db_path):db_conn_(std::make_unique<DatabaseConnection>(db_path))
{
    if (db_conn_->isConnected())
    {
        // 创建各个仓库
        user_repo_ = std::make_unique<UserRepository>(db_conn_.get());
        room_repo_ = std::make_unique<RoomRepository>(db_conn_.get());
        message_repo_ = std::make_unique<MessageRepository>(db_conn_.get());
    }
}

bool DatabaseManager::isConnected() const
{
    return db_conn_ && db_conn_->isConnected();
}

// 用户操作代理
bool DatabaseManager::createUser(const std::string &username, const std::string &password_hash)
{
    return user_repo_ ? user_repo_->createUser(username, password_hash) : false;
}

bool DatabaseManager::validateUser(const std::string &username, const std::string &password_hash)
{
    return user_repo_ ? user_repo_->validateUser(username, password_hash) : false;
}

bool DatabaseManager::userExists(const std::string &user_id)
{
    return user_repo_ ? user_repo_->userExists(user_id) : false;
}

std::vector<User> DatabaseManager::getAllUsers()
{
    return user_repo_ ? user_repo_->getAllUsers() : std::vector<User>();
}


std::optional<User> DatabaseManager::getUserByUsername(const std::string &username) const
{
    return user_repo_ ? user_repo_->getUserByUsername(username) : std::nullopt;
}

std::optional<User> DatabaseManager::getUserById(const std::string &user_id) const
{
    return user_repo_ ? user_repo_->getUserById(user_id) : std::nullopt;
}

std::string DatabaseManager::generateUserId()
{
    return user_repo_ ? user_repo_->generateUserId() : "";
}

// 房间操作代理
std::optional<Room> DatabaseManager::createRoom(const std::string &name, const std::string &description, const std::string &creator_id)
{
    return room_repo_ ? room_repo_->createRoom(name, description, creator_id) : std::nullopt;
}

bool DatabaseManager::deleteRoom(const std::string &room_id)
{
    return room_repo_ ? room_repo_->deleteRoom(room_id) : false;
}

bool DatabaseManager::roomExists(const std::string &room_id)
{
    return room_repo_ ? room_repo_->roomExists(room_id) : false;
}

std::vector<std::string> DatabaseManager::getRooms()
{
    return room_repo_ ? room_repo_->getRooms() : std::vector<std::string>();
}

std::optional<Room> DatabaseManager::getRoomById(const std::string &room_id) const
{
    return room_repo_ ? room_repo_->getRoomById(room_id) : std::nullopt;
}

std::optional<std::string> DatabaseManager::getRoomIdByName(const std::string &room_name) const
{
    return room_repo_ ? room_repo_->getRoomIdByName(room_name) : std::nullopt;
}

std::string DatabaseManager::generateRoomId()
{
    return room_repo_ ? room_repo_->generateRoomId() : "";
}

bool DatabaseManager::updateRoom(const std::string &room_id, const std::string &name, const std::string &description)
{
    return room_repo_ ? room_repo_->updateRoom(room_id, name, description) : false;
}

bool DatabaseManager::isRoomCreator(const std::string &room_id, const std::string &user_id)
{
    return room_repo_ ? room_repo_->isRoomCreator(room_id, user_id) : false;
}

// 房间成员操作代理
std::vector<nlohmann::json> DatabaseManager::getRoomMembers(const std::string &room_id) const
{
    return room_repo_ ? room_repo_->getRoomMembers(room_id) : std::vector<nlohmann::json>();
}

std::vector<Room> DatabaseManager::getUserJoinedRooms(const std::string &user_id) const
{
    return room_repo_ ? room_repo_->getUserJoinedRooms(user_id) : std::vector<Room>();
}

bool DatabaseManager::addRoomMember(const std::string &room_id, const std::string &user_id)
{
    return room_repo_ ? room_repo_->addRoomMember(room_id, user_id) : false;
}

bool DatabaseManager::removeRoomMember(const std::string &room_id, const std::string &user_id)
{
    return room_repo_ ? room_repo_->removeRoomMember(room_id, user_id) : false;
}

// 消息操作代理
bool DatabaseManager::saveMessage(const std::string &room_id, const std::string &user_id,
                                   const std::string &content, int64_t timestamp)
{
    return message_repo_ ? message_repo_->saveMessage(room_id, user_id, content, timestamp) : false;
}

std::vector<Message> DatabaseManager::getMessages(const std::string &room_id, int limit,
                                                  int64_t before_timestamp)
{
    return message_repo_ ? message_repo_->getMessages(room_id, limit, before_timestamp) : std::vector<Message>();
}

std::optional<Message> DatabaseManager::getMessageById(int64_t message_id)
{
    return message_repo_ ? message_repo_->getMessageById(message_id) : std::nullopt;
}

std::vector<Room> DatabaseManager::getAllRooms()
{
    return room_repo_ ? room_repo_->getAllRooms() : std::vector<Room>();
}
