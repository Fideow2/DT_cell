#include "BaseCell.h"
#include "../physics.h"
#include "../drawing.h"
#include <algorithm>

BaseCell::BaseCell(const cv::Point2f& pos, int playerNum, const cv::Vec3b& baseColor,
                float phaseOffset, float aggression)
    : position(pos),
      velocity(0, 0),
      acceleration(0, 0),
      faceRight(true),
      playerNumber(playerNum),
      color(baseColor),
      tailPhaseOffset(phaseOffset),
      aggressionLevel(aggression),
      health(100.0f),
      maxHealth(100.0f),
      isAttackingFlag(false),
      attackTimeValue(0.0f),
      isShieldingFlag(false),
      shieldTimeValue(0.0f),
      hasParriedFlag(false),
      parryTimeValue(0.0f),
      shieldDurationValue(2.0f),
      shieldCooldownTimeValue(0.0f),
      damageReductionValue(0.75f) {
}

void BaseCell::update(float deltaTime, const GameConfig& config, const cv::Size& canvasSize) {
    // 更新物理状态
    updatePhysics(deltaTime, config.maxSpeed, config.drag, canvasSize);
    
    // 更新攻击动画
    if (isAttackingFlag) {
        attackTimeValue += deltaTime / config.attackDuration;
        
        // 检查攻击动画是否完成
        if (attackTimeValue >= 1.0f) {
            isAttackingFlag = false;
            attackTimeValue = 0.0f;
        }
    }
    
    // 更新盾牌状态
    updateShieldState(deltaTime);
    
    // 更新血液粒子效果
    updateBloodDrops(bloodDrops, deltaTime);
}

void BaseCell::render(cv::Mat& canvas, const std::map<std::string, float>& config, float scale, float time) {
    // 使用现有的绘图函数
    drawCell(canvas, *this, config, scale, time);
}

bool BaseCell::isAlive() const {
    return health > 0;
}

cv::Point2f BaseCell::getPosition() const {
    return position;
}

void BaseCell::applyAcceleration(const cv::Point2f& acc) {
    acceleration += acc;
}

void BaseCell::applyKnockback(const cv::Point2f& force) {
    velocity += force;
}

bool BaseCell::isAttacking() const {
    return isAttackingFlag;
}

void BaseCell::setAttacking(bool attacking) {
    isAttackingFlag = attacking;
}

float BaseCell::getAttackTime() const {
    return attackTimeValue;
}

void BaseCell::setAttackTime(float time) {
    attackTimeValue = time;
}

void BaseCell::takeDamage(float damage) {
    health -= damage;
    if (health < 0) {
        health = 0;
    }
}

bool BaseCell::isShielding() const {
    return isShieldingFlag;
}

void BaseCell::setShielding(bool shielding) {
    isShieldingFlag = shielding;
}

float BaseCell::getShieldTime() const {
    return shieldTimeValue;
}

void BaseCell::setShieldTime(float time) {
    shieldTimeValue = time;
}

bool BaseCell::hasParried() const {
    return hasParriedFlag;
}

void BaseCell::setParried(bool parried) {
    hasParriedFlag = parried;
}

float BaseCell::getParryTime() const {
    return parryTimeValue;
}

void BaseCell::setParryTime(float time) {
    parryTimeValue = time;
}

float BaseCell::getShieldDuration() const {
    return shieldDurationValue;
}

void BaseCell::setShieldDuration(float duration) {
    shieldDurationValue = duration;
}

float BaseCell::getShieldCooldownTime() const {
    return shieldCooldownTimeValue;
}

void BaseCell::setShieldCooldownTime(float time) {
    shieldCooldownTimeValue = time;
}

float BaseCell::getDamageReduction() const {
    return damageReductionValue;
}

void BaseCell::setDamageReduction(float reduction) {
    damageReductionValue = reduction;
}

void BaseCell::updateShieldCooldown(float deltaTime) {
    if (shieldCooldownTimeValue > 0) {
        shieldCooldownTimeValue = std::max(0.0f, shieldCooldownTimeValue - deltaTime);
    }
}

bool BaseCell::canToggleShield() const {
    return shieldCooldownTimeValue <= 0;
}

bool BaseCell::isFacingRight() const {
    return faceRight;
}

void BaseCell::setFacingRight(bool facing) {
    faceRight = facing;
}

float BaseCell::getAggressionLevel() const {
    return aggressionLevel;
}

void BaseCell::setAggressionLevel(float level) {
    aggressionLevel = level;
}

const std::vector<BloodDrop>& BaseCell::getBloodDrops() const {
    return bloodDrops;
}

// 新添加的方法实现
void BaseCell::addBloodDrop(const cv::Point2f& position, const cv::Point2f& velocity, float size, float lifetime, float rotation) {
    bloodDrops.emplace_back(position, velocity, size, lifetime, rotation);
}

void BaseCell::updatePhysics(float deltaTime, float maxSpeed, float drag, const cv::Size& canvasSize) {
    // 基于加速度更新速度
    velocity += acceleration;
    
    // 限制速度不超过最大速度
    velocity.x = std::clamp(velocity.x, -maxSpeed, maxSpeed);
    velocity.y = std::clamp(velocity.y, -maxSpeed, maxSpeed);
    
    // 基于速度更新位置
    position += velocity;
    
    // 应用阻力
    velocity *= drag;
    
    // 重置加速度
    acceleration = cv::Point2f(0, 0);
    
    // 边界检查，保持细胞在画布内
    if (position.x < 0) {
        position.x = 0;
        velocity.x *= -0.5f; // 反弹
    }
    if (position.x > canvasSize.width) {
        position.x = canvasSize.width;
        velocity.x *= -0.5f; // 反弹
    }
    if (position.y < 0) {
        position.y = 0;
        velocity.y *= -0.5f; // 反弹
    }
    if (position.y > canvasSize.height) {
        position.y = canvasSize.height;
        velocity.y *= -0.5f; // 反弹
    }
    
    // 根据速度更新方向，添加更大的阈值和缓冲以避免频繁切换
    // 只有本地玩家或者速度超过较大阈值时才更新朝向
    if (velocity.x > 1.0f) {
        faceRight = true;
    } else if (velocity.x < -1.0f) {
        faceRight = false;
    }
    // 速度很小的情况下保持当前朝向
}

void BaseCell::updateShieldState(float deltaTime) {
    // 更新盾牌时间
    if (isShieldingFlag) {
        shieldTimeValue += deltaTime;
        
        // 盾牌持续时间超过限制后自动降下
        if (shieldTimeValue >= shieldDurationValue) {
            isShieldingFlag = false;
            shieldTimeValue = 0.0f;
            shieldCooldownTimeValue = 0.3f; // 盾牌自动降下后应用冷却时间
        }
    } else {
        shieldTimeValue = 0.0f;
    }
    
    // 更新格挡效果时间
    if (hasParriedFlag) {
        parryTimeValue += deltaTime;
        // 格挡效果持续0.5秒
        if (parryTimeValue >= 0.5f) {
            hasParriedFlag = false;
            parryTimeValue = 0.0f;
        }
    }
}