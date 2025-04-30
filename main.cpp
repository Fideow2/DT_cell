#include <opencv2/opencv.hpp>
#include <iostream>
#include "GameEngine.h"

int main() {
    try {
        // 创建游戏引擎
        GameEngine engine;
        
        // 运行游戏循环
        engine.run();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
