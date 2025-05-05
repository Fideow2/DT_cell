#ifndef BASE_CELL_H
#define BASE_CELL_H

#include "Entity.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include "../structs.h"

// 基础细胞类，实现所有细胞共有的功能
class BaseCell : public Entity {
public:
    BaseCell(const cv::Point2f& pos, int playerNum, const cv::Vec3b& baseColor,
            float phaseOffset, float aggression = 0.0f);
    virtual ~BaseCell() {}
    
    // 实现Entity接口
    void update(float deltaTime, const GameConfig& config, const cv::Size& canvasSize) override;
    void render(cv::Mat& canvas, const std::map<std::string, float>& config, float scale, float time) override;
    bool isAlive() const override;
    cv::Point2f getPosition() const override;
    
    // 物理和运动
    virtual void applyAcceleration(const cv::Point2f& acc);
    void applyKnockback(const cv::Point2f& force);
    
    // 添加直接设置位置和速度的方法
    void setPosition(const cv::Point2f& newPosition) { position = newPosition; }
    void setVelocity(const cv::Point2f& newVelocity) { velocity = newVelocity; }
    
    // 战斗系统
    bool isAttacking() const;
    void setAttacking(bool attacking);
    float getAttackTime() const;
    void setAttackTime(float time);
    void takeDamage(float damage);
    
    // 防御和格挡
    bool isShielding() const;
    void setShielding(bool shielding);
    float getShieldTime() const;
    void setShieldTime(float time);
    bool hasParried() const;
    void setParried(bool parried);
    float getParryTime() const;
    void setParryTime(float time);
    float getShieldDuration() const;
    void setShieldDuration(float duration);
    float getShieldCooldownTime() const;
    void setShieldCooldownTime(float time);
    float getDamageReduction() const;
    void setDamageReduction(float reduction);
    void updateShieldCooldown(float deltaTime);
    bool canToggleShield() const;
    
    // 视觉和状态
    bool isFacingRight() const;
    void setFacingRight(bool facing);
    float getAggressionLevel() const;
    void setAggressionLevel(float level);
    const std::vector<BloodDrop>& getBloodDrops() const;
    
    // 新增方法：添加血液滴
    void addBloodDrop(const cv::Point2f& position, const cv::Point2f& velocity, float size, float lifetime, float rotation);
    
    // 新增公共访问方法
    const cv::Vec3b& getColor() const { return color; }
    float getTailPhaseOffset() const { return tailPhaseOffset; }
    int getPlayerNumber() const { return playerNumber; }
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    
    // 为了允许drawing.cpp访问
    friend void drawCell(cv::Mat& canvas, const BaseCell& cell, const std::map<std::string, float>& config, float scale, float time);
    // 为了允许physics.cpp中的函数访问
    friend void createBloodEffect(BaseCell& cell, const cv::Point2f& hitPosition, bool faceRight, const cv::Point2f& spearTipPosition);
    
protected:
    cv::Point2f position;
    cv::Point2f velocity;
    cv::Point2f acceleration;
    bool faceRight;
    int playerNumber;
    cv::Vec3b color;
    float tailPhaseOffset;
    float aggressionLevel;
    
    float health;
    float maxHealth;
    bool isAttackingFlag;
    float attackTimeValue;
    
    bool isShieldingFlag;
    float shieldTimeValue;
    bool hasParriedFlag;
    float parryTimeValue;
    float shieldDurationValue;
    float shieldCooldownTimeValue;
    float damageReductionValue;
    
    std::vector<BloodDrop> bloodDrops;
    
    // 内部辅助方法
    void updatePhysics(float deltaTime, float maxSpeed, float drag, const cv::Size& canvasSize);
    void updateShieldState(float deltaTime);
};

#endif // BASE_CELL_H