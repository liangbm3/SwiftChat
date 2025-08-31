# 测试目录的CMake配置文件

# 创建用户测试可执行文件
add_executable(test_user 
    model/test_user.cpp
    ../src/model/user.cpp
)

# 创建消息测试可执行文件
add_executable(test_message
    model/test_message.cpp
    ../src/model/message.cpp
    ../src/model/user.cpp
)

# 创建房间测试可执行文件
add_executable(test_room
    model/test_room.cpp
    ../src/model/room.cpp
)

# 创建日志系统测试可执行文件
add_executable(test_logger 
    utils/test_logger.cpp
    ../src/utils/logger.cpp
)


# 创建连接池测试可执行文件
add_executable(test_connection_pool
    db/test_connection_pool.cpp
    ../src/db/connection_pool.cpp
    ../src/db/database_connection.cpp
    ../src/db/mysql_statement.cpp
    ../src/utils/logger.cpp
)


# 创建MySQL语句测试可执行文件
add_executable(test_mysql_statement
    db/test_mysql_statement.cpp
    ../src/db/mysql_statement.cpp
    ../src/db/connection_pool.cpp
    ../src/db/database_connection.cpp
    ../src/utils/logger.cpp
)

# 创建用户仓库测试可执行文件
add_executable(test_user_repository
    db/test_user_repository.cpp
    ../src/db/respository/user_repository.cpp
    ../src/db/mysql_statement.cpp
    ../src/db/connection_pool.cpp
    ../src/db/database_connection.cpp
    ../src/db/database_initializer.cpp
    ../src/model/user.cpp
    ../src/utils/logger.cpp
)

# 创建房间仓库测试可执行文件
add_executable(test_room_repository
    db/test_room_repository.cpp
    ../src/db/respository/room_repository.cpp
    ../src/db/respository/user_repository.cpp
    ../src/db/mysql_statement.cpp
    ../src/db/connection_pool.cpp
    ../src/db/database_connection.cpp
    ../src/db/database_initializer.cpp
    ../src/model/room.cpp
    ../src/model/user.cpp
    ../src/utils/logger.cpp
)

# 创建消息仓库测试可执行文件
add_executable(test_message_repository
    db/test_message_repository.cpp
    ../src/db/respository/message_repository.cpp
    ../src/db/respository/user_repository.cpp
    ../src/db/respository/room_repository.cpp
    ../src/db/mysql_statement.cpp
    ../src/db/connection_pool.cpp
    ../src/db/database_connection.cpp
    ../src/db/database_initializer.cpp
    ../src/model/message.cpp
    ../src/model/direct_message.cpp
    ../src/model/room.cpp
    ../src/model/user.cpp
    ../src/utils/logger.cpp
)

# 创建数据库管理器测试可执行文件
add_executable(test_database_manager
    db/test_database_manager.cpp
    ../src/db/database_manager.cpp
    ../src/db/respository/message_repository.cpp
    ../src/db/respository/user_repository.cpp
    ../src/db/respository/room_repository.cpp
    ../src/db/mysql_statement.cpp
    ../src/db/connection_pool.cpp
    ../src/db/database_connection.cpp
    ../src/db/database_initializer.cpp
    ../src/model/message.cpp
    ../src/model/direct_message.cpp
    ../src/model/room.cpp
    ../src/model/user.cpp
    ../src/utils/logger.cpp
)

# 创建线程池测试可执行文件
add_executable(test_thread_pool 
    utils/test_thread_pool.cpp
)

# 创建定时器测试可执行文件
add_executable(test_timer 
    utils/test_timer.cpp
    ../src/utils/timer.cpp
)

# 创建HTTP请求测试可执行文件
add_executable(test_http_request 
    http/test_http_request.cpp
    ../src/http/http_request.cpp
    ../src/utils/logger.cpp
)

# 创建HTTP响应测试可执行文件
add_executable(test_http_response 
    http/test_http_response.cpp
    ../src/http/http_response.cpp
)

# 创建HTTP服务器测试可执行文件
add_executable(test_http_server 
    http/test_http_server.cpp
    ../src/http/http_server.cpp
    ../src/http/http_request.cpp
    ../src/http/http_response.cpp
    ../src/utils/logger.cpp
    ../src/http/epoller.cpp
)


