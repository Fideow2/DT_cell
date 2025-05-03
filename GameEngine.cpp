#include "GameEngine.h"
#include "drawing.h"
#include "physics.h"
#include "entities/PlayerCell.h"
#include "entities/AICell.h"
#include <chrono>
#include <algorithm>

GameEngine::GameEngine() 
    : running(true), canvasSize(800, 600), scale(0.4f),
      startTime(std::chrono::high_resolution_clock::now()),
      lastUpdateTime(startTime) {
    
    // 初始化游戏配置
    initializeConfig();
    
    // 加载资源
    loadShieldImage();
    
    // 初始化随机数生成器
    initializeRandomGenerators();
    
    // 创建玩家和AI细胞
    createPlayers();
    createAICells();
}

void GameEngine::run() {
    while (running) {
        try {
            // 计算帧时间
            updateFrameTime();
            
            // 创建画布
            cv::Mat canvas = cv::Mat(canvasSize, CV_8UC3, cv::Scalar(255, 255, 255));
            
            // 更新所有实体
            updateEntities();
            
            // 处理玩家攻击和碰撞检测
            handleCombat();
            
            // 绘制所有实体
            renderEntities(canvas);
            
            // 显示控制提示
            displayControls(canvas);
            
            // 显示画布
            cv::imshow("Multi-Cell Simulation", canvas);
            
            // 处理用户输入
            handleInput();
        }
        catch (const cv::Exception& e) {
            std::cerr << "OpenCV error: " << e.what() << std::endl;
            continue;
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            continue;
        }
    }
    
    cv::destroyAllWindows();
}

void GameEngine::initializeConfig() {
    // 基本游戏参数
    gameConfig.maxSpeed = 6.0f;
    gameConfig.accelerationStep = 0.7f;
    gameConfig.drag = 0.94f;
    gameConfig.numCells = 20;
    gameConfig.scale = 0.4f;
    
    // 攻击和防御参数
    gameConfig.attackDuration = 0.5f;
    gameConfig.attackDamage = 8.0f;
    gameConfig.parryWindowDuration = 0.15f;  // 增大完美格挡窗口时间，从0.05秒到0.15秒
    gameConfig.shieldCooldown = 0.3f;
    gameConfig.shieldDuration = 2.0f;
    gameConfig.damageReduction = 0.5f;
    
    // AI参数
    gameConfig.randomMoveProbability = 0.05f;
    gameConfig.randomMoveStrength = 0.3f;
    gameConfig.aggressionChangeProbability = 0.01f;
    gameConfig.aggressionChangeAmount = 0.1f;
    gameConfig.maxAggression = 1.0f;
    gameConfig.minAggression = 0.0f;
    
    // 细胞渲染配置
    cellConfig = {
        {"cell_width", 100.f}, {"cell_height", 60.f},
        {"eye_size", 20.f}, {"eye_ecc", 0.1f}, {"eye_angle", 15.f},
        {"eye_y_off", 0.2f}, {"eye_x_off", 0.5f},
        {"mouth_x0", 0.6f}, {"mouth_y0", 0.55f},
        {"mouth_x1", 0.7f}, {"mouth_y1", 0.65f},
        {"mouth_x2", 0.85f}, {"mouth_y2", 0.55f},
        {"mouth_width", 3.f},
        {"tail_width", 3.f}
    };
}

void GameEngine::initializeRandomGenerators() {
    gen = std::mt19937(rd());
    xDist = std::uniform_real_distribution<float>(0, canvasSize.width);
    yDist = std::uniform_real_distribution<float>(0, canvasSize.height);
    moveDist = std::uniform_real_distribution<float>(-gameConfig.randomMoveStrength, gameConfig.randomMoveStrength);
    probDist = std::uniform_real_distribution<float>(0, 1);
    aggressionDist = std::uniform_real_distribution<float>(0, gameConfig.maxAggression);
    colorDist = std::uniform_int_distribution<int>(-30, 30);
    phaseDist = std::uniform_real_distribution<float>(0, 2 * CV_PI);
}

