#include "GameConfig.h"

GameConfig::GameConfig() {
    // 初始化游戏参数
    maxSpeed = 6.0f;
    accelerationStep = 0.7f;
    drag = 0.94f;
    numCells = 20;
    scale = 0.4f;

    // 攻击和防御参数
    attackDuration = 0.5f;
    attackDamage = 8.0f;
    parryWindowDuration = 0.15f;
    shieldCooldown = 0.3f;
    shieldDuration = 2.0f;
    damageReduction = 0.5f;

    // AI参数
    randomMoveProbability = 0.05f;
    randomMoveStrength = 0.3f;
    aggressionChangeProbability = 0.01f;
    aggressionChangeAmount = 0.1f;
    maxAggression = 1.0f;
    minAggression = 0.0f;
}