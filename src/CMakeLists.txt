# 源代码目录的CMake配置文件

# 添加可执行文件
add_executable(SwiftChat
    main.cpp
    model/user.cpp
    model/room.cpp
    model/message.cpp
    http/http_server.cpp
    http/http_request.cpp
    http/http_response.cpp
    http/epoller.cpp
    utils/logger.cpp
    utils/thread_pool.cpp
    utils/timer.cpp
    utils/jwt_utils.cpp
    service/auth_service.cpp
    service/room_service.cpp
    service/message_service.cpp
    service/user_service.cpp
    service/server_service.cpp
    middleware/auth_middleware.cpp
    websocket/websocket_server.cpp
    db/database_manager.cpp
    db/database_connection.cpp
    db/user_repository.cpp
    db/room_repository.cpp
    db/message_repository.cpp
)

# 设置包含目录
target_include_directories(SwiftChat PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/single_include
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

# 构建后自动安装到 bin 目录
add_custom_command(TARGET SwiftChat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SwiftChat> ${CMAKE_BINARY_DIR}/bin/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/static ${CMAKE_BINARY_DIR}/bin/static
    COMMENT "Installing SwiftChat and static files to bin directory"
)

# 安装规则（用于 make install）
install(TARGETS SwiftChat
    RUNTIME DESTINATION .
)

# 安装静态文件到 bin/static 目录
install(DIRECTORY ${CMAKE_SOURCE_DIR}/static/
    DESTINATION static
    FILES_MATCHING PATTERN "*"
)
