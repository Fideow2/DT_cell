#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <memory>

class NetworkClient;

class NetworkBehaviorMonitor {
public:
    NetworkBehaviorMonitor();
    ~NetworkBehaviorMonitor();

    // 监控客户端行为，检测异常
    bool monitorClientBehavior(const std::shared_ptr<NetworkClient>& client);
    
    // 处理异常行为
    void handleAbnormalBehavior(const std::shared_ptr<NetworkClient>& client);
    
    // 重置异常计数
    void resetAbnormalCount(const std::shared_ptr<NetworkClient>& client);
    
    // 设置行为检测阈值
    void setThresholds(int maxPacketsPerSecond, int maxPayloadSize, int maxAbnormalCount);

private:
    struct ClientStats {
        std::chrono::time_point<std::chrono::steady_clock> lastUpdateTime;
        int packetsReceivedCount;
        int abnormalBehaviorCount;
        bool isWarned;
    };

    std::vector<std::pair<std::shared_ptr<NetworkClient>, ClientStats>> clientStats;
    
    int maxPacketsPerSecond;
    int maxPayloadSize;
    int maxAbnormalCount;
    
    // 检查数据包频率是否异常
    bool isPacketRateAbnormal(ClientStats& stats);
    
    // 检查数据包大小是否异常
    bool isPayloadSizeAbnormal(const std::shared_ptr<NetworkClient>& client);
    
    // 记录异常行为
    void logAbnormalBehavior(const std::string& details);
};
