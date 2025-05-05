#ifndef AI_CELL_H
#define AI_CELL_H

#include "BaseCell.h"
#include <random>

// AI控制的细胞类，处理自主行为
class AICell : public BaseCell {
public:
    AICell(const cv::Point2f& pos, int id, const cv::Vec3b& baseColor,
           float phaseOffset, float aggression = 0.0f, const std::string& cellGene = "");
    
    // 重写更新方法以实现AI行为
    void update(float deltaTime, const GameConfig& config, const cv::Size& canvasSize) override;
    
private:
    // AI行为控制
    void updateAIBehavior(float deltaTime, const GameConfig& config);
    
    // 随机数生成器
    std::mt19937 gen;
    std::uniform_real_distribution<float> moveDist;
    std::uniform_real_distribution<float> probDist;
};

#endif // AI_CELL_H