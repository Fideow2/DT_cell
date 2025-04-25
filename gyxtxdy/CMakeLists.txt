# 指定CMake的最低版本
cmake_minimum_required(VERSION 1.0)

# 定义项目名称和使用的编程语言
project(ImageProcessor)

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV库的头文件
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加可执行文件
add_executable(image_processor main.cpp)

# 将OpenCV库链接到可执行文件
target_link_libraries(image_processor ${OpenCV_LIBS})
