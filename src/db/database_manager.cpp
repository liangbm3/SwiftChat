#include "database_manager.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include "../../third_party/nlohmann/single_include/nlohmann/json.hpp"

DatabaseManager::DatabaseManager(const db::MySQLConfig &config, size_t pool_size) {
  // 初始化连接池单例
  db::ConnectionPool::getInstance().init(config, pool_size);
  
  // 创建各个仓库
  user_repo_ = std::make_unique<db::UserRepository>(db::ConnectionPool::getInstance());
  room_repo_ = std::make_unique<db::RoomRepository>(db::ConnectionPool::getInstance());
  message_repo_ = std::make_unique<db::MessageRepository>(db::ConnectionPool::getInstance());
}

bool DatabaseManager::isConnected() const {
  return user_repo_ != nullptr && room_repo_ != nullptr && message_repo_ != nullptr;
}

// 用户操作代理
std::optional<int64_t> DatabaseManager::createUser(const std::string &username,
                                                    const std::string &password_hash) {
  return user_repo_->createUser(username, password_hash);
}

bool DatabaseManager::deleteUser(int64_t user_id) {
  return user_repo_->deleteUser(user_id);
}

bool DatabaseManager::validateUser(const std::string &username,
                                   const std::string &password_hash) {
  return user_repo_->validateUser(username, password_hash);
}

bool DatabaseManager::userExists(int64_t user_id) {
  return user_repo_->userExists(user_id);
}

bool DatabaseManager::userExists(const std::string &username) {
  return user_repo_->userExists(username);
}

bool DatabaseManager::updateUserStatus(int64_t user_id, int status) {
  return user_repo_->updateUserStatus(user_id, status);
}

bool DatabaseManager::updateLastSeen(int64_t user_id) {
  return user_repo_->updateLastSeen(user_id);
}

std::vector<User> DatabaseManager::getAllUsers() const {
  return user_repo_->getAllUsers();
}

std::optional<User> DatabaseManager::getUser(int64_t user_id) const {
  return user_repo_->getUser(user_id);
}

std::optional<User> DatabaseManager::getUser(const std::string &username) const {
  return user_repo_->getUser(username);
}

std::vector<User> DatabaseManager::getOnlineUsers() const {
  return user_repo_->getOnlineUsers();
}

// 房间操作代理
std::optional<int64_t> DatabaseManager::createRoom(const std::string &name, int64_t creator_id) {
  return room_repo_->createRoom(name, creator_id);
}

bool DatabaseManager::deleteRoom(int64_t room_id) {
  return room_repo_->deleteRoom(room_id);
}

bool DatabaseManager::roomExists(int64_t room_id) const {
  return room_repo_->roomExists(room_id);
}

bool DatabaseManager::updateRoom(int64_t room_id, const std::string &name,
                                 const std::string &description) {
  return room_repo_->updateRoom(room_id, name, description);
}

std::vector<std::string> DatabaseManager::getAllRoomNames() const {
  return room_repo_->getAllRoomNames();
}

std::vector<Room> DatabaseManager::getAllRooms() const {
  return room_repo_->getAllRooms();
}

std::optional<Room> DatabaseManager::getRoom(int64_t room_id) const {
  return room_repo_->getRoom(room_id);
}

std::optional<Room> DatabaseManager::getRoom(const std::string &room_name) const {
  return room_repo_->getRoom(room_name);
}

std::optional<int64_t> DatabaseManager::getRoomIdByName(const std::string &room_name) const {
  return room_repo_->getRoomIdByName(room_name);
}

bool DatabaseManager::isRoomCreator(int64_t user_id, int64_t room_id) const {
  return room_repo_->isRoomCreator(user_id, room_id);
}

// 房间成员操作代理
std::vector<User> DatabaseManager::getRoomMembers(int64_t room_id) const {
  return room_repo_->getRoomMembers(room_id);
}

bool DatabaseManager::addRoomMember(int64_t room_id, int64_t user_id) {
  return room_repo_->addRoomMember(room_id, user_id);
}

bool DatabaseManager::removeRoomMember(int64_t room_id, int64_t user_id) {
  return room_repo_->removeRoomMember(room_id, user_id);
}

// 房间消息操作代理
std::optional<int64_t> DatabaseManager::saveMessage(int64_t room_id, int64_t sender_id,
                                                     const std::string &content) {
  return message_repo_->saveMessage(room_id, sender_id, content);
}

bool DatabaseManager::deleteMessage(int64_t message_id) {
  return message_repo_->deleteMessage(message_id);
}

bool DatabaseManager::messageExists(int64_t message_id) const {
  return message_repo_->messageExists(message_id);
}

std::vector<Message> DatabaseManager::getRoomMessages(int64_t room_id, int limit,
                                                       int offset) const {
  return message_repo_->getRoomMessages(room_id, limit, offset);
}

std::vector<Message> DatabaseManager::getRoomMessagesAfter(int64_t room_id,
                                                            const std::string &created_at) const {
  return message_repo_->getRoomMessagesAfter(room_id, created_at);
}

std::optional<Message> DatabaseManager::getMessage(int64_t message_id) const {
  return message_repo_->getMessage(message_id);
}

int64_t DatabaseManager::getRoomMessageCount(int64_t room_id) const {
  return message_repo_->getRoomMessageCount(room_id);
}

// 私聊消息操作代理
std::optional<int64_t> DatabaseManager::saveDirectMessage(int64_t sender_id, int64_t receiver_id,
                                                           const std::string &content) {
  return message_repo_->saveDirectMessage(sender_id, receiver_id, content);
}

bool DatabaseManager::deleteDirectMessage(int64_t message_id) {
  return message_repo_->deleteDirectMessage(message_id);
}

bool DatabaseManager::directMessageExists(int64_t message_id) const {
  return message_repo_->directMessageExists(message_id);
}

std::vector<DirectMessage> DatabaseManager::getDirectMessages(int64_t user1_id, int64_t user2_id,
                                                               int limit, int offset) const {
  return message_repo_->getDirectMessages(user1_id, user2_id, limit, offset);
}

std::vector<DirectMessage> DatabaseManager::getDirectMessagesAfter(int64_t user1_id, int64_t user2_id,
                                                                    const std::string &created_at) const {
  return message_repo_->getDirectMessagesAfter(user1_id, user2_id, created_at);
}

std::optional<DirectMessage> DatabaseManager::getDirectMessage(int64_t message_id) const {
  return message_repo_->getDirectMessage(message_id);
}

int64_t DatabaseManager::getDirectMessageCount(int64_t user1_id, int64_t user2_id) const {
  return message_repo_->getDirectMessageCount(user1_id, user2_id);
}

std::vector<int64_t> DatabaseManager::getConversationPartners(int64_t user_id) const {
  return message_repo_->getConversationPartners(user_id);
}