void GameEngine::createPlayers() {
    // 创建玩家1
    cv::Vec3b baseColor(200, 230, 255);
    PlayerCell* player1 = new PlayerCell(
        cv::Point2f(canvasSize.width/3, canvasSize.height/2),
        1, baseColor, 0.0f, 0.0f
    );
    player1->setShieldDuration(gameConfig.shieldDuration);
    player1->setDamageReduction(gameConfig.damageReduction);
    entities.push_back(std::shared_ptr<BaseCell>(player1));
    playerCells.push_back(player1);
    
    // 创建玩家2
    cv::Vec3b player2Color(200, 255, 220);
    PlayerCell* player2 = new PlayerCell(
        cv::Point2f(canvasSize.width*2/3, canvasSize.height/2),
        2, player2Color, phaseDist(gen), 0.0f
    );
    player2->setShieldDuration(gameConfig.shieldDuration);
    player2->setDamageReduction(gameConfig.damageReduction);
    entities.push_back(std::shared_ptr<BaseCell>(player2));
    playerCells.push_back(player2);
}

void GameEngine::createAICells() {
    cv::Vec3b baseColor(200, 230, 255);
    
    // 创建AI控制的细胞
    for (int i = 2; i < gameConfig.numCells; ++i) {
        cv::Vec3b cellColor = baseColor;
        cellColor[0] = cv::saturate_cast<uchar>(baseColor[0] + colorDist(gen));
        cellColor[1] = cv::saturate_cast<uchar>(baseColor[1] + colorDist(gen));
        cellColor[2] = cv::saturate_cast<uchar>(baseColor[2] + colorDist(gen));
        
        float phaseOffset = phaseDist(gen);
        float aggression = aggressionDist(gen);
        
        auto aiCell = new AICell(
            cv::Point2f(xDist(gen), yDist(gen)),
            0, cellColor, phaseOffset, aggression
        );
        
        entities.push_back(std::shared_ptr<BaseCell>(aiCell));
    }
}

void GameEngine::updateFrameTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    time = std::chrono::duration<float>(currentTime - startTime).count();
    deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
    lastUpdateTime = currentTime;
    
    // 更新屏蔽冷却时间
    for (auto player : playerCells) {
        player->updateShieldCooldown(deltaTime);
    }
}

void GameEngine::updateEntities() {
    for (auto& entity : entities) {
        if (entity->isAlive() || !entity->getBloodDrops().empty()) {
            entity->update(deltaTime, gameConfig, canvasSize);
        }
    }
}

void GameEngine::handleCombat() {
    // 检查玩家攻击
    for (auto& attacker : entities) {
        if (attacker->isAttacking() && attacker->getAttackTime() < 0.5f) {
            for (auto& target : entities) {
                if (attacker.get() != target.get() && target->isAlive()) {
                    cv::Point2f hitPosition;
                    cv::Point2f spearTipPosition;
                    
                    bool hit = checkSpearCollision(
                        *attacker, *target, gameConfig.scale, 
                        cellConfig.at("cell_width"), hitPosition, spearTipPosition
                    );
                    
                    if (hit) {
                        handleHit(attacker.get(), target.get(), hitPosition, spearTipPosition);
                    }
                }
            }
        }
    }
}

void GameEngine::handleHit(BaseCell* attacker, BaseCell* target, 
                          const cv::Point2f& hitPosition, 
                          const cv::Point2f& spearTipPosition) {
    bool perfectParry = false;
    
    if (checkShieldBlock(*target, *attacker, gameConfig.scale, 
                        cellConfig.at("cell_width"), perfectParry)) {
        // 攻击被盾牌阻挡
        if (perfectParry) {
            // 完美格挡 - 创建格挡效果，不造成伤害
            createParryEffect(*attacker, *target);
        } else {
            // 普通格挡 - 取消攻击，造成减伤
            attacker->setAttacking(false);
            attacker->setAttackTime(0.0f);
            
            // 应用减伤后的伤害
            float damage = gameConfig.attackDamage * (1.0f + attacker->getAggressionLevel() * 0.5f);
            damage *= (1.0f - target->getDamageReduction());
            target->takeDamage(damage);
            
            // 创建小规模的血液效果
            cv::Point2f reducedHitPos = target->getPosition();
            createBloodEffect(*target, reducedHitPos, attacker->isFacingRight(), spearTipPosition);
        }
    } else {
        // 直接命中，没有格挡
        float damage = gameConfig.attackDamage * (1.0f + attacker->getAggressionLevel() * 0.5f);
        target->takeDamage(damage);
        
        // 创建血液效果
        createBloodEffect(*target, hitPosition, attacker->isFacingRight(), spearTipPosition);
        
        // 添加击退效果
        float knockbackStrength = 5.0f;
        target->applyKnockback(
            cv::Point2f(attacker->isFacingRight() ? 1.0f : -1.0f, -0.5f) * knockbackStrength
        );
    }
}

