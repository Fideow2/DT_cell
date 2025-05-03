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
    GameEngine.cpp
    entities/BaseCell.cpp
    entities/PlayerCell.cpp
    entities/AICell.cpp
    network/NetworkManager.cpp
    network/NetworkServer.cpp
    network/NetworkClient.cpp
    network/NetworkBehaviorMonitor.cpp
    NetGameEngine.cpp
)

# 添加可执行文件
add_executable(cell ${SOURCES})

# 将OpenCV库链接到可执行文件
target_link_libraries(cell ${OpenCV_LIBS})

# 在macOS上链接相关网络库
if(APPLE)
    target_link_libraries(cell "-framework CoreFoundation" "-framework SystemConfiguration" "-framework Network")
endif()

# 在Linux上链接相关网络库
if(UNIX AND NOT APPLE)
    target_link_libraries(cell pthread rt)
endif()

# 在Windows上链接相关网络库
if(WIN32)
    target_link_libraries(cell wsock32 ws2_32 Iphlpapi)
endif()

# 添加网络稳定性编译选项
add_compile_definitions(NETWORK_STABILITY_CHECK=1)
add_compile_definitions(MAX_NETWORK_TIMEOUT_MS=5000)
