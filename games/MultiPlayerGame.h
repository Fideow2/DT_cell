#ifndef MULTI_PLAYER_GAME_H
#define MULTI_PLAYER_GAME_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include "../GameConfig.h"
#include "../network/NetworkManager.h"
#include "../network/NetworkServer.h"
#include "../network/NetworkClient.h"

// 网络游戏模式
enum class NetGameMode {
    SERVER,     // 服务器模式
    CLIENT,     // 客户端模式
    STANDALONE  // 单机模式 (在多人游戏中作为本地测试模式)
};

// 多人游戏类
class MultiPlayerGame {
public:
    MultiPlayerGame();
    ~MultiPlayerGame();
    
    // 初始化网络游戏
    bool initialize(NetGameMode mode, const std::string& serverIP = "127.0.0.1", int port = 8888);
    
    // 运行游戏循环
    void run();
    
private:
    // 初始化方法
    void initializeConfig();
    void initializeNetworkManager();
    void createPlayers();
    void createAICells();
    
    // 游戏循环方法
    void updateFrameTime();
    void updateEntities();
    void handleCombat();
    void handleHit(BaseCell* attacker, BaseCell* target, 
                  const cv::Point2f& hitPosition, 
                  const cv::Point2f& spearTipPosition);
    void renderEntities(cv::Mat& canvas);
    void displayControls(cv::Mat& canvas);
    void handleInput();
    
    // 网络相关方法
    void processNetworkMessages();
    void handlePlayerInputMessage(const PlayerInputMessage& inputMsg);
    void sendPlayerState();
    void handlePlayerStateMessage(const PlayerStateMessage& stateMsg);
    void onNetworkMessage(const NetworkMessage& msg);
    
    // 检查和处理细胞繁殖
    void checkCellReproduction();
    
    // 游戏状态
    bool running;
    cv::Size canvasSize;
    float scale;
    float time;
    float deltaTime;
    NetGameMode gameMode;
    
    // 配置
    GameConfig gameConfig;
    std::map<std::string, float> cellConfig;
    
    // 实体管理
    std::vector<std::shared_ptr<BaseCell>> entities;
    std::vector<PlayerCell*> playerCells; // 不拥有这些指针，只是便于访问
    PlayerCell* localPlayer;   // 本地玩家
    PlayerCell* remotePlayer;  // 远程玩家
    
    // 计时
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point lastUpdateTime;
    std::chrono::high_resolution_clock::time_point lastNetworkUpdateTime;
    float networkUpdateInterval; // 网络更新间隔
    
    // 网络管理
    std::unique_ptr<NetworkManager> networkManager;
    bool networkInitialized;
    
    // 窗口标题
    std::string windowTitle;
};

#endif // MULTI_PLAYER_GAME_H
