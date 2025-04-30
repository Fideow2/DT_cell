#ifndef PHYSICS_H
#define PHYSICS_H

#include <opencv2/opencv.hpp>
#include "structs.h"
#include "entities/BaseCell.h"

// Function to update blood drops physics
void updateBloodDrops(std::vector<BloodDrop>& bloodDrops, float deltaTime);

// Function to create blood splash effect at a specific position
void createBloodEffect(BaseCell& cell, const cv::Point2f& hitPosition, bool faceRight, const cv::Point2f& spearTipPosition);

// Function to check if a spear attack hits another cell
bool checkSpearCollision(const BaseCell& attacker, const BaseCell& target, float scale, float cellWidth,
                         cv::Point2f& hitPosition, cv::Point2f& spearTipPosition);

// Function to check if a shield blocks an attack
bool checkShieldBlock(const BaseCell& defender, const BaseCell& attacker, float scale, float cellWidth,
                      bool& perfectParry);

// Function to create parry effect
void createParryEffect(BaseCell& attacker, BaseCell& defender);

#endif // PHYSICS_H
