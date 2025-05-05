#include "MultiPlayerGame.h"
#include "../drawing.h"
#include "../physics.h"
#include "../entities/PlayerCell.h"
#include "../entities/AICell.h"
#include <chrono>
#include <algorithm>
#include <iostream>

MultiPlayerGame::MultiPlayerGame()
    : running(true), canvasSize(800, 600), scale(0.4f),
      startTime(std::chrono::high_resolution_clock::now()),
      lastUpdateTime(startTime),
      lastNetworkUpdateTime(startTime),
      networkUpdateInterval(1.0f / 30.0f), // 30Hz网络更新频率
      gameMode(NetGameMode::STANDALONE),
      networkInitialized(false),
      localPlayer(nullptr),
      remotePlayer(nullptr),
      windowTitle("多细胞网络对战") {
    
    // 初始化游戏配置
    initializeConfig();
    
    // 加载资源
    loadShieldImage();
}

MultiPlayerGame::~MultiPlayerGame() {
    if (networkManager) {
        networkManager->shutdown();
    }
}

bool MultiPlayerGame::initialize(NetGameMode mode, const std::string& serverIP, int port) {
    gameMode = mode;
    
    // 初始化网络管理器
    if (gameMode == NetGameMode::SERVER) {
        // 服务器模式
        networkManager = std::make_unique<NetworkServer>(port);
        windowTitle = "多细胞网络对战 - 服务器模式";
    } 
    else if (gameMode == NetGameMode::CLIENT) {
        // 客户端模式
        networkManager = std::make_unique<NetworkClient>(serverIP, port);
        windowTitle = "多细胞网络对战 - 客户端模式";
    }
    
    // 创建玩家
    createPlayers();
    
    // 创建AI细胞（只在服务器或单机模式下创建）
    if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::STANDALONE) {
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
                if (client->connectToServer()) {
                    std::cout << "成功连接到服务器!" << std::endl;
                } else {
                    std::cout << "连接服务器失败!" << std::endl;
                    return false;
                }
            }
        }
    }
    
    return true;
}

void MultiPlayerGame::run() {
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

void MultiPlayerGame::initializeConfig() {
    // 初始化游戏参数
    gameConfig.maxSpeed = 6.0f;
    gameConfig.accelerationStep = 0.7f;
    gameConfig.drag = 0.94f;
    gameConfig.numCells = 20;
    gameConfig.scale = 0.4f;

    // 攻击和防御参数
    gameConfig.attackDuration = 0.5f;
    gameConfig.attackDamage = 8.0f;
    gameConfig.parryWindowDuration = 0.15f;
    gameConfig.shieldCooldown = 0.3f;
    gameConfig.shieldDuration = 2.0f;
    gameConfig.damageReduction = 0.5f;
    
    // AI参数
    gameConfig.randomMoveProbability = 0.08f; // 提高随机移动概率
    gameConfig.randomMoveStrength = 0.4f;    // 提高随机移动强度
    gameConfig.aggressionChangeProbability = 0.01f;
    gameConfig.aggressionChangeAmount = 0.1f;
    gameConfig.maxAggression = 1.0f;
    gameConfig.minAggression = 0.0f;
    
    // 细胞渲染配置
    cellConfig = {
        {"cell_width", 60.f},  // 从80.f减小到60.f
        {"cell_height", 36.f}, // 从48.f减小到36.f
        {"eye_size", 12.f},    // 从16.f减小到12.f，保持比例
        {"eye_ecc", 0.1f}, {"eye_angle", 15.f},
        {"eye_y_off", 0.2f}, {"eye_x_off", 0.5f},
        {"mouth_x0", 0.6f}, {"mouth_y0", 0.55f},
        {"mouth_x1", 0.7f}, {"mouth_y1", 0.65f},
        {"mouth_x2", 0.85f}, {"mouth_y2", 0.55f},
        {"mouth_width", 2.0f}, // 从2.5f减小到2.0f，保持比例
        {"tail_width", 2.0f}   // 从2.5f减小到2.0f，保持比例
    };
}

void MultiPlayerGame::createPlayers() {
    // 创建两个玩家细胞 - 使用随机基因
    cv::Point2f player1Pos(canvasSize.width * 0.25f, canvasSize.height * 0.5f);
    cv::Point2f player2Pos(canvasSize.width * 0.75f, canvasSize.height * 0.5f);
    
    auto player1 = std::make_shared<PlayerCell>(player1Pos, 1);
    auto player2 = std::make_shared<PlayerCell>(player2Pos, 2);
    
    // 添加玩家到实体列表
    entities.push_back(player1);
    entities.push_back(player2);
    
    // 保存玩家指针以方便访问
    playerCells.push_back(player1.get());
    playerCells.push_back(player2.get());
    
    // 设置本地和远程玩家指针
    if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::STANDALONE) {
        localPlayer = player1.get();
        remotePlayer = player2.get();
    } else {
        localPlayer = player2.get();
        remotePlayer = player1.get();
    }
}

