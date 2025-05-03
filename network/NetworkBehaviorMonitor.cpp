#include "NetworkBehaviorMonitor.h"
#include "NetworkClient.h"
#include <iostream>
#include <algorithm>

NetworkBehaviorMonitor::NetworkBehaviorMonitor() 
    : maxPacketsPerSecond(100), maxPayloadSize(1024), maxAbnormalCount(5) {
}

NetworkBehaviorMonitor::~NetworkBehaviorMonitor() {
}

bool NetworkBehaviorMonitor::monitorClientBehavior(const std::shared_ptr<NetworkClient>& client) {
    // 查找客户端统计数据
    auto it = std::find_if(clientStats.begin(), clientStats.end(),
                          [&client](const auto& pair) { return pair.first == client; });
    
    if (it == clientStats.end()) {
        // 新客户端，初始化统计数据
        ClientStats stats;
        stats.lastUpdateTime = std::chrono::steady_clock::now();
        stats.packetsReceivedCount = 0;
        stats.abnormalBehaviorCount = 0;
        stats.isWarned = false;
        clientStats.push_back(std::make_pair(client, stats));
        return true;
    }
    
    // 更新统计数据
    ClientStats& stats = it->second;
    stats.packetsReceivedCount++;
    
    // 检查异常行为
    bool isAbnormal = false;
    if (isPacketRateAbnormal(stats) || isPayloadSizeAbnormal(client)) {
        stats.abnormalBehaviorCount++;
        isAbnormal = true;
        
        // 记录异常行为
        logAbnormalBehavior("客户端 " + std::to_string(client->getClientId()) + " 出现异常行为");
        
        // 如果连续异常行为超过阈值，则处理
        if (stats.abnormalBehaviorCount >= maxAbnormalCount) {
            handleAbnormalBehavior(client);
            return false;
        }
    } else {
        // 正常行为，逐渐减少异常计数
        if (stats.abnormalBehaviorCount > 0) {
            stats.abnormalBehaviorCount--;
        }
    }
    
    return !isAbnormal;
}

void NetworkBehaviorMonitor::handleAbnormalBehavior(const std::shared_ptr<NetworkClient>& client) {
    auto it = std::find_if(clientStats.begin(), clientStats.end(),
                          [&client](const auto& pair) { return pair.first == client; });
    
    if (it != clientStats.end()) {
        ClientStats& stats = it->second;
        
        // 如果还未发出警告，则先警告
        if (!stats.isWarned) {
            std::cout << "警告：客户端 " << client->getClientId() << " 行为异常，可能影响游戏体验" << std::endl;
            stats.isWarned = true;
            stats.abnormalBehaviorCount = maxAbnormalCount / 2; // 重置一半的异常计数
        } else {
            // 已经警告过，进一步限制
            std::cout << "客户端 " << client->getClientId() << " 持续异常行为，正在进行网络节流" << std::endl;
            // 这里应该实现网络节流或临时断开连接的逻辑
            // client->throttleConnection() 或类似的方法
        }
    }
}

void NetworkBehaviorMonitor::resetAbnormalCount(const std::shared_ptr<NetworkClient>& client) {
    auto it = std::find_if(clientStats.begin(), clientStats.end(),
                          [&client](const auto& pair) { return pair.first == client; });
    
    if (it != clientStats.end()) {
        it->second.abnormalBehaviorCount = 0;
        it->second.isWarned = false;
    }
}

void NetworkBehaviorMonitor::setThresholds(int maxPacketsPerSecond, int maxPayloadSize, int maxAbnormalCount) {
    this->maxPacketsPerSecond = maxPacketsPerSecond;
    this->maxPayloadSize = maxPayloadSize;
    this->maxAbnormalCount = maxAbnormalCount;
}

bool NetworkBehaviorMonitor::isPacketRateAbnormal(ClientStats& stats) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - stats.lastUpdateTime).count();
    
    if (duration >= 1) {
        // 如果数据包率超过阈值，则认为异常
        bool isAbnormal = (stats.packetsReceivedCount / duration) > maxPacketsPerSecond;
        
        // 重置统计
        stats.packetsReceivedCount = 0;
        stats.lastUpdateTime = now;
        
        return isAbnormal;
    }
    
    return false;
}

bool NetworkBehaviorMonitor::isPayloadSizeAbnormal(const std::shared_ptr<NetworkClient>& client) {
    // 这里需要从客户端获取最后接收的数据包大小
    // 假设NetworkClient有getLastPacketSize()方法
    int lastPacketSize = client->getLastPacketSize();
    return lastPacketSize > maxPayloadSize;
}

void NetworkBehaviorMonitor::logAbnormalBehavior(const std::string& details) {
    std::cout << "网络异常行为: " << details << std::endl;
    // 可以扩展为写入日志文件
}
