#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <memory>
#include <opencv2/opencv.hpp>
#include "../entities/PlayerCell.h"
#include "NetworkBehaviorMonitor.h"

// 定义网络消息类型
enum class MessageType {
    CONNECT_REQUEST,    // 连接请求
    CONNECT_ACCEPT,     // 连接接受
    DISCONNECT,         // 断开连接
    PLAYER_STATE,       // 玩家状态更新
    PLAYER_INPUT,       // 玩家输入
    GAME_STATE,         // 游戏状态
    PING,               // 心跳检测
    PONG                // 心跳响应
};

// 网络消息结构
struct NetworkMessage {
    MessageType type;   // 消息类型
    std::vector<uint8_t> data; // 消息数据
    
    NetworkMessage() : type(MessageType::PING) {}
    
    NetworkMessage(MessageType t, const std::vector<uint8_t>& d) 
        : type(t), data(d) {}
};

// 玩家输入消息
struct PlayerInputMessage {
    bool moveUp;
    bool moveDown;
    bool moveLeft;
    bool moveRight;
    bool attack;
    bool shield;
    bool increaseAggression;
    bool decreaseAggression;
    
    PlayerInputMessage() 
        : moveUp(false), moveDown(false), moveLeft(false), moveRight(false),
          attack(false), shield(false), increaseAggression(false), decreaseAggression(false) {}
};

// 玩家状态消息
struct PlayerStateMessage {
    cv::Point2f position;
    cv::Point2f velocity;
    bool facingRight;
    float health;
    bool isAttacking;
    float attackTime;
    bool isShielding;
    float aggressionLevel;
};

// 用于序列化和反序列化网络消息
class NetworkSerializer {
public:
    // 序列化玩家输入
    static std::vector<uint8_t> serializePlayerInput(const PlayerInputMessage& input);
    
    // 反序列化玩家输入
    static PlayerInputMessage deserializePlayerInput(const std::vector<uint8_t>& data);
    
    // 序列化玩家状态
    static std::vector<uint8_t> serializePlayerState(const PlayerStateMessage& state);
    
    // 反序列化玩家状态
    static PlayerStateMessage deserializePlayerState(const std::vector<uint8_t>& data);
    
    // 从BaseCell获取玩家状态
    static PlayerStateMessage getPlayerStateFromCell(const BaseCell& cell);
    
    // 将玩家状态应用到BaseCell
    static void applyPlayerStateToCell(PlayerCell& cell, const PlayerStateMessage& state);
};

// 网络管理器接口
class NetworkManager {
public:
    virtual ~NetworkManager() {}
    
    // 初始化网络
    virtual bool initialize() = 0;
    
    // 发送消息
    virtual bool sendMessage(const NetworkMessage& msg) = 0;
    
    // 接收消息(非阻塞)
    virtual bool receiveMessage(NetworkMessage& msg) = 0;
    
    // 关闭连接
    virtual void shutdown() = 0;
    
    // 检查是否连接
    virtual bool isConnected() const = 0;
    
    // 设置消息接收回调
    typedef std::function<void(const NetworkMessage&)> MessageCallback;
    virtual void setMessageCallback(MessageCallback callback) = 0;
    
    // 显示服务器IP地址
    void displayServerIp(int port);
    
    // 启动服务器
    bool startServer(int port);
    
protected:
    MessageCallback messageCallback;

private:
    // 添加网络行为监控器
    NetworkBehaviorMonitor behaviorMonitor;
    
    // 在处理接收的数据前进行行为检查
    bool checkClientBehavior(const std::shared_ptr<NetworkClient>& client);
    
    // 获取本地IP地址
    std::vector<std::string> getLocalIpAddresses();
};

#endif // NETWORK_MANAGER_H