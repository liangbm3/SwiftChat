# 源代码目录的CMake配置文件

# 添加可执行文件
add_executable(SwiftChat
    main.cpp
    http/http_server.cpp
    http/http_request.cpp
    http/http_response.cpp
    utils/logger.cpp
    utils/thread_pool.cpp
    utils/timer.cpp
    service/auth_service.cpp
    service/room_service.cpp
    service/message_service.cpp
    middleware/auth_middleware.cpp
    websocket/websocket_server.cpp
    db/database_manager.cpp
    db/database_connection.cpp
    db/user_repository.cpp
    db/room_repository.cpp
    db/message_repository.cpp
    chat/user.cpp
)

# 设置包含目录
target_include_directories(SwiftChat PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/third_party/jwt-cpp/include
)

# 链接必要的库
target_link_libraries(SwiftChat
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
    sqlite3
)

# 设置可执行文件的输出目录
set_target_properties(SwiftChat PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