void GameEngine::renderEntities(cv::Mat& canvas) {
    for (auto& entity : entities) {
        if (entity->isAlive() || !entity->getBloodDrops().empty()) {
            entity->render(canvas, cellConfig, gameConfig.scale, time);
        }
    }
}

void GameEngine::displayControls(cv::Mat& canvas) {
    cv::putText(canvas,
               "Player 1: WASD to move, F to attack, G to shield, Q/E to change aggression",
               cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    
    cv::putText(canvas,
               "Player 2: Arrow keys to move, F to attack, G to shield, Q/E to change aggression",
               cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    
    // 显示屏蔽状态
    for (int i = 0; i < playerCells.size() && i < 2; i++) {
        std::string shieldStatus;
        auto player = playerCells[i];
        
        if (player->isShielding()) {
            float remainingTime = player->getShieldDuration() - player->getShieldTime();
            shieldStatus = "Shield: " + std::to_string(int(remainingTime * 10) / 10.0) + "s";
            if (player->getShieldTime() < gameConfig.parryWindowDuration) {
                shieldStatus += " (PERFECT PARRY)";
            }
        } else if (player->getShieldCooldownTime() > 0) {
            shieldStatus = "Shield CD: " + std::to_string(int(player->getShieldCooldownTime() * 10) / 10.0) + "s";
        } else {
            shieldStatus = "Shield Ready";
        }
        
        cv::putText(canvas, shieldStatus, 
                  cv::Point(10, 70 + i*20), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
}

void GameEngine::handleInput() {
    int key = cv::waitKey(16); // 等待16毫秒（大约60FPS）
    
    if (key == 27) { // ESC键
        running = false;
        return;
    }
    
    // 处理玩家1的输入 (WASD移动, F攻击, G防御, Q/E调整攻击性)
    if (key == 'w' || key == 'W') {
        playerCells[0]->moveUp(gameConfig.accelerationStep);
    }
    if (key == 's' || key == 'S') {
        playerCells[0]->moveDown(gameConfig.accelerationStep);
    }
    if (key == 'a' || key == 'A') {
        playerCells[0]->moveLeft(gameConfig.accelerationStep);
    }
    if (key == 'd' || key == 'D') {
        playerCells[0]->moveRight(gameConfig.accelerationStep);
    }
    if (key == 'q' || key == 'Q') {
        playerCells[0]->decreaseAggression(0.1f);
    }
    if (key == 'e' || key == 'E') {
        playerCells[0]->increaseAggression(0.1f);
    }
    if ((key == 'f' || key == 'F') && !playerCells[0]->isShielding()) {
        playerCells[0]->attack();
    }
    if ((key == 'g' || key == 'G') && playerCells[0]->canToggleShield()) {
        playerCells[0]->toggleShield(gameConfig.shieldCooldown);
    }
    
    // 处理玩家2的输入，仍然使用方向键移动，但使用与玩家1相同的按键方式 (F攻击, G防御, Q/E调整攻击性)
    if (key == 82 || key == 0x260000 || key == 63232) { // UP arrow (macOS: 63232)
        playerCells[1]->moveUp(gameConfig.accelerationStep);
    }
    if (key == 84 || key == 0x280000 || key == 63233) { // DOWN arrow (macOS: 63233)
        playerCells[1]->moveDown(gameConfig.accelerationStep);
    }
    if (key == 81 || key == 0x250000 || key == 63234) { // LEFT arrow (macOS: 63234)
        playerCells[1]->moveLeft(gameConfig.accelerationStep);
    }
    if (key == 83 || key == 0x270000 || key == 63235) { // RIGHT arrow (macOS: 63235)
        playerCells[1]->moveRight(gameConfig.accelerationStep);
    }
    if ((key == 'f' || key == 'F') && !playerCells[1]->isShielding()) {
        playerCells[1]->attack();
    }
    if ((key == 'g' || key == 'G') && playerCells[1]->canToggleShield()) {
        playerCells[1]->toggleShield(gameConfig.shieldCooldown);
    }
}