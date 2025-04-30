#include "AICell.h"
#include <algorithm>
#include <random>

AICell::AICell(const cv::Point2f& pos, int id, const cv::Vec3b& baseColor,
               float phaseOffset, float aggression)
    : BaseCell(pos, id, baseColor, phaseOffset, aggression) {
    
    // 初始化随机数生成器
    std::random_device rd;
    gen = std::mt19937(rd());
    moveDist = std::uniform_real_distribution<float>(-1.0f, 1.0f); // 将在update时根据config缩放
    probDist = std::uniform_real_distribution<float>(0.0f, 1.0f);
}

void AICell::update(float deltaTime, const GameConfig& config, const cv::Size& canvasSize) {
    // 首先更新AI行为
    updateAIBehavior(deltaTime, config);
    
    // 然后调用基类的更新方法处理物理、攻击动画等
    BaseCell::update(deltaTime, config, canvasSize);
}

void AICell::updateAIBehavior(float deltaTime, const GameConfig& config) {
    // 随机移动
    if (probDist(gen) < config.randomMoveProbability) {
        float strength = config.randomMoveStrength;
        applyAcceleration(cv::Point2f(
            moveDist(gen) * strength,
            moveDist(gen) * strength
        ));
    }
    
    // 随机改变攻击性
    if (probDist(gen) < config.aggressionChangeProbability) {
        float change = (probDist(gen) < 0.5f) ? 
                      -config.aggressionChangeAmount : 
                       config.aggressionChangeAmount;
        
        setAggressionLevel(std::clamp(
            getAggressionLevel() + change, 
            config.minAggression, 
            config.maxAggression
        ));
    }
    
    // 未来可以在这里添加更复杂的AI行为，例如：
    // - 根据附近实体追逐行为
    // - 基于健康状况的逃跑行为
    // - 协作或群体行为
    // - 根据攻击性的攻击行为
}