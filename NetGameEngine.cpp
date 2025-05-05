#include "NetGameEngine.h"
#include "drawing.h"
#include "physics.h"
#include "entities/PlayerCell.h"
#include "entities/AICell.h"
#include <chrono>
#include <algorithm>
#include <iostream>

NetGameEngine::NetGameEngine()
    : running(true), canvasSize(800, 600), scale(0.4f),
      startTime(std::chrono::high_resolution_clock::now()),
      lastUpdateTime(startTime),
      lastNetworkUpdateTime(startTime),
      networkUpdateInterval(1.0f / 30.0f), // 30Hz网络更新频率
      gameMode(NetGameMode::STANDALONE),
      networkInitialized(false),
      localPlayer(nullptr),
      remotePlayer(nullptr),
      windowTitle("Multi-Cell Network Simulation") {
    
    // 初始化游戏配置
    initializeConfig();
    
    // 加载资源
    loadShieldImage();
}

NetGameEngine::~NetGameEngine() {
    if (networkManager) {
        networkManager->shutdown();
    }
}

bool NetGameEngine::initialize(NetGameMode mode, const std::string& serverIP, int port) {
    gameMode = mode;
    
    // 根据游戏模式创建相应的网络管理器
    if (mode == NetGameMode::SERVER) {
        networkManager = std::make_unique<NetworkServer>(port);
        windowTitle = "Multi-Cell Server - Port: " + std::to_string(port);
    }
    else if (mode == NetGameMode::CLIENT) {
        networkManager = std::make_unique<NetworkClient>(serverIP, port);
        windowTitle = "Multi-Cell Client - Server: " + serverIP + ":" + std::to_string(port);
    }
    else {
        windowTitle = "Multi-Cell Standalone";
    }
    
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 创建玩家
    createPlayers();
    
    // 如果是单机模式，创建AI细胞
    if (mode == NetGameMode::STANDALONE) {
        createAICells();
    }
    
    // 如果是网络模式，初始化网络
    if (mode != NetGameMode::STANDALONE) {
        networkInitialized = networkManager->initialize();
        
        // 设置消息回调
        networkManager->setMessageCallback([this](const NetworkMessage& msg) {
            this->onNetworkMessage(msg);
        });
        
        // 如果是服务器，开始监听
        if (mode == NetGameMode::SERVER && networkInitialized) {
            NetworkServer* server = dynamic_cast<NetworkServer*>(networkManager.get());
            if (server) {
                server->startListening();
                std::cout << "服务器已启动，等待客户端连接..." << std::endl;
            }
        }
        
        // 如果是客户端，连接到服务器
        if (mode == NetGameMode::CLIENT && networkInitialized) {
            NetworkClient* client = dynamic_cast<NetworkClient*>(networkManager.get());
            if (client) {
                if (!client->connectToServer()) {
                    std::cerr << "无法连接到服务器" << std::endl;
                    return false;
                }
                
                // 发送连接请求
                NetworkMessage connectMsg(MessageType::CONNECT_REQUEST, {});
                client->sendMessage(connectMsg);
                
                std::cout << "已连接到服务器" << std::endl;
            }
        }
        
        return networkInitialized;
    }
    
    return true;
}

void NetGameEngine::run() {
    while (running) {
        try {
            // 计算帧时间
            updateFrameTime();
            
            // 创建画布
            cv::Mat canvas = cv::Mat(canvasSize, CV_8UC3, cv::Scalar(255, 255, 255));
            
            // 处理网络消息
            if (networkInitialized) {
                processNetworkMessages();
            }
            
            // 更新所有实体
            updateEntities();
            
            // 处理玩家攻击和碰撞检测
            handleCombat();
            
            // 发送本地玩家状态（如果是网络模式）
            if (networkInitialized && localPlayer && 
                std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - lastNetworkUpdateTime).count() >= networkUpdateInterval) {
                sendPlayerState();
                lastNetworkUpdateTime = std::chrono::high_resolution_clock::now();
            }
            
            // 绘制所有实体
            renderEntities(canvas);
            
            // 显示控制提示和网络状态
            displayControls(canvas);
            
            // 显示游戏模式
            std::string modeText;
            if (gameMode == NetGameMode::SERVER) {
                modeText = "服务器模式";
                NetworkServer* server = dynamic_cast<NetworkServer*>(networkManager.get());
                if (server && server->hasClient()) {
                    modeText += " - 客户端已连接";
                } else {
                    modeText += " - 等待客户端连接";
                }
            }
            else if (gameMode == NetGameMode::CLIENT) {
                modeText = "客户端模式";
                NetworkClient* client = dynamic_cast<NetworkClient*>(networkManager.get());
                if (client && client->isConnected()) {
                    modeText += " - 已连接";
                } else {
                    modeText += " - 未连接";
                }
            }
            else {
                modeText = "单机模式";
            }
            
            cv::putText(canvas, modeText,
                       cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            
            // 显示画布
            cv::imshow(windowTitle, canvas);
            
            // 处理用户输入
            handleInput();
        }
        catch (const cv::Exception& e) {
            std::cerr << "OpenCV错误: " << e.what() << std::endl;
            continue;
        }
        catch (const std::exception& e) {
            std::cerr << "错误: " << e.what() << std::endl;
            continue;
        }
    }
    
    cv::destroyAllWindows();
}

