#pragma once

#include <mysql/mysql.h>

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../utils/logger.hpp"

namespace db {

// MySQL 预处理语句包装类
class MySQLStatement {
 public:
  // 状态枚举
  enum class FetchStatus {
    SUCCESS,  // 成功获取一行数据
    NO_DATA,  // 没有更多数据
    ERROR     // 发生错误
  };

  // 构造函数，需要传入 MySQL 连接和查询语句
  MySQLStatement(MYSQL* mysql, const std::string& query);
  ~MySQLStatement();

  // 禁止拷贝和赋值
  MySQLStatement(const MySQLStatement&) = delete;
  MySQLStatement& operator=(const MySQLStatement&) = delete;

  // 绑定参数
  bool bindString(int index, const std::string& value);
  bool bindInt(int index, int value);
  bool bindLong(int index, long long value);
  bool bindNull(int index);

  // 执行更新操作
  bool executeUpdate();
  // 执行查询操作
  bool executeQuery();

  // 获取下一行数据
  FetchStatus fetch();

  // 获取结果，在fetch成功后调用
  std::string getString(int index);
  int getInt(int index);
  long long getLong(int index);
  bool isNull(int index);

  // 获取影响的行数
  my_ulonglong getAffectedRows();

  // 获取最后插入的ID
  my_ulonglong getLastInsertId();

 private:
  MYSQL* mysql_;
  MYSQL_STMT* stmt_;

  // --- 参数绑定相关成员 ---
  std::vector<MYSQL_BIND> param_binds_;
  // 使用 variant 来存储不同类型的值，避免不必要的动态分配
  std::vector<std::variant<std::string, int, long long>> param_values_;
  std::vector<char> param_nulls_;

  // --- 结果集绑定相关成员 ---
  MYSQL_RES* result_metadata_;
  std::vector<MYSQL_BIND> result_binds_;
  std::vector<std::vector<char>>
      result_string_buffers_;  // 存储字符串类型的结果
  std::vector<std::variant<int, long long>>
      result_native_values_;  // 存储原生整数类型的结果
  std::vector<unsigned long> result_lengths_;
  std::vector<char> result_nulls_;

  void cleanupParams();
  void cleanupResults();
  bool prepareResultMetadata();
};

}  // namespace db
