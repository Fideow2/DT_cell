#include "NetworkManager.h"
#include <cstring>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

// 获取本地IP地址
std::vector<std::string> NetworkManager::getLocalIpAddresses() {
    std::vector<std::string> ipAddresses;
    
#ifdef _WIN32
    // Windows实现
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return ipAddresses;
    }
    
    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) != 0) {
        WSACleanup();
        return ipAddresses;
    }
    
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    if (getaddrinfo(hostName, NULL, &hints, &result) != 0) {
        WSACleanup();
        return ipAddresses;
    }
    
    for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        struct sockaddr_in* addr = (struct sockaddr_in*)ptr->ai_addr;
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN);
        ipAddresses.push_back(std::string(ipStr));
    }
    
    freeaddrinfo(result);
    WSACleanup();
#else
    // Unix/Linux/macOS实现
    struct ifaddrs* interfaces = NULL;
    if (getifaddrs(&interfaces) == 0) {
        for (struct ifaddrs* ifa = interfaces; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                void* addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, addr, ipStr, INET_ADDRSTRLEN);
                
                // 过滤掉本地回环地址
                if (strcmp(ipStr, "127.0.0.1") != 0) {
                    ipAddresses.push_back(std::string(ipStr));
                }
            }
        }
        freeifaddrs(interfaces);
    }
#endif
    
    return ipAddresses;
}

// 显示服务器IP地址
void NetworkManager::displayServerIp(int port) {
    std::vector<std::string> ipAddresses = getLocalIpAddresses();
    
    std::cout << "=======================================" << std::endl;
    std::cout << "服务器已启动，可用的连接地址:" << std::endl;
    
    if (ipAddresses.empty()) {
        std::cout << "无法获取IP地址" << std::endl;
    } else {
        for (const auto& ip : ipAddresses) {
            std::cout << "  " << ip << ":" << port << std::endl;
        }
    }
    
    std::cout << "本地连接可使用: 127.0.0.1:" << port << std::endl;
    std::cout << "=======================================" << std::endl;
}

// 修改服务器启动方法，调用IP显示功能
bool NetworkManager::startServer(int port) {
    // ...原有的服务器启动代码...
    
    // 显示服务器IP
    displayServerIp(port);
    
    return true; // 根据实际启动结果返回
}

// 序列化玩家输入
std::vector<uint8_t> NetworkSerializer::serializePlayerInput(const PlayerInputMessage& input) {
    std::vector<uint8_t> data(8);
    uint8_t flags = 0;
    if (input.moveUp) flags |= 0x01;
    if (input.moveDown) flags |= 0x02;
    if (input.moveLeft) flags |= 0x04;
    if (input.moveRight) flags |= 0x08;
    if (input.attack) flags |= 0x10;
    if (input.shield) flags |= 0x20;
    if (input.increaseAggression) flags |= 0x40;
    if (input.decreaseAggression) flags |= 0x80;
    
    data[0] = flags;
    return data;
}

// 反序列化玩家输入
PlayerInputMessage NetworkSerializer::deserializePlayerInput(const std::vector<uint8_t>& data) {
    PlayerInputMessage input;
    if (data.empty()) return input;
    
    uint8_t flags = data[0];
    input.moveUp = (flags & 0x01) != 0;
    input.moveDown = (flags & 0x02) != 0;
    input.moveLeft = (flags & 0x04) != 0;
    input.moveRight = (flags & 0x08) != 0;
    input.attack = (flags & 0x10) != 0;
    input.shield = (flags & 0x20) != 0;
    input.increaseAggression = (flags & 0x40) != 0;
    input.decreaseAggression = (flags & 0x80) != 0;
    
    return input;
}

