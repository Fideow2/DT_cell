#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "NetworkManager.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <memory>

// 网络服务器实现
class NetworkServer : public NetworkManager {
public:
    NetworkServer(int port = 8888);
    virtual ~NetworkServer();
    
    // 实现NetworkManager接口
    bool initialize() override;
    bool sendMessage(const NetworkMessage& msg) override;
    bool receiveMessage(NetworkMessage& msg) override;
    void shutdown() override;
    bool isConnected() const override;
    void setMessageCallback(MessageCallback callback) override;
    
    // 服务器特有方法
    void startListening();
    void stopListening();
    bool hasClient() const;
    
private:
    int serverPort;
    int serverSocket;
    int clientSocket;
    std::atomic<bool> running;
    std::atomic<bool> connected;
    
    std::thread listenThread;
    std::thread receiveThread;
    
    std::mutex sendMutex;
    std::mutex receiveMutex;
    std::queue<NetworkMessage> receiveQueue;
    
    // 监听线程函数
    void listenThreadFunc();
    
    // 接收线程函数
    void receiveThreadFunc();
    
    // 初始化socket
    bool initializeSocket();
    
    // 处理网络消息
    void handleMessage(const NetworkMessage& msg);
};

#endif // NETWORK_SERVER_H