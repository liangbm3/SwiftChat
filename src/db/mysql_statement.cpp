#include "mysql_statement.hpp"

#include <cstring>

namespace db {

MySQLStatement::MySQLStatement(MYSQL *mysql, const std::string &query)
    : mysql_(mysql), stmt_(nullptr), result_metadata_(nullptr) {
  if (!mysql_) {
    LOG_ERROR << "MySQL connection is null";
    return;
  }

  stmt_ = mysql_stmt_init(mysql_);
  if (!stmt_) {
    LOG_ERROR << "mysql_stmt_init failed: " << mysql_error(mysql_);
    return;
  }

  if (mysql_stmt_prepare(stmt_, query.c_str(), query.length())) {
    LOG_ERROR << "mysql_stmt_prepare failed for query [" << query
              << "]: " << mysql_stmt_error(stmt_);
    mysql_stmt_close(stmt_);
    stmt_ = nullptr;
    return;
  }

  // 初始化参数绑定结构
  unsigned long param_count = mysql_stmt_param_count(stmt_);
  if (param_count > 0) {
    param_binds_.resize(param_count);
    param_values_.resize(param_count);
    param_nulls_.resize(param_count, 0);  // 默认非空
    memset(param_binds_.data(), 0, sizeof(MYSQL_BIND) * param_count);
  }
}

MySQLStatement::~MySQLStatement() {
  cleanupParams();
  cleanupResults();
  if (stmt_) {
    mysql_stmt_close(stmt_);
    stmt_ = nullptr;
  }
}

void MySQLStatement::cleanupParams() {
  param_binds_.clear();
  param_values_.clear();
  param_nulls_.clear();
}

void MySQLStatement::cleanupResults() {
  if (result_metadata_) {
    mysql_free_result(result_metadata_);
    result_metadata_ = nullptr;
  }
  result_binds_.clear();
  result_string_buffers_.clear();
  result_native_values_.clear();
  result_lengths_.clear();
  result_nulls_.clear();
}

// --- 参数绑定实现 ---

bool MySQLStatement::bindString(int index, const std::string &value) {
  if (!stmt_ || index >= param_binds_.size()) return false;

  param_values_[index] = value;
  auto &str_val = std::get<std::string>(param_values_[index]);

  param_binds_[index].buffer_type = MYSQL_TYPE_STRING;
  param_binds_[index].buffer = (char *)str_val.c_str();
  param_binds_[index].buffer_length = str_val.length();
  param_binds_[index].is_null = reinterpret_cast<bool *>(&param_nulls_[index]);
  param_nulls_[index] = 0;

  return true;
}

bool MySQLStatement::bindInt(int index, int value) {
  if (!stmt_ || index >= param_binds_.size()) return false;

  param_values_[index] = value;

  param_binds_[index].buffer_type = MYSQL_TYPE_LONG;
  param_binds_[index].buffer = (char *)&std::get<int>(param_values_[index]);
  param_binds_[index].is_null = reinterpret_cast<bool *>(&param_nulls_[index]);
  param_nulls_[index] = 0;

  return true;
}

bool MySQLStatement::bindLong(int index, long long value) {
  if (!stmt_ || index >= param_binds_.size()) return false;

  param_values_[index] = value;

  param_binds_[index].buffer_type = MYSQL_TYPE_LONGLONG;
  param_binds_[index].buffer =
      (char *)&std::get<long long>(param_values_[index]);
  param_binds_[index].is_null = reinterpret_cast<bool *>(&param_nulls_[index]);
  param_nulls_[index] = 0;

  return true;
}

bool MySQLStatement::bindNull(int index) {
  if (!stmt_ || index >= param_binds_.size()) return false;

  param_binds_[index].buffer_type = MYSQL_TYPE_NULL;
  param_nulls_[index] = 1;
  param_binds_[index].is_null = reinterpret_cast<bool *>(&param_nulls_[index]);

  return true;
}

// --- 执行 ---

bool MySQLStatement::executeUpdate() {
  if (!stmt_) return false;

  if (!param_binds_.empty()) {
    if (mysql_stmt_bind_param(stmt_, param_binds_.data())) {
      LOG_ERROR << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt_);
      return false;
    }
  }

  if (mysql_stmt_execute(stmt_)) {
    LOG_ERROR << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt_);
    return false;
  }

  return true;
}

bool MySQLStatement::executeQuery() {
  if (!executeUpdate()) {
    return false;
  }
  return prepareResultMetadata();
}

// --- 结果处理 ---

