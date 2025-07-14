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
)

# 链接必要的库
target_link_libraries(test_user 
    ${CMAKE_THREAD_LIBS_INIT}# 链接线程库  
)

target_link_libraries(test_logger 
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(test_database_manager 
    sqlite3
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(test_thread_pool
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(test_timer
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(test_http_request
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

set_target_properties(test_thread_pool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_timer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set_target_properties(test_http_request PROPERTIES
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

target_include_directories(test_database_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    
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

# 如果使用CTest，可以添加测试
enable_testing()
add_test(NAME UserTests COMMAND test_user)
add_test(NAME LoggerTests COMMAND test_logger)
add_test(NAME DatabaseManagerTests COMMAND test_database_manager)
add_test(NAME ThreadPoolTests COMMAND test_thread_pool)
add_test(NAME TimerTests COMMAND test_timer)
add_test(NAME HttpRequestTests COMMAND test_http_request)
