# 测试目录的CMake配置文件

# 创建测试可执行文件，包含user.cpp的实现
add_executable(test_user 
    test_user.cpp
    ../src/chat/user.cpp
)

# 链接必要的库
target_link_libraries(test_user 
    ${CMAKE_THREAD_LIBS_INIT}
)

# 设置测试可执行文件的输出目录
set_target_properties(test_user PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# 包含必要的头文件目录
target_include_directories(test_user PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

# 如果使用CTest，可以添加测试
enable_testing()
add_test(NAME UserTests COMMAND test_user)
