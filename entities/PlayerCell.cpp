#include "PlayerCell.h"
#include <algorithm>

PlayerCell::PlayerCell(const cv::Point2f& pos, int playerNum, const cv::Vec3b& baseColor,
                      float phaseOffset, float aggression, const std::string& cellGene)
    : BaseCell(pos, playerNum, baseColor, phaseOffset, aggression, cellGene) {
}

// 添加新的简化构造函数实现
PlayerCell::PlayerCell(const cv::Point2f& pos, int playerNum)
    : BaseCell(pos, playerNum, 
              // 根据玩家编号设置默认颜色
              (playerNum == 1) ? cv::Vec3b(200, 230, 255) : cv::Vec3b(200, 255, 220),
              // 默认相位偏移
              0.0f,
              // 默认攻击性
              0.0f) {
}

void PlayerCell::moveUp(float accelerationStep) {
    // 应用基因对移动速度的影响 - 使用平方关系增强效果
    applyAcceleration(cv::Point2f(0, -accelerationStep * std::pow(speedMultiplier, 1.5f)));
}

void PlayerCell::moveDown(float accelerationStep) {
    // 应用基因对移动速度的影响 - 使用平方关系增强效果
    applyAcceleration(cv::Point2f(0, accelerationStep * std::pow(speedMultiplier, 1.5f)));
}

void PlayerCell::moveLeft(float accelerationStep) {
    // 应用基因对移动速度的影响 - 使用平方关系增强效果
    applyAcceleration(cv::Point2f(-accelerationStep * std::pow(speedMultiplier, 1.5f), 0));
    setFacingRight(false);
}

void PlayerCell::moveRight(float accelerationStep) {
    // 应用基因对移动速度的影响 - 使用平方关系增强效果
    applyAcceleration(cv::Point2f(accelerationStep * std::pow(speedMultiplier, 1.5f), 0));
    setFacingRight(true);
}

void PlayerCell::attack() {
    // 只有没有举盾时才能攻击
    if (!isAttacking() && !isShielding()) {
        setAttacking(true);
        setAttackTime(0.0f);
    }
}

void PlayerCell::toggleShield(float cooldown) {
    // 切换盾牌状态
    bool newState = !isShielding();
    setShielding(newState);
    
    if (newState) {
        // 启动盾牌时重置格挡时间窗口
        setShieldTime(0.0f);
    } else {
        // 放下盾牌时应用冷却时间，考虑防御基因
        setShieldCooldownTime(cooldown / defenseMultiplier);
        
        // 如果正在攻击，取消攻击
        if (isAttacking()) {
            setAttacking(false);
            setAttackTime(0.0f);
        }
    }
}

void PlayerCell::increaseAggression(float amount) {
    // 应用攻击基因对攻击性增长的影响
    setAggressionLevel(std::min(1.0f, getAggressionLevel() + amount * attackMultiplier));
}

void PlayerCell::decreaseAggression(float amount) {
    // 应用防御基因对攻击性降低的影响
    setAggressionLevel(std::max(0.0f, getAggressionLevel() - amount * defenseMultiplier));
}