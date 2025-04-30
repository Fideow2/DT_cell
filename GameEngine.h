#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <random>
#include <map>
#include <chrono>
#include "GameConfig.h"

class BaseCell;
class PlayerCell;

class GameEngine {
public:
    GameEngine();
    void run();

private:
    // 初始化方法
    void initializeConfig();
    void initializeRandomGenerators();
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
    
    // 游戏状态
    bool running;
    cv::Size canvasSize;
    float scale;
    float time;
    float deltaTime;
    
    // 配置
    GameConfig gameConfig;
    std::map<std::string, float> cellConfig;
    
    // 实体管理
    std::vector<std::shared_ptr<BaseCell>> entities;
    std::vector<PlayerCell*> playerCells; // 不拥有这些指针，只是便于访问
    
    // 计时
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point lastUpdateTime;
    
    // 随机数生成
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> xDist;
    std::uniform_real_distribution<float> yDist;
    std::uniform_real_distribution<float> moveDist;
    std::uniform_real_distribution<float> probDist;
    std::uniform_real_distribution<float> aggressionDist;
    std::uniform_int_distribution<int> colorDist;
    std::uniform_real_distribution<float> phaseDist;
};

#endif // GAME_ENGINE_H