void NetGameEngine::initializeConfig() {
    // 基本游戏参数
    gameConfig.maxSpeed = 6.0f;
    // 减小加速度步长使移动更平滑
    gameConfig.accelerationStep = 0.5f;  // 从0.7f减小到0.5f
    // 调整阻力使移动更丝滑
    gameConfig.drag = 0.96f;  // 从0.94f调整到0.96f
    gameConfig.numCells = 5; // 联机模式下减少AI数量
    gameConfig.scale = 0.4f;
    
    // 攻击和防御参数
    gameConfig.attackDuration = 0.5f;
    gameConfig.attackDamage = 8.0f;
    gameConfig.parryWindowDuration = 0.15f;
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
        {"cell_width", 80.f},  // 从100.f减小到80.f
        {"cell_height", 48.f}, // 从60.f减小到48.f
        {"eye_size", 16.f},    // 从20.f减小到16.f，保持比例
        {"eye_ecc", 0.1f}, {"eye_angle", 15.f},
        {"eye_y_off", 0.2f}, {"eye_x_off", 0.5f},
        {"mouth_x0", 0.6f}, {"mouth_y0", 0.55f},
        {"mouth_x1", 0.7f}, {"mouth_y1", 0.65f},
        {"mouth_x2", 0.85f}, {"mouth_y2", 0.55f},
        {"mouth_width", 2.5f}, // 从3.f减小到2.5f，保持比例
        {"tail_width", 2.5f}   // 从3.f减小到2.5f，保持比例
    };
}

void NetGameEngine::createPlayers() {
    // 根据游戏模式创建玩家
    if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::STANDALONE) {
        // 服务器控制玩家1
        cv::Vec3b player1Color(200, 230, 255); // 蓝色
        PlayerCell* player1 = new PlayerCell(
            cv::Point2f(canvasSize.width/3, canvasSize.height/2),
            1, player1Color, 0.0f, 0.0f
        );
        player1->setShieldDuration(gameConfig.shieldDuration);
        player1->setDamageReduction(gameConfig.damageReduction);
        entities.push_back(std::shared_ptr<BaseCell>(player1));
        playerCells.push_back(player1);
        localPlayer = player1;
        
        // 创建远程玩家2（客户端控制）
        cv::Vec3b player2Color(200, 255, 220); // 绿色
        PlayerCell* player2 = new PlayerCell(
            cv::Point2f(canvasSize.width*2/3, canvasSize.height/2),
            2, player2Color, 0.0f, 0.0f
        );
        player2->setShieldDuration(gameConfig.shieldDuration);
        player2->setDamageReduction(gameConfig.damageReduction);
        entities.push_back(std::shared_ptr<BaseCell>(player2));
        playerCells.push_back(player2);
        remotePlayer = player2;
    }
    else if (gameMode == NetGameMode::CLIENT) {
        // 客户端控制玩家2
        cv::Vec3b player2Color(200, 255, 220); // 绿色
        PlayerCell* player2 = new PlayerCell(
            cv::Point2f(canvasSize.width*2/3, canvasSize.height/2),
            2, player2Color, 0.0f, 0.0f
        );
        player2->setShieldDuration(gameConfig.shieldDuration);
        player2->setDamageReduction(gameConfig.damageReduction);
        entities.push_back(std::shared_ptr<BaseCell>(player2));
        playerCells.push_back(player2);
        localPlayer = player2;
        
        // 创建远程玩家1（服务器控制）
        cv::Vec3b player1Color(200, 230, 255); // 蓝色
        PlayerCell* player1 = new PlayerCell(
            cv::Point2f(canvasSize.width/3, canvasSize.height/2),
            1, player1Color, 0.0f, 0.0f
        );
        player1->setShieldDuration(gameConfig.shieldDuration);
        player1->setDamageReduction(gameConfig.damageReduction);
        entities.push_back(std::shared_ptr<BaseCell>(player1));
        playerCells.push_back(player1);
        remotePlayer = player1;
    }
    else {
        // 单机模式，创建两个玩家
        cv::Vec3b player1Color(200, 230, 255); // 蓝色
        PlayerCell* player1 = new PlayerCell(
            cv::Point2f(canvasSize.width/3, canvasSize.height/2),
            1, player1Color, 0.0f, 0.0f
        );
        player1->setShieldDuration(gameConfig.shieldDuration);
        player1->setDamageReduction(gameConfig.damageReduction);
        entities.push_back(std::shared_ptr<BaseCell>(player1));
        playerCells.push_back(player1);
        
        cv::Vec3b player2Color(200, 255, 220); // 绿色
        PlayerCell* player2 = new PlayerCell(
            cv::Point2f(canvasSize.width*2/3, canvasSize.height/2),
            2, player2Color, 0.0f, 0.0f
        );
        player2->setShieldDuration(gameConfig.shieldDuration);
        player2->setDamageReduction(gameConfig.damageReduction);
        entities.push_back(std::shared_ptr<BaseCell>(player2));
        playerCells.push_back(player2);
    }
}

