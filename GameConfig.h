#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

// 游戏配置结构体，集中管理所有游戏参数
struct GameConfig {
    // 基本游戏参数
    float scale;
    int numCells;
    float maxSpeed;
    float accelerationStep;
    float drag;
    
    // 攻击和防御参数
    float attackDuration;
    float attackDamage;
    float parryWindowDuration;
    float shieldCooldown;
    float shieldDuration;
    float damageReduction;
    
    // AI行为参数
    float randomMoveProbability;
    float randomMoveStrength;
    float aggressionChangeProbability;
    float aggressionChangeAmount;
    float maxAggression;
    float minAggression;
};

#endif // GAME_CONFIG_H