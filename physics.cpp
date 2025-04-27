#include "physics.h"
#include <random>
#include <algorithm> // for std::clamp

using namespace cv;
using namespace std;

// Function to update blood drops physics
void updateBloodDrops(vector<BloodDrop>& bloodDrops, float deltaTime) {
    // Gravity constant
    const float gravity = 9.8f;

    for (auto it = bloodDrops.begin(); it != bloodDrops.end();) {
        // Update lifetime
        it->lifetime -= deltaTime;

        if (it->lifetime <= 0) {
            // Remove dead blood drops
            it = bloodDrops.erase(it);
        } else {
            // Update position based on velocity
            it->position += it->velocity * deltaTime;

            // Apply gravity
            it->velocity.y += gravity * deltaTime;

            // Note: We keep rotation fixed as assigned at creation

            ++it;
        }
    }
}

// Function to create blood splash effect at a specific position
void createBloodEffect(Cell& cell, const Point2f& hitPosition, bool faceRight, const Point2f& spearTipPosition) {
    // Direction factor based on facing direction
    float directionFactor = faceRight ? 1.0f : -1.0f;

    // Random number generators
    random_device rd;
    mt19937 gen(rd());

    // Distributions for a splatter effect
    uniform_real_distribution<float> velXDist(-2.0f, 2.0f);
    uniform_real_distribution<float> velYDist(-3.0f, 0.5f); // Upward bias
    uniform_real_distribution<float> offsetXDist(-20.0f, 20.0f); // Position X offset
    uniform_real_distribution<float> offsetYDist(-20.0f, 20.0f); // Position Y offset
    uniform_real_distribution<float> forwardOffsetDist(15.0f, 25.0f); // Forward offset (in front of spear)
    uniform_real_distribution<float> sizeDist(4.0f, 7.0f); // Even larger blood drop since we only have one
    uniform_real_distribution<float> lifeDist(0.6f, 1.3f); // Longer lifetime

    // Just one blood drop now
    // Add forward offset in the facing direction plus some randomness
    float forwardOffset = forwardOffsetDist(gen); // 15-25 pixels forward
    float offsetX = offsetXDist(gen);
    float offsetY = offsetYDist(gen);

    // Calculate the base position with forward offset in the direction the spear is facing
    Point2f basePosition = hitPosition + Point2f(directionFactor * forwardOffset, 0);

    // Then add the random offset
    Point2f dropPosition(basePosition.x + offsetX, basePosition.y + offsetY);

    // Random velocity with directional bias
    float vx = velXDist(gen) + directionFactor * 1.5f;
    float vy = velYDist(gen); // Mostly upward for splash effect

    // Random size and lifetime
    float size = sizeDist(gen);
    float lifetime = lifeDist(gen);

    // Calculate the direction from the spear tip to the blood drop position
    Point2f directionVector = dropPosition - spearTipPosition;

    // Calculate the rotation angle so that the pointed end of the blood drop points AWAY from the spear
    float rotation = atan2(directionVector.y, directionVector.x) - CV_PI/2;

    cell.bloodDrops.emplace_back(dropPosition, Point2f(vx, vy), size, lifetime, rotation);
}

// Function to check if a spear attack hits another cell
bool checkSpearCollision(const Cell& attacker, const Cell& target, float scale, float cellWidth,
                         Point2f& hitPosition, Point2f& spearTipPosition) {
    if (!attacker.isAttacking || attacker.attackTime >= 0.5f) {
        return false; // Only check during forward thrust
    }

    // Direction facing
    float directionFactor = attacker.faceRight ? 1.0f : -1.0f;

    // Calculate spear tip position with attack extension
    float attackExtension = attacker.attackTime * 2.0f * 50.0f * scale;
    float spearLength = 100.0f * scale;
    float spearTipX = attacker.position.x + directionFactor * (spearLength * 1.5f + attackExtension);
    float spearTipY = attacker.position.y + 60.0f * scale - spearLength * 0.3f;
    Point2f spearTip(spearTipX, spearTipY);

    // Calculate distance from spear tip to target center
    float distance = norm(spearTip - target.position);

    // Store the hit position (the spear tip)
    hitPosition = spearTip;
    spearTipPosition = spearTip;

    // Hit if distance is less than target cell width
    return distance < cellWidth * scale * 0.8f;
}

// Function to update cell physics
void updateCellPhysics(Cell& cell, const Size& canvasSize, float maxSpeed, float drag, float deltaTime) {
    // Update velocity based on acceleration
    cell.velocity += cell.acceleration;

    // Clamp velocity to maximum speed
    cell.velocity.x = std::clamp(cell.velocity.x, -maxSpeed, maxSpeed);
    cell.velocity.y = std::clamp(cell.velocity.y, -maxSpeed, maxSpeed);

    // Update position based on velocity
    cell.position += cell.velocity;

    // Apply drag
    cell.velocity *= drag;

    // Reset acceleration
    cell.acceleration = Point2f(0, 0);

    // Boundary check to keep cells inside the canvas
    if (cell.position.x < 0) {
        cell.position.x = 0;
        cell.velocity.x *= -0.5f; // Bounce off the wall
    }
    if (cell.position.x > canvasSize.width) {
        cell.position.x = canvasSize.width;
        cell.velocity.x *= -0.5f; // Bounce off the wall
    }
    if (cell.position.y < 0) {
        cell.position.y = 0;
        cell.velocity.y *= -0.5f; // Bounce off the wall
    }
    if (cell.position.y > canvasSize.height) {
        cell.position.y = canvasSize.height;
        cell.velocity.y *= -0.5f; // Bounce off the wall
    }

    // Update direction based on velocity
    if (cell.velocity.x > 0.5f) {
        cell.faceRight = true;
    } else if (cell.velocity.x < -0.5f) {
        cell.faceRight = false;
    }

    // Update blood drops physics
    updateBloodDrops(cell.bloodDrops, deltaTime);
}