void NetGameEngine::createAICells() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(0, canvasSize.width);
    std::uniform_real_distribution<float> yDist(0, canvasSize.height);
    std::uniform_real_distribution<float> aggressionDist(0, gameConfig.maxAggression);
    std::uniform_int_distribution<int> colorDist(-30, 30);
    std::uniform_real_distribution<float> phaseDist(0, 2 * CV_PI);
    
    cv::Vec3b baseColor(200, 230, 255);
    
    // 创建AI控制的细胞
    for (int i = 0; i < gameConfig.numCells; ++i) {
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

void NetGameEngine::updateFrameTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    time = std::chrono::duration<float>(currentTime - startTime).count();
    deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
    lastUpdateTime = currentTime;
    
    // 更新盾牌冷却时间
    for (auto player : playerCells) {
        player->updateShieldCooldown(deltaTime);
    }
}

void NetGameEngine::updateEntities() {
    for (auto& entity : entities) {
        if (entity->isAlive() || !entity->getBloodDrops().empty()) {
            // 如果是网络模式，只更新本地玩家和AI
            if ((gameMode == NetGameMode::SERVER || gameMode == NetGameMode::CLIENT) &&
                entity.get() == remotePlayer) {
                // 远程玩家的状态由网络更新，只更新血液滴
                entity->update(deltaTime, gameConfig, canvasSize);
            }
            else {
                entity->update(deltaTime, gameConfig, canvasSize);
            }
        }
    }
}

