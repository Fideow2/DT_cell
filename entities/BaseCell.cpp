#include "BaseCell.h"
#include "../physics.h"
#include "../drawing.h"
#include <algorithm>
#include <cmath>
#include <functional>

// 添加AICell的头文件引用
#include "AICell.h"

// 添加随机数生成引擎
std::mt19937& BaseCell::getRandomEngine() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

// 生成杂乱的随机基因
std::string BaseCell::generateRandomGene(int length) {
    static const char charset[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789!@#$%^&*()-_=+";
    
    const size_t max_index = sizeof(charset) - 1;
    std::string gene;
    gene.reserve(length);
    
    auto& gen = getRandomEngine();
    std::uniform_int_distribution<size_t> dist(0, max_index);
    
    for (int i = 0; i < length; i++) {
        gene += charset[dist(gen)];
    }
    
    return gene;
}

BaseCell::BaseCell(const cv::Point2f& pos, int playerNum, const cv::Vec3b& baseColor,
                float phaseOffset, float aggression, const std::string& cellGene)
    : position(pos),
      velocity(0, 0),
      acceleration(0, 0),
      faceRight(true),
      playerNumber(playerNum),
      color(baseColor),
      tailPhaseOffset(phaseOffset),
      aggressionLevel(aggression),
      health(100.0f),
      maxHealth(100.0f),
      isAttackingFlag(false),
      attackTimeValue(0.0f),
      isShieldingFlag(false),
      shieldTimeValue(0.0f),
      hasParriedFlag(false),
      parryTimeValue(0.0f),
      shieldDurationValue(2.0f),
      shieldCooldownTimeValue(0.0f),
      damageReductionValue(0.75f),
      gene(cellGene.empty() ? generateRandomGene() : cellGene),
      faction(0),  // 默认阵营
      sizeMultiplier(1.0f),
      sharpnessMultiplier(1.0f),
      speedMultiplier(1.0f),
      attackMultiplier(1.0f),
      defenseMultiplier(1.0f) {
    
    // 解析基因并设置细胞属性
    parseGene();
}

// 从杂乱的基因中提取特定属性值
float BaseCell::extractGeneAttribute(const std::string& geneStr, int startIndex, int length) const {
    if (geneStr.empty()) return 1.0f;
    
    // 使用一个简单的哈希函数从基因子串中提取属性值
    float value = 0.0f;
    int actualLength = std::min((int)geneStr.length() - startIndex, length);
    
    if (actualLength <= 0) return 1.0f;
    
    // 计算这段基因的特征值
    for (int i = 0; i < actualLength; i++) {
        char c = geneStr[(startIndex + i) % geneStr.length()];
        value += (float)(c) / 255.0f;
    }
    
    // 将值归一化到0.4-1.6范围 - 更宽的范围使差异更明显
    value = 0.4f + (value / actualLength) * 1.2f;
    return value;
}

void BaseCell::parseGene() {
    // 从杂乱的基因中提取各种属性
    // 将基因分成几个部分，每部分对应一个属性
    
    if (gene.empty()) return;
    
    // 使用基因不同部分计算不同属性 - 缩小范围使差异更细微
    sizeMultiplier = 0.9f + extractGeneAttribute(gene, 0, 3) * 0.2f; // 前3个字符影响大小 (0.9-1.1)
    sharpnessMultiplier = 0.9f + extractGeneAttribute(gene, 3, 3) * 0.2f; // 接下来3个字符影响尖锐度 (0.9-1.1)
    attackMultiplier = 0.8f + extractGeneAttribute(gene, 6, 3) * 0.4f; // 接下来3个字符影响攻击力 (0.8-1.2)
    defenseMultiplier = 0.8f + extractGeneAttribute(gene, 9, 3) * 0.4f; // 接下来3个字符影响防御力 (0.8-1.2)
    speedMultiplier = 0.8f + extractGeneAttribute(gene, 12, 3) * 0.4f; // 最后3个字符影响速度 (0.8-1.2)
    
    // 计算基因的哈希值来确定阵营（使基因相似的细胞更可能是同一阵营）
    size_t hash = 0;
    for (char c : gene) {
        hash = hash * 31 + c;
    }
    
    // 弱化属性之间的关系
    
    // 大小会影响血量和防御，但降低速度 - 效果更温和
    maxHealth = 100.0f * (1.0f + (sizeMultiplier - 1.0f) * 0.5f); // 线性增长，更温和
    health = maxHealth;
    damageReductionValue = std::min(0.9f, 0.75f * defenseMultiplier);
    
    // 尖锐度会提高攻击力和速度但降低防御 - 效果更温和
    attackMultiplier *= (1.0f + (sharpnessMultiplier - 1.0f) * 0.2f);
    defenseMultiplier *= (1.0f - (sharpnessMultiplier - 1.0f) * 0.1f);
    speedMultiplier *= (1.0f + (sharpnessMultiplier - 1.0f) * 0.1f);
    
    // 大小对速度的影响 - 线性且更温和
    speedMultiplier *= (1.0f - (sizeMultiplier - 1.0f) * 0.1f);
    
    // 结果的合理范围限制 - 缩小范围使差异更细微
    speedMultiplier = std::max(0.8f, std::min(1.2f, speedMultiplier));
    attackMultiplier = std::max(0.8f, std::min(1.3f, attackMultiplier));
    defenseMultiplier = std::max(0.8f, std::min(1.2f, defenseMultiplier));
    
    // 根据玩家编号设置阵营 (1为0阵营，2为1阵营)
    if (playerNumber > 0) {
        faction = (playerNumber == 2) ? 1 : 0;
    } else {
        // 对于非玩家细胞，根据基因哈希值决定阵营
        faction = hash % 2;
    }

    // 根据阵营和基因设置基础颜色
    updateColorByFaction();
}

void BaseCell::update(float deltaTime, const GameConfig& config, const cv::Size& canvasSize) {
    // 基于调整后的速度更新物理状态
    float adjustedMaxSpeed = config.maxSpeed * speedMultiplier;
    updatePhysics(deltaTime, adjustedMaxSpeed, config.drag, canvasSize);
    
    // 更新攻击动画
    if (isAttackingFlag) {
        attackTimeValue += deltaTime / config.attackDuration;
        
        // 检查攻击动画是否完成
        if (attackTimeValue >= 1.0f) {
            isAttackingFlag = false;
            attackTimeValue = 0.0f;
        }
    }
    
    // 更新盾牌状态
    updateShieldState(deltaTime);
    
    // 更新血液粒子效果
    updateBloodDrops(bloodDrops, deltaTime);
}

void BaseCell::render(cv::Mat& canvas, const std::map<std::string, float>& config, float scale, float time) {
    // 使用现有的绘图函数
    drawCell(canvas, *this, config, scale, time);
}

bool BaseCell::isAlive() const {
    return health > 0;
}

cv::Point2f BaseCell::getPosition() const {
    return position;
}

void BaseCell::applyAcceleration(const cv::Point2f& acc) {
    acceleration += acc;
}

void BaseCell::applyKnockback(const cv::Point2f& force) {
    velocity += force;
}

bool BaseCell::isAttacking() const {
    return isAttackingFlag;
}

void BaseCell::setAttacking(bool attacking) {
    isAttackingFlag = attacking;
}

float BaseCell::getAttackTime() const {
    return attackTimeValue;
}

void BaseCell::setAttackTime(float time) {
    attackTimeValue = time;
}

void BaseCell::takeDamage(float damage) {
    health -= damage;
    if (health < 0) {
        health = 0;
    }
}

bool BaseCell::isShielding() const {
    return isShieldingFlag;
}

void BaseCell::setShielding(bool shielding) {
    isShieldingFlag = shielding;
}

float BaseCell::getShieldTime() const {
    return shieldTimeValue;
}

void BaseCell::setShieldTime(float time) {
    shieldTimeValue = time;
}

bool BaseCell::hasParried() const {
    return hasParriedFlag;
}

void BaseCell::setParried(bool parried) {
    hasParriedFlag = parried;
}

float BaseCell::getParryTime() const {
    return parryTimeValue;
}

void BaseCell::setParryTime(float time) {
    parryTimeValue = time;
}

float BaseCell::getShieldDuration() const {
    return shieldDurationValue;
}

void BaseCell::setShieldDuration(float duration) {
    shieldDurationValue = duration;
}

float BaseCell::getShieldCooldownTime() const {
    return shieldCooldownTimeValue;
}

void BaseCell::setShieldCooldownTime(float time) {
    shieldCooldownTimeValue = time;
}

float BaseCell::getDamageReduction() const {
    return damageReductionValue;
}

void BaseCell::setDamageReduction(float reduction) {
    damageReductionValue = reduction;
}

void BaseCell::updateShieldCooldown(float deltaTime) {
    if (shieldCooldownTimeValue > 0) {
        shieldCooldownTimeValue = std::max(0.0f, shieldCooldownTimeValue - deltaTime);
    }
}

bool BaseCell::canToggleShield() const {
    return shieldCooldownTimeValue <= 0;
}

bool BaseCell::isFacingRight() const {
    return faceRight;
}

void BaseCell::setFacingRight(bool facing) {
    faceRight = facing;
}

float BaseCell::getAggressionLevel() const {
    return aggressionLevel;
}

void BaseCell::setAggressionLevel(float level) {
    aggressionLevel = level;
}

const std::vector<BloodDrop>& BaseCell::getBloodDrops() const {
    return bloodDrops;
}

// 新添加的方法实现
void BaseCell::addBloodDrop(const cv::Point2f& position, const cv::Point2f& velocity, float size, float lifetime, float rotation) {
    bloodDrops.emplace_back(position, velocity, size, lifetime, rotation);
}

void BaseCell::updatePhysics(float deltaTime, float maxSpeed, float drag, const cv::Size& canvasSize) {
    // 基于加速度更新速度
    velocity += acceleration;
    
    // 限制速度不超过最大速度
    velocity.x = std::clamp(velocity.x, -maxSpeed, maxSpeed);
    velocity.y = std::clamp(velocity.y, -maxSpeed, maxSpeed);
    
    // 基于速度更新位置
    position += velocity;
    
    // 应用阻力
    velocity *= drag;
    
    // 重置加速度
    acceleration = cv::Point2f(0, 0);
    
    // 边界检查，保持细胞在画布内
    if (position.x < 0) {
        position.x = 0;
        velocity.x *= -0.5f; // 反弹
    }
    if (position.x > canvasSize.width) {
        position.x = canvasSize.width;
        velocity.x *= -0.5f; // 反弹
    }
    if (position.y < 0) {
        position.y = 0;
        velocity.y *= -0.5f; // 反弹
    }
    if (position.y > canvasSize.height) {
        position.y = canvasSize.height;
        velocity.y *= -0.5f; // 反弹
    }
    
    // 根据速度更新方向，添加更大的阈值和缓冲以避免频繁切换
    // 只有本地玩家或者速度超过较大阈值时才更新朝向
    if (velocity.x > 1.0f) {
        faceRight = true;
    } else if (velocity.x < -1.0f) {
        faceRight = false;
    }
    // 速度很小的情况下保持当前朝向
}

void BaseCell::updateShieldState(float deltaTime) {
    // 更新盾牌时间
    if (isShieldingFlag) {
        shieldTimeValue += deltaTime;
        
        // 盾牌持续时间超过限制后自动降下
        if (shieldTimeValue >= shieldDurationValue) {
            isShieldingFlag = false;
            shieldTimeValue = 0.0f;
            shieldCooldownTimeValue = 0.3f; // 盾牌自动降下后应用冷却时间
        }
    } else {
        shieldTimeValue = 0.0f;
    }
    
    // 更新格挡效果时间
    if (hasParriedFlag) {
        parryTimeValue += deltaTime;
        // 格挡效果持续0.5秒
        if (parryTimeValue >= 0.5f) {
            hasParriedFlag = false;
            parryTimeValue = 0.0f;
        }
    }
}

// 获取基因
const std::string& BaseCell::getGene() const {
    return gene;
}

// 获取阵营
int BaseCell::getFaction() const {
    return faction;
}

// 设置阵营
void BaseCell::setFaction(int newFaction) {
    faction = newFaction;
    // 更新颜色以反映新阵营
    updateColorByFaction();
}

// 获取细胞尺寸倍数
float BaseCell::getSizeMultiplier() const {
    return sizeMultiplier;
}

// 计算两个细胞的基因相似度 (0-1)
float BaseCell::getGeneticSimilarity(const BaseCell& other) const {
    if (gene.empty() || other.getGene().empty()) {
        return 0.0f;
    }
    
    // 使用Levenshtein距离计算两个基因的相似度
    const std::string& s1 = gene;
    const std::string& s2 = other.getGene();
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();
    
    // 计算字符级别的匹配度
    int matches = 0;
    size_t minLength = std::min(len1, len2);
    for (size_t i = 0; i < minLength; i++) {
        if (s1[i] == s2[i]) {
            matches++;
        }
    }
    
    return static_cast<float>(matches) / minLength;
}

// 计算基因特异性伤害系数
float BaseCell::getGeneticDamageMultiplier(const BaseCell& target) const {
    // 基因相似度越低，伤害越高 (模拟免疫系统)
    float similarity = getGeneticSimilarity(target);
    return 1.0f + (1.0f - similarity) * 0.5f;
}

// 计算基因特异性防御系数
float BaseCell::getGeneticDefenseMultiplier(const BaseCell& attacker) const {
    // 基因相似度越高，防御越高
    float similarity = getGeneticSimilarity(attacker);
    return 1.0f + similarity * 0.3f;
}

// 修改为createOffspring以解决编译错误
BaseCell* BaseCell::createOffspring(const BaseCell& parent1, 
                                  const BaseCell& parent2, 
                                  const cv::Size& canvasSize) {
    // 混合父母基因生成新基因
    std::string newGene = mutateGene(parent1.getGene(), parent2.getGene());
    
    // 混合父母的颜色
    cv::Vec3b newColor = (parent1.getColor() + parent2.getColor()) / 2;
    
    // 随机变异颜色
    std::uniform_int_distribution<int> colorDist(-20, 20);
    auto& gen = getRandomEngine();
    newColor[0] = cv::saturate_cast<uchar>(newColor[0] + colorDist(gen));
    newColor[1] = cv::saturate_cast<uchar>(newColor[1] + colorDist(gen));
    newColor[2] = cv::saturate_cast<uchar>(newColor[2] + colorDist(gen));
    
    // 确定位置 (两个父母之间的位置加随机偏移)
    cv::Point2f parentPos1 = parent1.getPosition();
    cv::Point2f parentPos2 = parent2.getPosition();
    cv::Point2f midPoint = (parentPos1 + parentPos2) * 0.5f;
    
    // 添加随机偏移
    std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
    midPoint.x += posDist(gen);
    midPoint.y += posDist(gen);
    
    // 限制在画布范围内
    midPoint.x = std::clamp(midPoint.x, 0.0f, static_cast<float>(canvasSize.width));
    midPoint.y = std::clamp(midPoint.y, 0.0f, static_cast<float>(canvasSize.height));
    
    // 随机相位偏移
    std::uniform_real_distribution<float> phaseDist(0, 2 * CV_PI);
    float phaseOffset = phaseDist(gen);
    
    // 创建新细胞 - 使用AICell而不是BaseCell，因为我们需要一个具体类
    AICell* offspring = new AICell(
        midPoint, 0, newColor, phaseOffset, 0.5f, newGene
    );
    
    // 随机确定阵营继承 (80%概率继承父母的阵营)
    std::uniform_real_distribution<float> factionDist(0.0f, 1.0f);
    if (parent1.getFaction() == parent2.getFaction() || factionDist(gen) < 0.8f) {
        offspring->setFaction(factionDist(gen) < 0.5f ? parent1.getFaction() : parent2.getFaction());
    } else {
        // 小概率产生阵营变异
        offspring->setFaction(factionDist(gen) < 0.5f ? 0 : 1);
    }
    
    return offspring;
}

// 基因突变 - 生成更杂乱的基因
std::string BaseCell::mutateGene(const std::string& parentGene1, const std::string& parentGene2) {
    std::string newGene;
    auto& gen = getRandomEngine();
    
    // 确保生成的基因长度一致
    size_t targetLength = std::max(parentGene1.length(), parentGene2.length());
    if (targetLength < 16) targetLength = 16; // 确保至少16个字符
    
    newGene.reserve(targetLength);
    
    // 均匀分布随机选择父母基因
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (size_t i = 0; i < targetLength; i++) {
        char geneChar;
        
        // 60%概率从父母继承，40%概率变异
        if (dist(gen) < 0.6f) {
            // 从两个父母中随机选择一个基因位点
            if (i < parentGene1.length() && i < parentGene2.length()) {
                geneChar = (dist(gen) < 0.5f) ? parentGene1[i] : parentGene2[i];
            } else if (i < parentGene1.length()) {
                geneChar = parentGene1[i];
            } else if (i < parentGene2.length()) {
                geneChar = parentGene2[i];
            } else {
                // 如果超出两个父母的长度，生成随机字符
                geneChar = generateRandomGene(1)[0];
            }
        } else {
            // 生成随机变异字符
            geneChar = generateRandomGene(1)[0];
        }
        
        newGene += geneChar;
    }
    
    return newGene;
}

// 单个基因位点突变 - 现在使用更广泛的字符集
char BaseCell::mutateGeneChar(char c) {
    static const char charset[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789!@#$%^&*()-_=+";
    
    const size_t max_index = sizeof(charset) - 1;
    auto& gen = getRandomEngine();
    std::uniform_int_distribution<size_t> charDist(0, max_index);
    
    return charset[charDist(gen)];
}

// 设置细胞颜色
void BaseCell::setColor(const cv::Vec3b& newColor) {
    color = newColor;
}

// 根据阵营更新颜色
void BaseCell::updateColorByFaction() {
    // 生成一个基于基因的随机颜色变化
    auto& gen = getRandomEngine();
    std::uniform_int_distribution<int> colorVariation(-15, 15);
    
    // 计算一个特定于这个基因的颜色种子
    unsigned int colorSeed = 0;
    for (size_t i = 0; i < gene.length(); i++) {
        colorSeed = colorSeed * 31 + gene[i];
    }
    
    // 使用这个种子为这个特定的基因设置一个固定的颜色变化
    std::mt19937 colorGen(colorSeed);
    
    // 保存原色调
    cv::Vec3b originalColor = color;
    float colorSum = (originalColor[0] + originalColor[1] + originalColor[2]) / 3.0f;
    
    // 根据阵营调整颜色，但添加更多基于基因的变化
    if (faction == 0) { // 蓝色阵营
        color[0] = cv::saturate_cast<uchar>(colorSum * (0.5f + sharpnessMultiplier * 0.1f) + colorVariation(colorGen));
        color[1] = cv::saturate_cast<uchar>(colorSum * (0.6f + attackMultiplier * 0.1f) + colorVariation(colorGen));
        color[2] = cv::saturate_cast<uchar>(colorSum * (1.4f + speedMultiplier * 0.1f) + colorVariation(colorGen));
    } else { // 绿色阵营
        color[0] = cv::saturate_cast<uchar>(colorSum * (0.5f + sharpnessMultiplier * 0.1f) + colorVariation(colorGen));
        color[1] = cv::saturate_cast<uchar>(colorSum * (1.4f + attackMultiplier * 0.1f) + colorVariation(colorGen));
        color[2] = cv::saturate_cast<uchar>(colorSum * (0.6f + speedMultiplier * 0.1f) + colorVariation(colorGen));
    }
    
    // 根据尺寸调整饱和度 - 更大的细胞颜色更饱和
    float saturationFactor = 0.8f + sizeMultiplier * 0.2f;
    for (int i = 0; i < 3; i++) {
        float avgColor = (color[0] + color[1] + color[2]) / 3.0f;
        color[i] = cv::saturate_cast<uchar>((color[i] - avgColor) * saturationFactor + avgColor);
    }
}