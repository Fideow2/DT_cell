#ifndef STRUCTS_H
#define STRUCTS_H

#include <opencv2/opencv.hpp>
#include <vector>

// Blood drop structure for attack effect with fixed rotation
struct BloodDrop {
    cv::Point2f position;
    cv::Point2f velocity;
    float size;
    float lifetime;
    float maxLifetime;
    float rotation;  // Fixed rotation angle

    BloodDrop(cv::Point2f pos, cv::Point2f vel, float sz, float life, float rot) :
        position(pos),
        velocity(vel),
        size(sz),
        lifetime(life),
        maxLifetime(life),
        rotation(rot) {}
};

// Cell structure to store individual cell properties
struct Cell {
    cv::Point2f position;
    cv::Point2f velocity;
    cv::Point2f acceleration;
    bool faceRight;
    bool isPlayerControlled;
    int playerNumber;  // 1 for first player, 2 for second player
    cv::Vec3b color;  // Each cell has its own color
    float tailPhaseOffset; // Random phase offset for tail wave
    float aggressionLevel; // Aggression index (0.0 - 1.0) affects mouth and eyes

    // Properties for health system and attack animation
    float health;        // Current health (0-100)
    float maxHealth;     // Maximum health
    bool isAttacking;    // Whether the cell is currently attacking
    float attackTime;    // Time tracker for attack animation

    // Shield properties
    bool isShielding;    // Whether the cell is currently holding shield
    float shieldTime;    // Time tracker for shield animation and parry window
    bool hasParried;     // Whether a successful parry just occurred
    float parryTime;     // Time tracker for parry effect

    // Blood effect properties
    std::vector<BloodDrop> bloodDrops;

    Cell(cv::Point2f pos, bool isPlayer, int playerNum, const cv::Vec3b& baseColor,
         float phaseOffset, float aggression = 0.0f) :
        position(pos),
        velocity(0, 0),
        acceleration(0, 0),
        faceRight(true),
        isPlayerControlled(isPlayer),
        playerNumber(playerNum),
        color(baseColor),
        tailPhaseOffset(phaseOffset),
        aggressionLevel(aggression),
        health(100.0f),
        maxHealth(100.0f),
        isAttacking(false),
        attackTime(0.0f),
        isShielding(false),
        shieldTime(0.0f),
        hasParried(false),
        parryTime(0.0f) {}
};

#endif // STRUCTS_H
