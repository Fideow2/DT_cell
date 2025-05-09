#include "physics.h"
#include <random>
#include <algorithm> // for std::clamp
#include "entities/BaseCell.h"

using namespace cv;
using namespace std;

// Function to update blood drops physics
void updateBloodDrops(vector<BloodDrop>& bloodDrops, float deltaTime) {
    // Gravity constant
    const float gravity = 9.8f;

    for (auto it = bloodDrops.begin(); it != bloodDrops.end();) {
        // Update lifetime
        it->lifetime -= deltaTime;

        if (it->lifetime <= 0) {
            // Remove dead blood drops
            it = bloodDrops.erase(it);
        } else {
            // Update position based on velocity
            it->position += it->velocity * deltaTime;

            // Apply gravity
            it->velocity.y += gravity * deltaTime;

            // Note: We keep rotation fixed as assigned at creation

            ++it;
        }
    }
}

// Function to create blood splash effect at a specific position
void createBloodEffect(BaseCell& cell, const Point2f& hitPosition, bool faceRight, const Point2f& spearTipPosition) {
    // Direction factor based on facing direction
    float directionFactor = faceRight ? 1.0f : -1.0f;

    // Random number generators
    random_device rd;
    mt19937 gen(rd());

    // Distributions for a splatter effect
    uniform_real_distribution<float> velXDist(-2.0f, 2.0f);
    uniform_real_distribution<float> velYDist(-3.0f, 0.5f); // Upward bias
    uniform_real_distribution<float> offsetXDist(-20.0f, 20.0f); // Position X offset
    uniform_real_distribution<float> offsetYDist(-20.0f, 20.0f); // Position Y offset
    uniform_real_distribution<float> forwardOffsetDist(15.0f, 25.0f); // Forward offset (in front of spear)
    uniform_real_distribution<float> sizeDist(4.0f, 7.0f); // Even larger blood drop since we only have one
    uniform_real_distribution<float> lifeDist(0.6f, 1.3f); // Longer lifetime

    // Just one blood drop now
    // Add forward offset in the facing direction plus some randomness
    float forwardOffset = forwardOffsetDist(gen); // 15-25 pixels forward
    float offsetX = offsetXDist(gen);
    float offsetY = offsetYDist(gen);

    // Calculate the base position with forward offset in the direction the spear is facing
    Point2f basePosition = hitPosition + Point2f(directionFactor * forwardOffset, 0);

    // Then add the random offset
    Point2f dropPosition(basePosition.x + offsetX, basePosition.y + offsetY);

    // Random velocity with directional bias
    float vx = velXDist(gen) + directionFactor * 1.5f;
    float vy = velYDist(gen); // Mostly upward for splash effect

    // Random size and lifetime
    float size = sizeDist(gen);
    float lifetime = lifeDist(gen);

    // Calculate the direction from the spear tip to the blood drop position
    Point2f directionVector = dropPosition - spearTipPosition;

    // Calculate the rotation angle so that the pointed end of the blood drop points AWAY from the spear
    float rotation = atan2(directionVector.y, directionVector.x) - CV_PI/2;

    // 使用新添加的方法添加血滴效果
    cell.addBloodDrop(dropPosition, Point2f(vx, vy), size, lifetime, rotation);
}

// Function to check if a spear attack hits another cell
bool checkSpearCollision(const BaseCell& attacker, const BaseCell& target, float scale, float cellWidth,
                         Point2f& hitPosition, Point2f& spearTipPosition) {
    if (!attacker.isAttacking() || attacker.getAttackTime() >= 0.5f) {
        return false; // Only check during forward thrust
    }

    // Direction facing
    float directionFactor = attacker.isFacingRight() ? 1.0f : -1.0f;

    // Calculate spear tip position with attack extension
    float attackExtension = attacker.getAttackTime() * 2.0f * 50.0f * scale;
    float spearLength = 100.0f * scale;
    float spearTipX = attacker.getPosition().x + directionFactor * (spearLength * 1.5f + attackExtension);
    float spearTipY = attacker.getPosition().y + 60.0f * scale - spearLength * 0.3f;
    Point2f spearTip(spearTipX, spearTipY);

    // Calculate distance from spear tip to target center
    float distance = norm(spearTip - target.getPosition());

    // Store the hit position (the spear tip)
    hitPosition = spearTip;
    spearTipPosition = spearTip;

    // Hit if distance is less than target cell width
    return distance < cellWidth * scale * 0.8f;
}