void MultiPlayerGame::createAICells() {
    // 创建AI细胞
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(50.0f, canvasSize.width - 50.0f);
    std::uniform_real_distribution<float> yDist(50.0f, canvasSize.height - 50.0f);
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> aggressionDist(0.0f, 0.5f);
    
    // 创建一些AI细胞添加到游戏中
    for (int i = 0; i < gameConfig.numCells; ++i) {
        cv::Point2f position(xDist(gen), yDist(gen));
        float phase = phaseDist(gen);
        float aggression = aggressionDist(gen);
        
        auto cell = std::make_shared<AICell>(position, 0, cv::Vec3b(0, 0, 0), phase, aggression);
        entities.push_back(cell);
    }
}

void MultiPlayerGame::updateFrameTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
    lastUpdateTime = currentTime;
    time = std::chrono::duration<float>(currentTime - startTime).count();
    
    // 限制deltaTime，防止过大的时间步长
    deltaTime = std::min(deltaTime, 0.1f);
}

void MultiPlayerGame::updateEntities() {
    // 更新所有实体
    for (auto it = entities.begin(); it != entities.end();) {
        (*it)->update(deltaTime, gameConfig, canvasSize);
        
        // 如果实体不存活，移除它
        if (!(*it)->isAlive()) {
            // 检查是否是玩家，如果是则不移除
            if (std::find(playerCells.begin(), playerCells.end(), it->get()) == playerCells.end()) {
                it = entities.erase(it);
            } else {
                // 如果是玩家，则重置它的位置和状态
                (*it)->setPosition(cv::Point2f(canvasSize.width / 2.0f, canvasSize.height / 2.0f));
                BaseCell* cell = dynamic_cast<BaseCell*>(it->get());
                if (cell) {
                    cell->takeDamage(-100.0f); // 恢复健康
                }
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    // 检查细胞繁殖
    checkCellReproduction();
}

void MultiPlayerGame::checkCellReproduction() {
    // 服务器才负责管理繁殖
    if (gameMode != NetGameMode::SERVER && gameMode != NetGameMode::STANDALONE) {
        return;
    }
    
    // 简单的繁殖逻辑：当两个AI细胞靠近时可能会繁殖
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    
    for (size_t i = 0; i < entities.size(); ++i) {
        if (entities.size() >= 50) break; // 限制最大实体数量
        
        // 只有AI细胞可以繁殖
        AICell* cell1 = dynamic_cast<AICell*>(entities[i].get());
        if (!cell1 || cell1->getPlayerNumber() > 0) continue;
        
        for (size_t j = i + 1; j < entities.size(); ++j) {
            AICell* cell2 = dynamic_cast<AICell*>(entities[j].get());
            if (!cell2 || cell2->getPlayerNumber() > 0) continue;
            
            // 检查距离
            float distance = cv::norm(cell1->getPosition() - cell2->getPosition());
            float combinedSize = (cell1->getSizeMultiplier() + cell2->getSizeMultiplier()) * 30.0f;
            
            if (distance < combinedSize) {
                // 繁殖几率
                float breedChance = 0.001f;  // 每帧0.1%的繁殖几率
                if (chanceDist(gen) < breedChance) {
                    // 创建后代
                    BaseCell* offspring = BaseCell::createOffspring(*cell1, *cell2, canvasSize);
                    if (offspring) {
                        auto newCell = std::shared_ptr<BaseCell>(offspring);
                        entities.push_back(newCell);
                        break; // 每个细胞每次检查只繁殖一次
                    }
                }
            }
        }
    }
}

void MultiPlayerGame::handleCombat() {
    // 检查攻击碰撞
    for (auto& attacker : entities) {
        // 只有正在攻击的细胞才能造成伤害
        if (attacker->isAttacking()) {
            for (auto& target : entities) {
                // 不能攻击自己
                if (attacker.get() == target.get()) continue;
                
                // 检查是否命中
                cv::Point2f hitPosition;
                cv::Point2f spearTipPosition;
                if (checkSpearCollision(*attacker, *target, gameConfig.scale, 
                                      cellConfig.at("cell_width"), hitPosition, spearTipPosition)) {
                    // 处理命中效果
                    handleHit(attacker.get(), target.get(), hitPosition, spearTipPosition);
                }
            }
        }
    }
}

void MultiPlayerGame::handleHit(BaseCell* attacker, BaseCell* target, 
                            const cv::Point2f& hitPosition, 
                            const cv::Point2f& spearTipPosition) {
    bool perfectParry = false;
    
    // 计算阵营倍率 - 同一阵营伤害降低，不同阵营伤害提高
    float factionMultiplier = 1.0f;
    if (attacker->getFaction() == target->getFaction() && attacker->getFaction() != 0) {
        factionMultiplier = 0.5f; // 同阵营伤害减半
    } else if (attacker->getFaction() != target->getFaction() && attacker->getFaction() != 0 && target->getFaction() != 0) {
        factionMultiplier = 1.5f; // 敌对阵营伤害增加
    }
    
    // 计算基因相似度影响
    float geneticDamageMultiplier = attacker->getGeneticDamageMultiplier(*target);
    float geneticDefenseMultiplier = target->getGeneticDefenseMultiplier(*attacker);
    
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
            
            // 应用修饰后的伤害
            float damage = gameConfig.attackDamage 
                         * (1.0f + attacker->getAggressionLevel() * 0.5f)
                         * attacker->getSizeMultiplier()  // 大细胞伤害更高
                         * factionMultiplier
                         * geneticDamageMultiplier
                         / geneticDefenseMultiplier;
            
            damage *= (1.0f - target->getDamageReduction());
            target->takeDamage(damage);
            
            // 创建小规模的血液效果
            cv::Point2f reducedHitPos = target->getPosition();
            createBloodEffect(*target, reducedHitPos, attacker->isFacingRight(), spearTipPosition);
        }
    } else {
        // 直接命中，没有格挡
        float damage = gameConfig.attackDamage 
                     * (1.0f + attacker->getAggressionLevel() * 0.5f)
                     * attacker->getSizeMultiplier()  // 大细胞伤害更高
                     * factionMultiplier
                     * geneticDamageMultiplier
                     / geneticDefenseMultiplier;
        
        target->takeDamage(damage);
        
        // 创建血液效果
        createBloodEffect(*target, hitPosition, attacker->isFacingRight(), spearTipPosition);
        
        // 添加击退效果
        float knockbackStrength = 5.0f * attacker->getSizeMultiplier();
        target->applyKnockback(
            cv::Point2f(attacker->isFacingRight() ? 1.0f : -1.0f, -0.5f) * knockbackStrength
        );
    }
}

void MultiPlayerGame::renderEntities(cv::Mat& canvas) {
    // 绘制所有实体
    for (auto& entity : entities) {
        entity->render(canvas, cellConfig, scale, time);
    }
}

void MultiPlayerGame::displayControls(cv::Mat& canvas) {
    // Display control instructions with unified control scheme
    if (gameMode == NetGameMode::SERVER || gameMode == NetGameMode::STANDALONE) {
        cv::putText(canvas,
                   "Player 1 (Local): WASD to move, J to attack, K to defend, Q/E to adjust aggression",
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    
    if (gameMode == NetGameMode::CLIENT) {
        cv::putText(canvas,
                   "Player 2 (Local): WASD to move, J to attack, K to defend, Q/E to adjust aggression",
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
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
    
    // 显示基因和阵营信息
    for (int i = 0; i < playerCells.size() && i < 2; i++) {
        auto player = playerCells[i];
        
        // 显示基因信息
        std::string geneInfo = "玩家 " + std::to_string(i+1) + 
                               " 基因: " + player->getGene() + 
                               " 阵营: " + std::to_string(player->getFaction());
        
        cv::putText(canvas, geneInfo, 
                   cv::Point(10, 130 + i*20), cv::FONT_HERSHEY_SIMPLEX, 
                   0.4, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    
    // 显示实体数量
    std::string entitiesInfo = "实体数量: " + std::to_string(entities.size());
    cv::putText(canvas, entitiesInfo, 
               cv::Point(10, 170), cv::FONT_HERSHEY_SIMPLEX, 
               0.4, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
}

void MultiPlayerGame::handleInput() {
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

void MultiPlayerGame::processNetworkMessages() {
    if (!networkInitialized) return;
    
    NetworkMessage msg;
    while (networkManager->receiveMessage(msg)) {
        onNetworkMessage(msg);
    }
}

void MultiPlayerGame::onNetworkMessage(const NetworkMessage& msg) {
    if (!networkInitialized) return;
    
    switch (msg.type) {
        case MessageType::PLAYER_INPUT: {
            // 解析玩家输入
            PlayerInputMessage inputMsg = NetworkSerializer::deserializePlayerInput(msg.data);
            handlePlayerInputMessage(inputMsg);
            break;
        }
        case MessageType::PLAYER_STATE: {
            // 解析玩家状态
            PlayerStateMessage stateMsg = NetworkSerializer::deserializePlayerState(msg.data);
            handlePlayerStateMessage(stateMsg);
            break;
        }
        // 处理其他类型的消息...
        default:
            break;
    }
}

void MultiPlayerGame::handlePlayerInputMessage(const PlayerInputMessage& inputMsg) {
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

void MultiPlayerGame::sendPlayerState() {
    if (!networkInitialized || !localPlayer) return;
    
    // 获取本地玩家的状态
    PlayerStateMessage stateMsg = NetworkSerializer::getPlayerStateFromCell(*localPlayer);
    
    // 序列化状态消息
    std::vector<uint8_t> stateData = NetworkSerializer::serializePlayerState(stateMsg);
    
    // 发送状态消息
    NetworkMessage netMsg(MessageType::PLAYER_STATE, stateData);
    networkManager->sendMessage(netMsg);
}

void MultiPlayerGame::handlePlayerStateMessage(const PlayerStateMessage& stateMsg) {
    if (!remotePlayer) return;
    
    // 客户端收到服务器的状态更新，或服务器收到客户端的状态更新
    NetworkSerializer::applyPlayerStateToCell(*remotePlayer, stateMsg);
}