# 链接必要的库
target_link_libraries(test_user 
    GTest::gtest
    GTest::gtest_main
    Threads::Threads # 链接线程库  
)

target_link_libraries(test_message
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_link_libraries(test_room
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_link_libraries(test_logger 
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)


target_link_libraries(test_connection_pool
    GTest::gtest
    GTest::gtest_main
    ${MYSQL_LIBRARIES}
    Threads::Threads
)


target_link_libraries(test_mysql_statement
    GTest::gtest
    GTest::gtest_main
    ${MYSQL_LIBRARIES}
    Threads::Threads
)

target_link_libraries(test_user_repository
    GTest::gtest
    GTest::gtest_main
    ${MYSQL_LIBRARIES}
    Threads::Threads
)

target_link_libraries(test_room_repository
    GTest::gtest
    GTest::gtest_main
    ${MYSQL_LIBRARIES}
    Threads::Threads
)

target_link_libraries(test_message_repository
    GTest::gtest
    GTest::gtest_main
    ${MYSQL_LIBRARIES}
    Threads::Threads
)

target_link_libraries(test_database_manager
    GTest::gtest
    GTest::gtest_main
    ${MYSQL_LIBRARIES}
    Threads::Threads
)

target_link_libraries(test_thread_pool
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_link_libraries(test_timer
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_link_libraries(test_http_request
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_link_libraries(test_http_response
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_link_libraries(test_http_server
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)



# 设置测试可执行文件的输出目录
set_target_properties(test_user PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_message PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_room PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_logger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_connection_pool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)


set_target_properties(test_mysql_statement PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_user_repository PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_database_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)


set_target_properties(test_thread_pool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_timer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_http_request PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_http_response PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_http_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)


# set_target_properties(test_auth_utils PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
# )

# 包含必要的头文件目录
target_include_directories(test_user PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

target_include_directories(test_message PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

target_include_directories(test_room PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

target_include_directories(test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

target_include_directories(test_connection_pool PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${MYSQL_INCLUDE_DIRS}
)


target_include_directories(test_mysql_statement PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${MYSQL_INCLUDE_DIRS}
)

target_include_directories(test_user_repository PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/single_include
    ${MYSQL_INCLUDE_DIRS}
)

target_include_directories(test_room_repository PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/single_include
    ${MYSQL_INCLUDE_DIRS}
)

target_include_directories(test_message_repository PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/single_include
    ${MYSQL_INCLUDE_DIRS}
)

target_include_directories(test_database_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/single_include
    ${MYSQL_INCLUDE_DIRS}
)

target_include_directories(test_thread_pool PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    
)

target_include_directories(test_timer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    
)

target_include_directories(test_http_request PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    
)

target_include_directories(test_http_response PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    
)

target_include_directories(test_http_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    
)


# target_include_directories(test_auth_utils PRIVATE
#     ${CMAKE_SOURCE_DIR}/src
#     ${CMAKE_SOURCE_DIR}/third_party
#     ${CMAKE_SOURCE_DIR}/third_party/jwt-cpp/include
#     ${OPENSSL_INCLUDE_DIR}
# )

# 如果使用CTest，可以添加测试
add_test(NAME UserTests COMMAND test_user)
add_test(NAME MessageTests COMMAND test_message)
add_test(NAME RoomTests COMMAND test_room)
add_test(NAME LoggerTests COMMAND test_logger)
add_test(NAME DatabaseManagerTests COMMAND test_database_manager)
add_test(NAME ConnectionPoolTests COMMAND test_connection_pool)
add_test(NAME DatabaseConnectionTests COMMAND test_database_connection)
add_test(NAME DatabaseInitializerTests COMMAND test_database_initializer)
add_test(NAME MySQLStatementTests COMMAND test_mysql_statement)
add_test(NAME UserRepositoryTests COMMAND test_user_repository)
add_test(NAME RoomRepositoryTests COMMAND test_room_repository)
add_test(NAME MessageRepositoryTests COMMAND test_message_repository)
add_test(NAME ThreadPoolTests COMMAND test_thread_pool)
add_test(NAME TimerTests COMMAND test_timer)
add_test(NAME HttpRequestTests COMMAND test_http_request)
add_test(NAME HttpResponseTests COMMAND test_http_response)
add_test(NAME HttpServerTests COMMAND test_http_server)
