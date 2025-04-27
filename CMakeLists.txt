# 指定CMake的最低版本
cmake_minimum_required(VERSION 3.8)

# 定义项目名称和使用的编程语言
project(cell)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV库的头文件
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加可执行文件
add_executable(cell main.cpp)

# 将OpenCV库链接到可执行文件
target_link_libraries(cell ${OpenCV_LIBS})
