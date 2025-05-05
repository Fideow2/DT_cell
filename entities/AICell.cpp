#include "AICell.h"
#include <algorithm>
#include <random>

AICell::AICell(const cv::Point2f& pos, int id, const cv::Vec3b& baseColor,
               float phaseOffset, float aggression, const std::string& cellGene)
    : BaseCell(pos, id, baseColor, phaseOffset, aggression, cellGene) {
    
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
    // 随机移动，基于基因调整移动概率 - 提高移动概率
    float moveProbability = config.randomMoveProbability * speedMultiplier * 3.0f; // 3倍的移动概率
    if (probDist(gen) < moveProbability) {
        float strength = config.randomMoveStrength * speedMultiplier * 1.5f; // 1.5倍的移动强度
        applyAcceleration(cv::Point2f(
            moveDist(gen) * strength,
            moveDist(gen) * strength
        ));
    }
    
    // 追逐或逃避行为 - 根据自身健康状态决定
    float healthRatio = getHealth() / getMaxHealth();
    if (probDist(gen) < 0.05f) { // 有5%的概率进行方向性移动
        float dirX = 0.0f, dirY = 0.0f;
        
        if (healthRatio > 0.7f) {
            // 健康状态好 - 随机方向移动
            dirX = moveDist(gen);
            dirY = moveDist(gen);
        } else if (healthRatio > 0.3f) {
            // 健康状态中等 - 沿屏幕对角线移动
            dirX = (probDist(gen) > 0.5f) ? 1.0f : -1.0f;
            dirY = (probDist(gen) > 0.5f) ? 1.0f : -1.0f;
        } else {
            // 健康状态差 - 远离屏幕中心
            cv::Point2f screenCenter(400, 300); // 假设屏幕中心
            cv::Point2f awayDir = position - screenCenter;
            float length = cv::norm(awayDir);
            if (length > 0) {
                dirX = awayDir.x / length;
                dirY = awayDir.y / length;
            }
        }
        
        // 应用加速度，基于健康状态调整力度
        float escapeFactor = 1.0f + (1.0f - healthRatio) * 2.0f;
        applyAcceleration(cv::Point2f(dirX, dirY) * config.randomMoveStrength * 2.0f * escapeFactor);
    }
    
    // 随机改变攻击性，基于基因调整变化幅度
    if (probDist(gen) < config.aggressionChangeProbability) {
        float change = (probDist(gen) < 0.5f) ? 
                      -config.aggressionChangeAmount : 
                       config.aggressionChangeAmount;
        
        // 基于攻击基因调整攻击性变化
        change *= attackMultiplier;
        
        setAggressionLevel(std::clamp(
            getAggressionLevel() + change, 
            config.minAggression, 
            config.maxAggression
        ));
    }
    
    // 随机决定是否攻击，攻击性更强的细胞更容易攻击 - 提高攻击概率
    if (probDist(gen) < getAggressionLevel() * 0.05f * attackMultiplier && !isAttacking() && !isShielding()) {
        setAttacking(true);
        setAttackTime(0.0f);
    }
    
    // 随机决定是否举盾，防御性更强的细胞更容易举盾 - 提高举盾概率
    if (probDist(gen) < 0.02f * defenseMultiplier && canToggleShield() && !isAttacking()) {
        setShielding(!isShielding());
        if (isShielding()) {
            setShieldTime(0.0f);
        }
    }
}