bool MySQLStatement::prepareResultMetadata() {
  cleanupResults();  // 清理上一次查询的结果
  result_metadata_ = mysql_stmt_result_metadata(stmt_);
  if (!result_metadata_) {
    return mysql_stmt_field_count(stmt_) == 0;
  }

  int field_count = mysql_num_fields(result_metadata_);
  if (field_count > 0) {
    result_binds_.resize(field_count);
    result_lengths_.resize(field_count);
    result_nulls_.resize(field_count);
    result_string_buffers_.resize(field_count);
    result_native_values_.resize(field_count);
    memset(result_binds_.data(), 0, sizeof(MYSQL_BIND) * field_count);

    for (int i = 0; i < field_count; ++i) {
      MYSQL_FIELD *field = mysql_fetch_field_direct(result_metadata_, i);
      auto &bind = result_binds_[i];

      bind.length = &result_lengths_[i];
      bind.is_null = reinterpret_cast<bool *>(&result_nulls_[i]);

      // 根据字段选择绑定方式
      switch (field->type) {
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
          bind.buffer_type = MYSQL_TYPE_LONG;
          result_native_values_[i] = static_cast<int>(0);
          bind.buffer = &std::get<int>(result_native_values_[i]);
          bind.buffer_length = sizeof(int);
          break;
        case MYSQL_TYPE_LONGLONG:
          bind.buffer_type = MYSQL_TYPE_LONGLONG;
          result_native_values_[i] = static_cast<long long>(0);
          bind.buffer = &std::get<long long>(result_native_values_[i]);
          bind.buffer_length = sizeof(long long);
          break;
        default:
          bind.buffer_type = MYSQL_TYPE_STRING;
          // 为字符串分配一个合理大小的缓冲区
          result_string_buffers_[i].resize(field->length +
                                           1);  // +1 for null-terminator
          bind.buffer = result_string_buffers_[i].data();
          bind.buffer_length = result_string_buffers_[i].size();
          break;
      }
    }

    if (mysql_stmt_bind_result(stmt_, result_binds_.data())) {
      LOG_ERROR << "mysql_stmt_bind_result failed: " << mysql_stmt_error(stmt_);
      return false;
    }
  }

  // 将结果集缓存到客户端，这样可以获取行数等信息
  if (mysql_stmt_store_result(stmt_)) {
    LOG_ERROR << "mysql_stmt_store_result failed: " << mysql_stmt_error(stmt_);
    return false;
  }

  return true;
}

MySQLStatement::FetchStatus MySQLStatement::fetch() {
  if (!stmt_) return FetchStatus::ERROR;

  int result = mysql_stmt_fetch(stmt_);
  if (result == 0) {
    return FetchStatus::SUCCESS;
  } else if (result == MYSQL_NO_DATA) {
    return FetchStatus::NO_DATA;
  } else {
    LOG_ERROR << "mysql_stmt_fetch failed: " << mysql_stmt_error(stmt_);
    return FetchStatus::ERROR;
  }
}

bool MySQLStatement::isNull(int index) {
  if (index >= result_nulls_.size()) return true;
  return result_nulls_[index];
}

std::string MySQLStatement::getString(int index) {
  if (isNull(index) || index >= result_string_buffers_.size()) return "";
  if (result_binds_[index].buffer_type == MYSQL_TYPE_STRING) {
    return std::string(result_string_buffers_[index].data(),
                       result_lengths_[index]);
  } else if (result_binds_[index].buffer_type == MYSQL_TYPE_LONG) {
    return std::to_string(std::get<int>(result_native_values_[index]));
  } else if (result_binds_[index].buffer_type == MYSQL_TYPE_LONGLONG) {
    return std::to_string(std::get<long long>(result_native_values_[index]));
  } else {
    return "";
  }
}

int MySQLStatement::getInt(int index) {
  if (isNull(index) || index >= result_native_values_.size()) return 0;
  // 检查variant类型
  if (std::holds_alternative<int>(result_native_values_[index])) {
    return std::get<int>(result_native_values_[index]);
  } else if (std::holds_alternative<long long>(result_native_values_[index])) {
    return static_cast<int>(std::get<long long>(result_native_values_[index]));
  } else {
    LOG_ERROR << "Unhandled type in variant on getInt for column " << index;
    return 0;
  }
}

long long MySQLStatement::getLong(int index) {
  if (isNull(index) || index >= result_native_values_.size()) return 0;
  try {
    if (std::holds_alternative<long long>(result_native_values_[index])) {
      return std::get<long long>(result_native_values_[index]);
    } else if (std::holds_alternative<int>(result_native_values_[index])) {
      return static_cast<long long>(
          std::get<int>(result_native_values_[index]));
    } else {
      return 0;
    }
  } catch (const std::bad_variant_access &) {
    LOG_ERROR << "Type mismatch on getLong for column " << index;
    return 0;
  }
}

my_ulonglong MySQLStatement::getAffectedRows() {
  if (!stmt_) return 0;
  return mysql_stmt_affected_rows(stmt_);
}

my_ulonglong MySQLStatement::getLastInsertId() {
  if (!mysql_) return 0;
  return mysql_insert_id(mysql_);
}

}  // namespace db