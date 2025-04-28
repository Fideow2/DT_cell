#ifndef PHYSICS_H
#define PHYSICS_H

#include <opencv2/opencv.hpp>
#include "structs.h"

// Function to update blood drops physics
void updateBloodDrops(std::vector<BloodDrop>& bloodDrops, float deltaTime);

// Function to create blood splash effect at a specific position
void createBloodEffect(Cell& cell, const cv::Point2f& hitPosition, bool faceRight, const cv::Point2f& spearTipPosition);

// Function to update cell physics
void updateCellPhysics(Cell& cell, const cv::Size& canvasSize, float maxSpeed, float drag, float deltaTime);

// Function to check if a spear attack hits another cell
bool checkSpearCollision(const Cell& attacker, const Cell& target, float scale, float cellWidth,
                         cv::Point2f& hitPosition, cv::Point2f& spearTipPosition);

// Function to check if a shield blocks an attack
bool checkShieldBlock(const Cell& defender, const Cell& attacker, float scale, float cellWidth,
                      bool& perfectParry);

// Function to create parry effect
void createParryEffect(Cell& attacker, Cell& defender);

#endif // PHYSICS_H
