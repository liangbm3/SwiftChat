#pragma once

#include <string>

#include "connection_pool.hpp"

namespace db {
namespace initializer {
// 初始化所有表和索引
bool initializeSchema(ConnectionPool& pool);
}  // namespace initializer
}  // namespace db