void NetGameEngine::handleCombat() {
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

void NetGameEngine::handleHit(BaseCell* attacker, BaseCell* target, 
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

void NetGameEngine::renderEntities(cv::Mat& canvas) {
    for (auto& entity : entities) {
        if (entity->isAlive() || !entity->getBloodDrops().empty()) {
            entity->render(canvas, cellConfig, gameConfig.scale, time);
        }
    }
}

void NetGameEngine::displayControls(cv::Mat& canvas) {
    // 显示控制提示，统一控制方式
    if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::STANDALONE) {
        cv::putText(canvas,
                   "Player 1 (本地): WASD to move, J to attack, K to shield, Q/E to change aggression",
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    
    if (gameMode == NetGameMode::CLIENT) {
        cv::putText(canvas,
                   "Player 2 (本地): WASD to move, J to attack, K to shield, Q/E to change aggression",
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    
    if (gameMode == NetGameMode::STANDALONE) {
        cv::putText(canvas,
                   "Player 2 (本地): WASD to move, J to attack, K to shield, Q/E to change aggression",
                   cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    
    // 在服务器模式下还要显示远程玩家2的控制方式
    if (gameMode == NetGameMode::SERVER) {
        cv::putText(canvas,
                   "Player 2 (远程): 由客户端控制",
                   cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    
    // 在客户端模式下还要显示远程玩家1的控制方式
    if (gameMode == NetGameMode::CLIENT) {
        cv::putText(canvas,
                   "Player 1 (远程): 由服务器控制",
                   cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    
    // 显示盾牌状态
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

void NetGameEngine::handleInput() {
    int key = cv::waitKey(16); // 等待16毫秒（大约60FPS）
    
    if (key == 27) { // ESC键
        running = false;
        return;
    }
    
    // 记录输入状态
    PlayerInputMessage inputMsg;
    
    // 根据游戏模式处理输入
    if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::CLIENT) {
        // 统一玩家控制方式，均使用WASD + J/K
        if (key == 'w' || key == 'W') {
            localPlayer->moveUp(gameConfig.accelerationStep);
            inputMsg.moveUp = true;
        }
        if (key == 's' || key == 'S') {
            localPlayer->moveDown(gameConfig.accelerationStep);
            inputMsg.moveDown = true;
        }
        if (key == 'a' || key == 'A') {
            localPlayer->moveLeft(gameConfig.accelerationStep);
            inputMsg.moveLeft = true;
        }
        if (key == 'd' || key == 'D') {
            localPlayer->moveRight(gameConfig.accelerationStep);
            inputMsg.moveRight = true;
        }
        if (key == 'q' || key == 'Q') {
            localPlayer->decreaseAggression(0.1f);
            inputMsg.decreaseAggression = true;
        }
        if (key == 'e' || key == 'E') {
            localPlayer->increaseAggression(0.1f);
            inputMsg.increaseAggression = true;
        }
        if ((key == 'j' || key == 'J') && !localPlayer->isShielding()) {
            localPlayer->attack();
            inputMsg.attack = true;
        }
        if ((key == 'k' || key == 'K') && localPlayer->canToggleShield()) {
            localPlayer->toggleShield(gameConfig.shieldCooldown);
            inputMsg.shield = true;
        }
    }
    
    // 单机模式需要处理两个玩家的输入
    if (gameMode == NetGameMode::STANDALONE) {
        // 玩家1控制
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
        if ((key == 'j' || key == 'J') && !playerCells[0]->isShielding()) {
            playerCells[0]->attack();
        }
        if ((key == 'k' || key == 'K') && playerCells[0]->canToggleShield()) {
            playerCells[0]->toggleShield(gameConfig.shieldCooldown);
        }
        
        // 玩家2也使用相同的控制方式，但使用方向键和其他按键
        PlayerCell* player2 = playerCells[1];
        
        if (key == 82 || key == 0x260000 || key == 63232) { // UP arrow (macOS: 63232)
            player2->moveUp(gameConfig.accelerationStep);
        }
        if (key == 84 || key == 0x280000 || key == 63233) { // DOWN arrow (macOS: 63233)
            player2->moveDown(gameConfig.accelerationStep);
        }
        if (key == 81 || key == 0x250000 || key == 63234) { // LEFT arrow (macOS: 63234)
            player2->moveLeft(gameConfig.accelerationStep);
        }
        if (key == 83 || key == 0x270000 || key == 63235) { // RIGHT arrow (macOS: 63235)
            player2->moveRight(gameConfig.accelerationStep);
        }
        if ((key == '/' || key == 47) && !player2->isShielding()) {
            player2->attack();
        }
        if ((key == '.' || key == 46) && player2->canToggleShield()) {
            player2->toggleShield(gameConfig.shieldCooldown);
        }
    }
    
    // 如果是网络模式且有输入，发送输入消息
    if ((gameMode == NetGameMode::SERVER || gameMode == NetGameMode::CLIENT) && 
        networkInitialized && (inputMsg.moveUp || inputMsg.moveDown || 
                               inputMsg.moveLeft || inputMsg.moveRight || 
                               inputMsg.attack || inputMsg.shield || 
                               inputMsg.increaseAggression || inputMsg.decreaseAggression)) {
        
        // 序列化输入消息
        std::vector<uint8_t> inputData = NetworkSerializer::serializePlayerInput(inputMsg);
        
        // 发送输入消息
        NetworkMessage netMsg(MessageType::PLAYER_INPUT, inputData);
        networkManager->sendMessage(netMsg);
    }
}

void NetGameEngine::processNetworkMessages() {
    if (!networkInitialized) return;
    
    NetworkMessage msg;
    while (networkManager->receiveMessage(msg)) {
        onNetworkMessage(msg);
    }
}

void NetGameEngine::onNetworkMessage(const NetworkMessage& msg) {
    switch (msg.type) {
        case MessageType::CONNECT_REQUEST:
            // 客户端请求连接
            if (gameMode == NetGameMode::SERVER) {
                // 发送接受连接的响应
                NetworkMessage acceptMsg(MessageType::CONNECT_ACCEPT, {});
                networkManager->sendMessage(acceptMsg);
                std::cout << "客户端已连接，发送接受连接响应" << std::endl;
            }
            break;
            
        case MessageType::CONNECT_ACCEPT:
            // 服务器接受连接
            if (gameMode == NetGameMode::CLIENT) {
                std::cout << "服务器已接受连接" << std::endl;
            }
            break;
            
        case MessageType::PLAYER_INPUT:
            // 接收玩家输入
            if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::CLIENT) {
                PlayerInputMessage inputMsg = NetworkSerializer::deserializePlayerInput(msg.data);
                handlePlayerInputMessage(inputMsg);
            }
            break;
            
        case MessageType::PLAYER_STATE:
            // 接收玩家状态
            if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::CLIENT) {
                PlayerStateMessage stateMsg = NetworkSerializer::deserializePlayerState(msg.data);
                handlePlayerStateMessage(stateMsg);
            }
            break;
            
        case MessageType::PING:
            // 收到Ping，回应Pong
            {
                NetworkMessage pongMsg(MessageType::PONG, {});
                networkManager->sendMessage(pongMsg);
            }
            break;
            
        case MessageType::PONG:
            // 收到Pong，不做处理
            break;
            
        case MessageType::DISCONNECT:
            // 对方断开连接
            std::cout << "对方断开连接" << std::endl;
            break;
            
        default:
            std::cerr << "收到未知消息类型: " << static_cast<int>(msg.type) << std::endl;
            break;
    }
}

void NetGameEngine::handlePlayerInputMessage(const PlayerInputMessage& inputMsg) {
    // 根据游戏模式应用输入
    if (gameMode == NetGameMode::SERVER && remotePlayer) {
        // 服务器接收到客户端输入，应用到玩家2
        if (inputMsg.moveUp) {
            remotePlayer->moveUp(gameConfig.accelerationStep);
        }
        if (inputMsg.moveDown) {
            remotePlayer->moveDown(gameConfig.accelerationStep);
        }
        if (inputMsg.moveLeft) {
            remotePlayer->moveLeft(gameConfig.accelerationStep);
        }
        if (inputMsg.moveRight) {
            remotePlayer->moveRight(gameConfig.accelerationStep);
        }
        if (inputMsg.decreaseAggression) {
            remotePlayer->decreaseAggression(0.1f);
        }
        if (inputMsg.increaseAggression) {
            remotePlayer->increaseAggression(0.1f);
        }
        if (inputMsg.attack && !remotePlayer->isShielding()) {
            remotePlayer->attack();
        }
        if (inputMsg.shield && remotePlayer->canToggleShield()) {
            remotePlayer->toggleShield(gameConfig.shieldCooldown);
        }
    }
    else if (gameMode == NetGameMode::CLIENT && remotePlayer) {
        // 客户端接收到服务器输入，应用到玩家1
        if (inputMsg.moveUp) {
            remotePlayer->moveUp(gameConfig.accelerationStep);
        }
        if (inputMsg.moveDown) {
            remotePlayer->moveDown(gameConfig.accelerationStep);
        }
        if (inputMsg.moveLeft) {
            remotePlayer->moveLeft(gameConfig.accelerationStep);
        }
        if (inputMsg.moveRight) {
            remotePlayer->moveRight(gameConfig.accelerationStep);
        }
        if (inputMsg.decreaseAggression) {
            remotePlayer->decreaseAggression(0.1f);
        }
        if (inputMsg.increaseAggression) {
            remotePlayer->increaseAggression(0.1f);
        }
        if (inputMsg.attack && !remotePlayer->isShielding()) {
            remotePlayer->attack();
        }
        if (inputMsg.shield && remotePlayer->canToggleShield()) {
            remotePlayer->toggleShield(gameConfig.shieldCooldown);
        }
    }
}

void NetGameEngine::sendPlayerState() {
    if (!networkInitialized || !localPlayer) return;
    
    // 获取本地玩家状态
    PlayerStateMessage stateMsg = NetworkSerializer::getPlayerStateFromCell(*localPlayer);
    
    // 序列化状态消息
    std::vector<uint8_t> stateData = NetworkSerializer::serializePlayerState(stateMsg);
    
    // 发送状态消息
    NetworkMessage netMsg(MessageType::PLAYER_STATE, stateData);
    networkManager->sendMessage(netMsg);
}

void NetGameEngine::handlePlayerStateMessage(const PlayerStateMessage& stateMsg) {
    // 根据游戏模式应用状态
    if ((gameMode == NetGameMode::SERVER || gameMode == NetGameMode::CLIENT) && remotePlayer) {
        // 将远程玩家状态应用到对应的Cell
        NetworkSerializer::applyPlayerStateToCell(*remotePlayer, stateMsg);
    }
}