#ifndef ENTITY_H
#define ENTITY_H

#include <opencv2/opencv.hpp>
#include <map>
#include <vector>
#include "../GameConfig.h"

// 实体基本接口，所有游戏对象的基类
class Entity {
public:
    virtual ~Entity() {}
    
    // 核心生命周期方法
    virtual void update(float deltaTime, const GameConfig& config, const cv::Size& canvasSize) = 0;
    virtual void render(cv::Mat& canvas, const std::map<std::string, float>& config, float scale, float time) = 0;
    
    // 基本状态查询
    virtual bool isAlive() const = 0;
    virtual cv::Point2f getPosition() const = 0;
};

#endif // ENTITY_H