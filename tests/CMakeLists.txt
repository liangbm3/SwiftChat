# 测试目录的CMake配置文件

# 创建用户测试可执行文件
add_executable(test_user 
    chat/test_user.cpp
    ../src/chat/user.cpp
)

# 创建日志系统测试可执行文件
add_executable(test_logger 
    utils/test_logger.cpp
)

# 创建数据库管理器测试可执行文件
add_executable(test_database_manager 
    db/test_database_manager.cpp
    ../src/db/database_manager.cpp
    ../src/chat/user.cpp
)

# 链接必要的库
target_link_libraries(test_user 
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(test_logger 
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(test_database_manager 
    sqlite3
    ${CMAKE_THREAD_LIBS_INIT}
)

# 设置测试可执行文件的输出目录
set_target_properties(test_user PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_logger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_database_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# 包含必要的头文件目录
target_include_directories(test_user PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

target_include_directories(test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

# 如果使用CTest，可以添加测试
enable_testing()
add_test(NAME UserTests COMMAND test_user)
add_test(NAME LoggerTests COMMAND test_logger)
