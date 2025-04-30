#ifndef DRAWING_H
#define DRAWING_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include "structs.h"

// 前向声明
class BaseCell;

// Function to compute Bezier curve points
cv::Point2f bezierPoint(const std::vector<cv::Point2f>& controlPoints, float t);

// Function to draw a teardrop shape for blood
void drawTeardropShape(cv::Mat& canvas, const cv::Point2f& position,
                      float size, float rotation, const cv::Scalar& color);

// Function to draw blood drops
void drawBloodDrops(cv::Mat& canvas, const std::vector<BloodDrop>& bloodDrops);

// Function to draw a spear with attack animation
void drawSpear(cv::Mat& canvas, const cv::Point2f& cellPosition, bool faceRight, float scale,
               float cellWidth, float cellHeight, bool isAttacking, float attackTime);

// Function to draw shield
void drawShield(cv::Mat& canvas, const cv::Point2f& cellPosition, bool faceRight, float scale,
               float cellWidth, float cellHeight, bool isShielding, float shieldTime, bool hasParried);

// Function to draw health bar
void drawHealthBar(cv::Mat& canvas, const cv::Point2f& position,
                  float cellWidth, float health, float maxHealth);

// Function to draw the cell directly on the canvas
void drawCell(cv::Mat& canvas, const BaseCell& cell, const std::map<std::string, float>& config,
             float scale, float time);

// Load shield image
void loadShieldImage();

// Get shield image reference
cv::Mat& getShieldImage();

#endif // DRAWING_H