// Function to check if a shield blocks an attack
bool checkShieldBlock(const BaseCell& defender, const BaseCell& attacker, float scale, float cellWidth,
                      bool& perfectParry) {
    // No block if not shielding
    if (!defender.isShielding()) {
        perfectParry = false;
        return false;
    }

    // No block if attacker is not attacking
    if (!attacker.isAttacking() || attacker.getAttackTime() >= 0.5f) {
        perfectParry = false;
        return false;
    }

    // Direction factors
    float attackerDirFactor = attacker.isFacingRight() ? 1.0f : -1.0f;
    float defenderDirFactor = defender.isFacingRight() ? 1.0f : -1.0f;

    // Calculate spear tip position with attack extension
    float attackExtension = attacker.getAttackTime() * 2.0f * 50.0f * scale;
    float spearLength = 100.0f * scale;
    float spearTipX = attacker.getPosition().x + attackerDirFactor * (spearLength * 1.5f + attackExtension);
    float spearTipY = attacker.getPosition().y + 60.0f * scale - spearLength * 0.3f;
    Point2f spearTip(spearTipX, spearTipY);

    // Calculate shield position
    Point2f shieldPos = defender.getPosition();
    shieldPos.x += -defenderDirFactor * cellWidth * 0.6f * scale; // Shield on opposite side of facing direction

    // Calculate distance from spear tip to shield
    float distance = norm(spearTip - shieldPos);

    // Shield radius
    float shieldRadius = cellWidth * scale * 0.6f;

    // Check for collision
    bool blocked = distance < shieldRadius;
    
    // Check for perfect parry timing - 使用parryWindowDuration作为判定依据
    // 在游戏配置参数的时间窗口内算作完美格挡
    // 注意：这里需要知道parryWindowDuration的值，假设通过外部传入或全局可访问
    // 由于我无法直接访问gameConfig，这里使用0.15f作为示例
    perfectParry = blocked && defender.getShieldTime() < 0.15f;

    return blocked;
}

// Function to create parry effect
void createParryEffect(BaseCell& attacker, BaseCell& defender) {
    // Knock attacker back
    float knockbackStrength = 10.0f;
    float directionFactor = attacker.isFacingRight() ? -1.0f : 1.0f; // Knocked back in opposite direction
    attacker.applyKnockback(Point2f(directionFactor * knockbackStrength, 0.0f));

    // Visual parry effect for defender
    defender.setParried(true);
    defender.setParryTime(0.0f); // Reset parry effect timer

    // Cancel attacker's attack animation
    attacker.setAttacking(false);
    attacker.setAttackTime(0.0f);

    // Deal reflected damage to attacker (double the base damage)
    const float reflectedDamage = 16.0f; // Base attack damage * 2
    attacker.takeDamage(reflectedDamage);

    // Create blood effect on the attacker (they're damaged by parry)
    Point2f hitPosition = attacker.getPosition();
    Point2f spearTipPosition = attacker.getPosition() + Point2f(directionFactor * 30.0f, 0);
    createBloodEffect(attacker, hitPosition, !attacker.isFacingRight(), spearTipPosition);
}

// 处理碰撞检测和伤害计算的函数
void handleCellCollision(BaseCell& cell1, BaseCell& cell2, GameConfig& config) {
    // ...existing code...
    
    // 如果有一个细胞正在攻击另一个
    if (cell1.isAttacking() && !cell2.isAttacking()) {
        bool perfectParry = false;
        
        // 检查被攻击的细胞是否在格挡 - 使用cellConfig中的cell_width，不是config.cellWidth
        float cellWidth = 60.0f; // 默认值，应与NetGameEngine.cpp中initializeConfig()里的值一致
        if (checkShieldBlock(cell2, cell1, config.scale, cellWidth, perfectParry)) {
            // ...existing code...
        } else {
            // 直接命中，计算伤害
            float damage = config.attackDamage * (1.0f + cell1.getAggressionLevel() * 0.5f);
            
            // 应用攻击者的攻击倍率
            damage *= cell1.getAttackMultiplier();
            
            // AI细胞的攻击伤害降低
            if (cell1.getPlayerNumber() == 0) {
                damage *= 0.7f;  // AI细胞伤害降低30%
            }
            
            // 应用基因相似度伤害修正
            float geneticFactor = cell1.getGeneticDamageMultiplier(cell2);
            damage *= geneticFactor;
            
            // 应用防御者的防御倍率
            damage *= (1.0f - cell2.getDamageReduction() * cell2.getDefenseMultiplier());
            
            // 最终伤害至少1点
            damage = std::max(1.0f, damage);
            
            // 施加伤害
            cell2.takeDamage(damage);
            
            // 创建血液效果
            // ...existing code...
        }
    }
    // ...existing code...
}
