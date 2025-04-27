# 指定CMake的最低版本
cmake_minimum_required(VERSION 3.8)

# 定义项目名称和使用的编程语言
project(cell)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV库和当前目录的头文件
include_directories(${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

# 添加所有源文件
set(SOURCES
    main.cpp
    drawing.cpp
    physics.cpp
)

# 添加可执行文件
add_executable(cell ${SOURCES})

# 将OpenCV库链接到可执行文件
target_link_libraries(cell ${OpenCV_LIBS})
