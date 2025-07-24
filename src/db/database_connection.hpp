#pragma once

#include <string>
#include <sqlite3.h>
#include <mutex>
#include "../utils/logger.hpp"

// 数据库连接管理基类
class DatabaseConnection
{
public:
    explicit DatabaseConnection(const std::string &db_path);
    virtual ~DatabaseConnection();//后面需要通过基类指针来删除一个派生类，所以需要将基类的析构函数声明为虚函数

    bool isConnected() const { return db_ != nullptr; }
    sqlite3* getDb() const { return db_; }

    // 互斥锁访问接口
    std::recursive_mutex& getMutex() { return mutex_; }

protected:
    bool executeQuery(const std::string &query);
    bool initializeTables();
    bool enableForeignKeys();

    sqlite3 *db_;                // 指向sqlite3 结构体的指针
    std::string db_path_;        // 数据库路径
    mutable std::recursive_mutex mutex_; // 递归互斥锁

private:
    bool createUsersTable();
    bool createRoomsTable();
    bool createRoomMembersTable();
    bool createMessagesTable();
    bool createIndexes();
};
