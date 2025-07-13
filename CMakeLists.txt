# 指定CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(SwiftChat)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 要求必须支持指定的C++标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型为调试模式
set(CMAKE_BUILD_TYPE Debug)
# 设置调试选项：启用调试信息(-g)，禁用优化(-O0)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# 查找并链接线程库
find_package(Threads REQUIRED)
# 查找并链接SQLite3数据库库
find_package(SQLite3 REQUIRED)

# 设置头文件搜索路径
include_directories(
    ${CMAKE_SOURCE_DIR}/src        # 项目源码目录
    ${CMAKE_SOURCE_DIR}/third_party # 第三方库目录
    ${SQLite3_INCLUDE_DIRS}        # SQLite3头文件目录
)

# 设置安装路径为构建目录
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")

# 添加src子目录到构建系统
add_subdirectory(src)

# 添加测试目录到构建系统
add_subdirectory(tests)


