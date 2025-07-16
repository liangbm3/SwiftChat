# 创建examples目录的CMake配置文件

# 创建WebSocket演示程序
add_executable(websocket_demo 
    websocket_demo.cpp
    ../src/websocket/websocket_server.cpp
    ../src/utils/logger.cpp
)

# 链接必要的库
target_link_libraries(websocket_demo
    ${CMAKE_THREAD_LIBS_INIT}
)

# 设置头文件目录
target_include_directories(websocket_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/websocketpp
)

# 设置输出目录
set_target_properties(websocket_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

# 创建API服务器演示程序
add_executable(api_server_demo
    api_server_demo.cpp
    ../src/http/http_server.cpp
    ../src/http/http_request.cpp
    ../src/http/http_response.cpp
    ../src/service/user_service.cpp
    ../src/service/room_service.cpp
    ../src/db/database_manager.cpp
    ../src/chat/user.cpp
    ../src/utils/logger.cpp
    ../src/utils/jwt.cpp
    ../src/utils/api_response.cpp
    ../src/utils/auth_middleware.cpp
)

# 链接必要的库
target_link_libraries(api_server_demo
    ${CMAKE_THREAD_LIBS_INIT}
    sqlite3
    ${OPENSSL_LIBRARIES}
)

# 设置头文件目录
target_include_directories(api_server_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/jwt-cpp/include
    ${OPENSSL_INCLUDE_DIR}
)

# 设置输出目录
set_target_properties(api_server_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
