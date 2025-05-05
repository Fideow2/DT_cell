#ifndef BASE_CELL_H
#define BASE_CELL_H

#include "Entity.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <random>
#include "../structs.h"

// 前向声明AI细胞类用于后代生成
class AICell;

// 基础细胞类，实现所有细胞共有的功能
class BaseCell : public Entity {
public:
    BaseCell(const cv::Point2f& pos, int playerNum, const cv::Vec3b& baseColor,
            float phaseOffset, float aggression = 0.0f, const std::string& cellGene = "");
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
    // 添加设置颜色的方法
    void setColor(const cv::Vec3b& newColor);
    float getTailPhaseOffset() const { return tailPhaseOffset; }
    int getPlayerNumber() const { return playerNumber; }
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    
    // 修改基因相关方法
    const std::string& getGene() const;
    int getFaction() const;
    void setFaction(int newFaction);
    float getGeneticSimilarity(const BaseCell& other) const;
    
    // 修改繁殖方法返回类型为BaseCell*
    static BaseCell* createOffspring(const BaseCell& parent1, 
                                    const BaseCell& parent2, 
                                    const cv::Size& canvasSize);
    
    // 基因特异性伤害系数
    float getGeneticDamageMultiplier(const BaseCell& target) const;
    float getGeneticDefenseMultiplier(const BaseCell& attacker) const;
    
    // 获取细胞尺寸倍数
    float getSizeMultiplier() const;
    
    // 新增获取攻击和防御倍率的方法
    float getAttackMultiplier() const { return attackMultiplier; }
    float getDefenseMultiplier() const { return defenseMultiplier; }
    
    // 为了允许drawing.cpp访问
    friend void drawCell(cv::Mat& canvas, const BaseCell& cell, const std::map<std::string, float>& config, float scale, float time);
    // 为了允许physics.cpp中的函数访问
    friend void createBloodEffect(BaseCell& cell, const cv::Point2f& hitPosition, bool faceRight, const cv::Point2f& spearTipPosition);
    friend void handleCellCollision(BaseCell& cell1, BaseCell& cell2, GameConfig& config);
    
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
    
    // 基因和阵营属性
    const std::string gene;
    int faction;
    float sizeMultiplier;
    float sharpnessMultiplier;
    float speedMultiplier;
    float attackMultiplier;
    float defenseMultiplier;
    
    // 内部辅助方法
    void updatePhysics(float deltaTime, float maxSpeed, float drag, const cv::Size& canvasSize);
    void updateShieldState(float deltaTime);
    void parseGene();
    void updateColorByFaction();
    
    // 修改基因突变辅助函数生成更杂乱的字符串
    static std::string mutateGene(const std::string& parentGene1, const std::string& parentGene2);
    static char mutateGeneChar(char c);
    static std::string generateRandomGene(int length = 16);
    static std::mt19937& getRandomEngine();
    
    // 从杂乱基因中提取属性的辅助方法
    float extractGeneAttribute(const std::string& geneStr, int startIndex, int length) const;
};

#endif // BASE_CELL_H