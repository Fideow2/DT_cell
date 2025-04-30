#include "PlayerCell.h"
#include <algorithm>

PlayerCell::PlayerCell(const cv::Point2f& pos, int playerNum, const cv::Vec3b& baseColor,
                      float phaseOffset, float aggression)
    : BaseCell(pos, playerNum, baseColor, phaseOffset, aggression) {
}

void PlayerCell::moveUp(float accelerationStep) {
    applyAcceleration(cv::Point2f(0, -accelerationStep));
}

void PlayerCell::moveDown(float accelerationStep) {
    applyAcceleration(cv::Point2f(0, accelerationStep));
}

void PlayerCell::moveLeft(float accelerationStep) {
    applyAcceleration(cv::Point2f(-accelerationStep, 0));
    setFacingRight(false);
}

void PlayerCell::moveRight(float accelerationStep) {
    applyAcceleration(cv::Point2f(accelerationStep, 0));
    setFacingRight(true);
}

void PlayerCell::attack() {
    if (!isAttacking()) {
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
        // 放下盾牌时应用冷却时间
        setShieldCooldownTime(cooldown);
        
        // 如果正在攻击，取消攻击
        if (isAttacking()) {
            setAttacking(false);
            setAttackTime(0.0f);
        }
    }
}

void PlayerCell::increaseAggression(float amount) {
    setAggressionLevel(std::min(1.0f, getAggressionLevel() + amount));
}

void PlayerCell::decreaseAggression(float amount) {
    setAggressionLevel(std::max(0.0f, getAggressionLevel() - amount));
}