// 序列化玩家状态
std::vector<uint8_t> NetworkSerializer::serializePlayerState(const PlayerStateMessage& state) {
    std::vector<uint8_t> data(32); // 预分配足够的内存
    float* dataPtr = reinterpret_cast<float*>(data.data());
    
    dataPtr[0] = state.position.x;
    dataPtr[1] = state.position.y;
    dataPtr[2] = state.velocity.x;
    dataPtr[3] = state.velocity.y;
    dataPtr[4] = state.health;
    dataPtr[5] = state.attackTime;
    dataPtr[6] = state.aggressionLevel;
    
    uint8_t* flagsPtr = data.data() + 7 * sizeof(float);
    *flagsPtr = 0;
    if (state.facingRight) *flagsPtr |= 0x01;
    if (state.isAttacking) *flagsPtr |= 0x02;
    if (state.isShielding) *flagsPtr |= 0x04;
    
    return data;
}

// 反序列化玩家状态
PlayerStateMessage NetworkSerializer::deserializePlayerState(const std::vector<uint8_t>& data) {
    PlayerStateMessage state;
    if (data.size() < 32) return state;
    
    const float* dataPtr = reinterpret_cast<const float*>(data.data());
    
    state.position.x = dataPtr[0];
    state.position.y = dataPtr[1];
    state.velocity.x = dataPtr[2];
    state.velocity.y = dataPtr[3];
    state.health = dataPtr[4];
    state.attackTime = dataPtr[5];
    state.aggressionLevel = dataPtr[6];
    
    const uint8_t* flagsPtr = data.data() + 7 * sizeof(float);
    state.facingRight = (*flagsPtr & 0x01) != 0;
    state.isAttacking = (*flagsPtr & 0x02) != 0;
    state.isShielding = (*flagsPtr & 0x04) != 0;
    
    return state;
}

// 从BaseCell获取玩家状态
PlayerStateMessage NetworkSerializer::getPlayerStateFromCell(const BaseCell& cell) {
    PlayerStateMessage state;
    state.position = cell.getPosition();
    // 使用typeid可以在运行时获取velocity，但由于BaseCell没有公开API，我们只能获取公开的属性
    state.facingRight = cell.isFacingRight();
    state.health = cell.getHealth();
    state.isAttacking = cell.isAttacking();
    state.attackTime = cell.getAttackTime();
    state.isShielding = cell.isShielding();
    state.aggressionLevel = cell.getAggressionLevel();
    
    return state;
}

// 将玩家状态应用到BaseCell
void NetworkSerializer::applyPlayerStateToCell(PlayerCell& cell, const PlayerStateMessage& state) {
    // 直接使用远程玩家的朝向状态，避免朝向频繁切换
    cell.setFacingRight(state.facingRight);
    
    // 位置更新使用直接设置位置而不是通过移动方法
    cell.setPosition(state.position);
    
    // 设置速度状态
    cell.setVelocity(state.velocity);
    
    // 更新攻击状态
    if (state.isAttacking && !cell.isAttacking()) {
        cell.attack();
    }
    
    // 更新护盾状态
    if (state.isShielding != cell.isShielding()) {
        cell.toggleShield(0.3f); // 使用默认值
    }
    
    // 更新攻击性
    float currentAggression = cell.getAggressionLevel();
    float aggressionDiff = state.aggressionLevel - currentAggression;
    if (aggressionDiff > 0.01f) {
        cell.increaseAggression(aggressionDiff);
    } else if (aggressionDiff < -0.01f) {
        cell.decreaseAggression(-aggressionDiff);
    }
}

bool NetworkManager::checkClientBehavior(const std::shared_ptr<NetworkClient>& client) {
    return behaviorMonitor.monitorClientBehavior(client);
}

// 在处理接收数据的方法中调用行为检查
// 例如在receiveData或类似的方法中添加如下代码:
/*
bool NetworkManager::receiveData(const std::shared_ptr<NetworkClient>& client, const std::vector<uint8_t>& data) {
    // 检查客户端行为
    if (!checkClientBehavior(client)) {
        // 如果行为异常，可能需要额外处理
        return false;
    }
    
    // 正常处理接收到的数据
    // ...
    
    return true;
}
*/