#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <chrono>

#include "structs.h"
#include "drawing.h"
#include "physics.h"

using namespace cv;
using namespace std;

// === Main Program ===
int main() {
    // Constants - Optimized values for better movement
    const float scale = 0.4f;  // Scale factor to make cells smaller
    const int numCells = 20;   // Total number of cells (including 2 player-controlled cells)
    const float maxSpeed = 6.0f;  // Increased max speed for more responsive movement
    const float accelerationStep = 0.7f;  // Increased acceleration for more responsive controls
    const float drag = 0.94f; // Less drag for smoother movement (was 0.98)
    const float randomMoveProbability = 0.05f; // Probability of random movement change
    const float randomMoveStrength = 0.3f; // Strength of random movements
    const Vec3b baseColor(200, 230, 255); // Base cell color (BGR)
    const Vec3b player2Color(200, 255, 220); // Different color for player 2 (more greenish)

    // Constants for aggression behavior
    const float aggressionChangeProbability = 0.01f; // Probability of changing aggression
    const float aggressionChangeAmount = 0.1f; // How much aggression can change at once
    const float maxAggression = 1.0f; // Maximum aggression level
    const float minAggression = 0.0f; // Minimum aggression level

    // Constants for attack behavior - faster attacks for more responsive gameplay
    const float attackDuration = 0.5f; // Reduced attack animation time (was 1.0f)
    const float attackDamage = 8.0f;  // Base damage for attacks

    // New constants for shield behavior
    const float parryWindowDuration = 0.05f; // Time window for perfect parry (in seconds) - 50ms
    const float shieldCooldown = 0.3f; // Cooldown time between shield uses
    const float shieldDuration = 2.0f; // Time shield stays up before auto-lowering
    const float damageReduction = 0.5f; // Damage reduction while shielding (50%)

    map<string, float> config = {
        {"cell_width", 100.f}, {"cell_height", 60.f}, // Absolute size for the cell
        {"eye_size", 20.f}, {"eye_ecc", 0.1f}, {"eye_angle", 15.f},
        {"eye_y_off", 0.2f}, {"eye_x_off", 0.5f},
        {"mouth_x0", 0.6f}, {"mouth_y0", 0.55f},
        {"mouth_x1", 0.7f}, {"mouth_y1", 0.65f},
        {"mouth_x2", 0.85f}, {"mouth_y2", 0.55f},
        {"mouth_width", 3.f},
        {"tail_width", 3.f}
    };

    Size canvasSize(800, 600);

    // Load the shield image
    loadShieldImage();

    // Create random number generator for cell positions, movements and colors
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> xDist(0, canvasSize.width);
    uniform_real_distribution<float> yDist(0, canvasSize.height);
    uniform_real_distribution<float> moveDist(-randomMoveStrength, randomMoveStrength);
    uniform_real_distribution<float> probDist(0, 1);
    uniform_real_distribution<float> aggressionDist(0, maxAggression); // For initial aggression level
    uniform_int_distribution<int> colorDist(-30, 30); // For color variation
    uniform_real_distribution<float> phaseDist(0, 2 * CV_PI); // Random phase for tail wave

    // Create cells - first two are player-controlled
    vector<Cell> cells;

    // Create player 1 cell (WASD controls, F attack, G shield)
    cells.emplace_back(Point2f(canvasSize.width/3, canvasSize.height/2), true, 1, baseColor, 0.0f, 0.0f);
    cells[0].shieldDuration = shieldDuration;
    cells[0].damageReduction = damageReduction;

    // Create player 2 cell (Arrow key controls, / attack, . shield)
    cells.emplace_back(Point2f(canvasSize.width*2/3, canvasSize.height/2), true, 2, player2Color, phaseDist(gen), 0.0f);
    cells[1].shieldDuration = shieldDuration;
    cells[1].damageReduction = damageReduction;

    // Create AI-controlled cells with random positions, colors and aggression levels
    for (int i = 2; i < numCells; ++i) {
        // Create a slight color variation from the base color
        Vec3b cellColor = baseColor;
        cellColor[0] = saturate_cast<uchar>(baseColor[0] + colorDist(gen)); // Blue
        cellColor[1] = saturate_cast<uchar>(baseColor[1] + colorDist(gen)); // Green
        cellColor[2] = saturate_cast<uchar>(baseColor[2] + colorDist(gen)); // Red

        // Each cell has a random phase offset for its tail wave
        float phaseOffset = phaseDist(gen);

        // Each cell has a random initial aggression level
        float aggression = aggressionDist(gen);

        cells.emplace_back(Point2f(xDist(gen), yDist(gen)), false, 0, cellColor, phaseOffset, aggression);
    }

    // For animation timing
    auto startTime = chrono::high_resolution_clock::now();
    auto lastUpdateTime = startTime;
    float frameTime = 1.0f / 60.0f; // Target 60 FPS (was 30)

    // Keyboard code storage
    int key;

    // Shield cooldown timers for players
    float player1ShieldCooldown = 0.0f;
    float player2ShieldCooldown = 0.0f;

    while (true) {
        try {
            // Calculate elapsed time for animation
            auto currentTime = chrono::high_resolution_clock::now();
            float time = chrono::duration<float>(currentTime - startTime).count();
            float deltaTime = chrono::duration<float>(currentTime - lastUpdateTime).count();
            lastUpdateTime = currentTime;

            // Update shield cooldowns
            player1ShieldCooldown = max(0.0f, player1ShieldCooldown - deltaTime);
            player2ShieldCooldown = max(0.0f, player2ShieldCooldown - deltaTime);

            // Create canvas with white background
            Mat canvas = Mat(canvasSize, CV_8UC3, Scalar(255, 255, 255));

            // Update and draw each cell
            for (auto& cell : cells) {
                // Update attack animation if cell is attacking
                if (cell.isAttacking) {
                    // Progress the attack animation
                    cell.attackTime += deltaTime / attackDuration;

                    // Check if attack animation is complete
                    if (cell.attackTime >= 1.0f) {
                        cell.isAttacking = false;
                        cell.attackTime = 0.0f;
                    }
                }

                if (cell.isPlayerControlled) {
                    // Player cell updates are controlled by keyboard

                    // Check for player attacks hitting other cells
                    if (cell.isAttacking && cell.attackTime < 0.5f) { // Only during forward thrust
                        for (auto& target : cells) {
                            // Don't hit self and only hit living cells
                            if (&target != &cell && target.health > 0) {
                                Point2f hitPosition;
                                Point2f spearTipPosition;
                                bool hit = checkSpearCollision(cell, target, scale, config.at("cell_width"),
                                                    hitPosition, spearTipPosition);

                                // Check if the target is shielding and can block
                                bool perfectParry = false;
                                if (hit && checkShieldBlock(target, cell, scale, config.at("cell_width"), perfectParry)) {
                                    // Attack blocked by shield
                                    if (perfectParry) {
                                        // Perfect parry - create parry effect and don't apply damage
                                        createParryEffect(cell, target);
                                    } else {
                                        // Regular shield block - just cancel the attack and apply reduced damage
                                        cell.isAttacking = false;
                                        cell.attackTime = 0.0f;

                                        // Apply reduced damage when blocking with shield
                                        float damage = attackDamage * (1.0f + cell.aggressionLevel * 0.5f);
                                        damage *= (1.0f - target.damageReduction); // Reduce damage by shield
                                        target.health -= damage;

                                        // Ensure health doesn't go below 0
                                        if (target.health < 0) target.health = 0;

                                        // Create smaller blood effect for reduced damage
                                        Point2f reducedHitPos = target.position;
                                        createBloodEffect(target, reducedHitPos, cell.faceRight, spearTipPosition);
                                    }
                                } else if (hit) {
                                    // Calculate damage based on player's aggression
                                    float damage = attackDamage * (1.0f + cell.aggressionLevel * 0.5f);
                                    target.health -= damage;

                                    // Create blood effect at hit position, passing spear tip position
                                    createBloodEffect(target, hitPosition, cell.faceRight, spearTipPosition);

                                    // Ensure health doesn't go below 0
                                    if (target.health < 0) target.health = 0;

                                    // Add knockback effect
                                    float knockbackStrength = 5.0f;
                                    target.velocity += knockbackStrength *
                                        Point2f(cell.faceRight ? 1.0f : -1.0f, -0.5f);
                                }
                            }
                        }
                    }
                } else {
                    // Random movement for AI cells
                    if (probDist(gen) < randomMoveProbability) {
                        cell.acceleration.x += moveDist(gen);
                        cell.acceleration.y += moveDist(gen);
                    }

                    // Random aggression changes for AI cells
                    if (probDist(gen) < aggressionChangeProbability) {
                        // Decide whether to increase or decrease aggression
                        float change = (probDist(gen) < 0.5f) ? -aggressionChangeAmount : aggressionChangeAmount;
                        cell.aggressionLevel = std::clamp(cell.aggressionLevel + change, minAggression, maxAggression);
                    }
                }

                // Only update physics for cells with health > 0
                if (cell.health > 0 || !cell.bloodDrops.empty()) {
                    // Update physics for each cell
                    updateCellPhysics(cell, canvasSize, maxSpeed, drag, deltaTime);

                    // Draw the cell with its unique color, animated tail, and aggression level
                    drawCell(canvas, cell, config, scale, time);
                }
            }

            // Display controls
            putText(canvas, "Player 1: WASD to move, F to attack, G to shield, Q/E to change aggression",
                    Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1, LINE_AA);
            putText(canvas, "Player 2: Arrow keys to move, / to attack, . to shield",
                    Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1, LINE_AA);

            imshow("Multi-Cell Simulation", canvas);

            // Handle keyboard input for both players
            key = waitKey(16); // Wait for 16ms (targeting ~60fps)
            if (key == 27) { // ESC key
                break;
            }

            // Player 1 controls (WASD, Q/E for aggression, F for attack, G for shield)
            if (key == 'w' || key == 'W') {
                cells[0].acceleration.y -= accelerationStep;
            }
            if (key == 's' || key == 'S') {
                cells[0].acceleration.y += accelerationStep;
            }
            if (key == 'a' || key == 'A') {
                cells[0].acceleration.x -= accelerationStep;
                cells[0].faceRight = false;
            }
            if (key == 'd' || key == 'D') {
                cells[0].acceleration.x += accelerationStep;
                cells[0].faceRight = true;
            }
            if (key == 'q' || key == 'Q') {
                cells[0].aggressionLevel = std::max(0.0f, cells[0].aggressionLevel - 0.1f);
            }
            if (key == 'e' || key == 'E') {
                cells[0].aggressionLevel = std::min(1.0f, cells[0].aggressionLevel + 0.1f);
            }
            if ((key == 'f' || key == 'F') && !cells[0].isShielding) {
                if (!cells[0].isAttacking) {
                    cells[0].isAttacking = true;
                    cells[0].attackTime = 0.0f;
                }
            }
            if ((key == 'g' || key == 'G') && player1ShieldCooldown <= 0) {
                // Toggle shield
                cells[0].isShielding = !cells[0].isShielding;

                if (cells[0].isShielding) {
                    cells[0].shieldTime = 0.0f; // Reset shield time for parry window
                } else {
                    // Apply cooldown when shield is put down
                    player1ShieldCooldown = shieldCooldown;

                    // Cancel any attack if putting up shield
                    if (cells[0].isAttacking) {
                        cells[0].isAttacking = false;
                        cells[0].attackTime = 0.0f;
                    }
                }
            }

            // Player 2 controls using proper OpenCV key codes
            // Using more universal key codes for arrow keys
            if (key == 82 || key == 0x260000) { // UP arrow
                cells[1].acceleration.y -= accelerationStep;
            }
            if (key == 84 || key == 0x280000) { // DOWN arrow
                cells[1].acceleration.y += accelerationStep;
            }
            if (key == 81 || key == 0x250000) { // LEFT arrow
                cells[1].acceleration.x -= accelerationStep;
                cells[1].faceRight = false;
            }
            if (key == 83 || key == 0x270000) { // RIGHT arrow
                cells[1].acceleration.x += accelerationStep;
                cells[1].faceRight = true;
            }
            if ((key == '/' || key == 47) && !cells[1].isShielding) {
                if (!cells[1].isAttacking) {
                    cells[1].isAttacking = true;
                    cells[1].attackTime = 0.0f;
                }
            }
            // Use period (.) key for player 2 shield
            if ((key == '.' || key == 46) && player2ShieldCooldown <= 0) {
                // Toggle shield
                cells[1].isShielding = !cells[1].isShielding;

                if (cells[1].isShielding) {
                    cells[1].shieldTime = 0.0f; // Reset shield time for parry window
                } else {
                    // Apply cooldown when shield is put down
                    player2ShieldCooldown = shieldCooldown;

                    // Cancel any attack if putting up shield
                    if (cells[1].isAttacking) {
                        cells[1].isAttacking = false;
                        cells[1].attackTime = 0.0f;
                    }
                }
            }

            // Display shield cooldown and status 
            for (int i = 0; i < 2; i++) {
                string shieldStatus;
                if (cells[i].isShielding) {
                    float remainingTime = cells[i].shieldDuration - cells[i].shieldTime;
                    shieldStatus = "Shield: " + to_string(int(remainingTime * 10) / 10.0) + "s";
                    if (cells[i].shieldTime < parryWindowDuration) {
                        shieldStatus += " (PERFECT PARRY)";
                    }
                } else if (cells[i].shieldCooldownTime > 0) {
                    shieldStatus = "Shield CD: " + to_string(int(cells[i].shieldCooldownTime * 10) / 10.0) + "s";
                } else {
                    shieldStatus = "Shield Ready";
                }

                putText(canvas, shieldStatus, 
                        Point(10, 70 + i*20), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 0), 1, LINE_AA);
            }
        }
        catch (const cv::Exception& e) {
            cerr << "OpenCV error: " << e.what() << endl;
            continue;  // Try to continue with the next frame
        }
        catch (const std::exception& e) {
            cerr << "Error: " << e.what() << endl;
            continue;
        }
    }

    destroyAllWindows();
    return 0;
}
