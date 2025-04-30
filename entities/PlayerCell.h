#ifndef PLAYER_CELL_H
#define PLAYER_CELL_H

#include "BaseCell.h"

// 玩家控制的细胞类，处理玩家输入逻辑
class PlayerCell : public BaseCell {
public:
    PlayerCell(const cv::Point2f& pos, int playerNum, const cv::Vec3b& baseColor,
              float phaseOffset, float aggression = 0.0f);
    
    // 玩家输入处理方法
    void moveUp(float accelerationStep);
    void moveDown(float accelerationStep);
    void moveLeft(float accelerationStep);
    void moveRight(float accelerationStep);
    void attack();
    void toggleShield(float cooldown);
    void increaseAggression(float amount);
    void decreaseAggression(float amount);
};

#endif // PLAYER_CELL_H