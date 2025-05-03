#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "NetworkManager.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <memory>

// 网络客户端实现
class NetworkClient : public NetworkManager {
public:
    NetworkClient(const std::string& serverIP = "127.0.0.1", int serverPort = 8888);
    virtual ~NetworkClient();
    
    // 实现NetworkManager接口
    bool initialize() override;
    bool sendMessage(const NetworkMessage& msg) override;
    bool receiveMessage(NetworkMessage& msg) override;
    void shutdown() override;
    bool isConnected() const override;
    void setMessageCallback(MessageCallback callback) override;
    
    // 客户端特有方法
    bool connectToServer();
    void disconnect();
    
    // 获取最后接收的数据包大小
    int getLastPacketSize() const;
    
    // 获取客户端ID
    int getClientId() const;
    
    // 限制连接速度（处理异常行为时使用）
    void throttleConnection();
    
private:
    std::string serverIP;
    int serverPort;
    int clientSocket;
    std::atomic<bool> running;
    std::atomic<bool> connected;
    
    std::thread receiveThread;
    
    std::mutex sendMutex;
    std::mutex receiveMutex;
    std::queue<NetworkMessage> receiveQueue;
    
    // 添加以下成员变量
    int clientId;                 // 客户端唯一标识
    int lastPacketSize;           // 最后接收的数据包大小
    bool isThrottled;             // 连接是否被限制
    int throttleDelayMs;          // 限制连接时的延迟毫秒数
    
    // 接收线程函数
    void receiveThreadFunc();
    
    // 初始化socket
    bool initializeSocket();
    
    // 处理网络消息
    void handleMessage(const NetworkMessage& msg);
};

#endif // NETWORK_CLIENT_H