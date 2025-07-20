# 测试目录的CMake配置文件

# 创建用户测试可执行文件
add_executable(test_user 
    chat/test_user.cpp
    ../src/chat/user.cpp
)

# 创建消息测试可执行文件
add_executable(test_message
    chat/test_message.cpp
    ../src/chat/message.cpp
    ../src/chat/user.cpp
)

# 创建房间测试可执行文件
add_executable(test_room
    chat/test_room.cpp
    ../src/chat/room.cpp
)

# 创建日志系统测试可执行文件
add_executable(test_logger 
    utils/test_logger.cpp
    ../src/utils/logger.cpp
)


# 创建数据库管理器测试可执行文件
add_executable(test_database_manager
    db/test_database_manager.cpp
    ../src/db/database_manager.cpp
    ../src/db/database_connection.cpp
    ../src/db/user_repository.cpp
    ../src/db/room_repository.cpp
    ../src/db/message_repository.cpp
    ../src/chat/user.cpp
    ../src/chat/room.cpp
    ../src/chat/message.cpp
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
)

# 创建WebSocket基础测试可执行文件
add_executable(test_websocket_basic 
    websocket/test_websocket_basic.cpp
    ../src/websocket/websocket_server.cpp
    ../src/utils/logger.cpp
    ../src/utils/jwt_utils.cpp
    ../src/http/http_request.cpp
    ../src/db/database_manager.cpp
    ../src/db/database_connection.cpp
    ../src/db/user_repository.cpp
    ../src/db/room_repository.cpp
    ../src/db/message_repository.cpp
    ../src/chat/user.cpp
    ../src/chat/room.cpp
    ../src/chat/message.cpp
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

target_link_libraries(test_database_manager 
    GTest::gtest
    GTest::gtest_main
    sqlite3
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

target_link_libraries(test_websocket_basic
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
    ${OPENSSL_LIBRARIES}
    sqlite3
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

set_target_properties(test_websocket_basic PROPERTIES
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

target_include_directories(test_database_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    
)

target_include_directories(test_database_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
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

target_include_directories(test_websocket_basic PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/websocketpp
)

# target_include_directories(test_auth_utils PRIVATE
#     ${CMAKE_SOURCE_DIR}/src
#     ${CMAKE_SOURCE_DIR}/third_party
#     ${CMAKE_SOURCE_DIR}/third_party/jwt-cpp/include
#     ${OPENSSL_INCLUDE_DIR}
# )

# 如果使用CTest，可以添加测试
enable_testing()
add_test(NAME UserTests COMMAND test_user)
add_test(NAME MessageTests COMMAND test_message)
add_test(NAME RoomTests COMMAND test_room)
add_test(NAME LoggerTests COMMAND test_logger)
add_test(NAME DatabaseManagerTests COMMAND test_database_manager)
add_test(NAME ThreadPoolTests COMMAND test_thread_pool)
add_test(NAME TimerTests COMMAND test_timer)
add_test(NAME HttpRequestTests COMMAND test_http_request)
add_test(NAME HttpResponseTests COMMAND test_http_response)
add_test(NAME HttpServerTests COMMAND test_http_server)
add_test(NAME WebSocketBasicTests COMMAND test_websocket_basic)
add_test(NAME AuthUtilsTests COMMAND test_auth_utils)
