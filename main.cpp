#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <chrono>

using namespace cv;
using namespace std;

// Blood drop structure for attack effect with fixed rotation
struct BloodDrop {
    Point2f position;
    Point2f velocity;
    float size;
    float lifetime;
    float maxLifetime;
    float rotation;  // Fixed rotation angle

    BloodDrop(Point2f pos, Point2f vel, float sz, float life, float rot) :
        position(pos),
        velocity(vel),
        size(sz),
        lifetime(life),
        maxLifetime(life),
        rotation(rot) {}
};

// Cell structure to store individual cell properties
struct Cell {
    Point2f position;
    Point2f velocity;
    Point2f acceleration;
    bool faceRight;
    bool isPlayerControlled;
    int playerNumber;  // 1 for first player, 2 for second player
    Vec3b color;  // Each cell has its own color
    float tailPhaseOffset; // Random phase offset for tail wave
    float aggressionLevel; // Aggression index (0.0 - 1.0) affects mouth and eyes

    // New properties for health system and attack animation
    float health;        // Current health (0-100)
    float maxHealth;     // Maximum health
    bool isAttacking;    // Whether the cell is currently attacking
    float attackTime;    // Time tracker for attack animation

    // Blood effect properties
    vector<BloodDrop> bloodDrops;

    Cell(Point2f pos, bool isPlayer, int playerNum, const Vec3b& baseColor, float phaseOffset, float aggression = 0.0f) :
        position(pos),
        velocity(0, 0),
        acceleration(0, 0),
        faceRight(true),
        isPlayerControlled(isPlayer),
        playerNumber(playerNum),
        color(baseColor),
        tailPhaseOffset(phaseOffset),
        aggressionLevel(aggression),
        health(100.0f),      // Initialize with full health
        maxHealth(100.0f),   // Maximum health
        isAttacking(false),  // Not attacking initially
        attackTime(0.0f)     // Reset attack time
    {}
};

// Function to compute Bezier curve points
Point2f bezierPoint(const vector<Point2f>& controlPoints, float t) {
    int n = controlPoints.size() - 1;
    Point2f point(0, 0);
    for (int i = 0; i <= n; ++i) {
        float binomial = tgamma(n + 1) / (tgamma(i + 1) * tgamma(n - i + 1));
        float coeff = binomial * pow(1 - t, n - i) * pow(t, i);
        point += coeff * controlPoints[i];
    }
    return point;
}

// Function to draw a teardrop shape for blood
void drawTeardropShape(Mat& canvas, const Point2f& position, float size, float rotation, const Scalar& color) {
    // Define the teardrop shape as a combination of an ellipse and a triangle

    // Calculate rotation matrix
    float angle = rotation * (180.0f / CV_PI); // Convert to degrees for OpenCV

    // Ellipse part (the round part of the teardrop)
    Size2f ellipseSize(size * 0.8f, size); // Slightly wider than tall
    RotatedRect ellipseRect(position, ellipseSize, angle);
    ellipse(canvas, ellipseRect, color, -1, LINE_AA);

    // Triangle part (the pointed tail of the teardrop)
    // Calculate points for the triangle at the tail end of the teardrop
    float tailLength = size * 1.5f;

    // Calculate the direction vector from the rotation angle
    Point2f dir(cos(rotation - CV_PI/2), sin(rotation - CV_PI/2));

    // Calculate triangle points
    Point2f tailTip = position + tailLength * dir;
    Point2f tailSide1 = position + size * 0.4f * Point2f(-dir.y, dir.x);
    Point2f tailSide2 = position + size * 0.4f * Point2f(dir.y, -dir.x);

    // Draw the triangle
    Point trianglePoints[3] = {
        Point(static_cast<int>(tailTip.x), static_cast<int>(tailTip.y)),
        Point(static_cast<int>(tailSide1.x), static_cast<int>(tailSide1.y)),
        Point(static_cast<int>(tailSide2.x), static_cast<int>(tailSide2.y))
    };

    fillConvexPoly(canvas, trianglePoints, 3, color, LINE_AA);
}

// Function to draw blood drops
void drawBloodDrops(Mat& canvas, const vector<BloodDrop>& bloodDrops) {
    for (const auto& drop : bloodDrops) {
        // Calculate alpha based on remaining lifetime
        float alpha = drop.lifetime / drop.maxLifetime;

        // Blood color - pure red
        Scalar bloodColor(0, 0, 255);

        // Draw a teardrop shape instead of a circle
        drawTeardropShape(canvas, drop.position, drop.size, drop.rotation, bloodColor);
    }
}

// Function to draw a spear with attack animation
void drawSpear(Mat& canvas, const Point2f& cellPosition, bool faceRight, float scale,
               float cellWidth, float cellHeight, bool isAttacking, float attackTime) {
    // Scale factor for left/right direction
    float directionFactor = faceRight ? 1.0f : -1.0f;

    // Spear parameters
    float spearLength = 100.0f * scale;
    float spearThickness = max(1.0f, 2.0f * scale); // Ensure thickness is at least 1
    float spearheadWidth = 10.0f * scale;
    float spearheadHeight = 15.0f * scale;

    // Calculate attack extension
    float attackExtension = 0.0f;
    if (isAttacking) {
        // Extend forward during the first half of the animation, retract during second half
        if (attackTime < 0.5f) {
            attackExtension = attackTime * 2.0f; // 0 to 1
        } else {
            attackExtension = (1.0f - attackTime) * 2.0f; // 1 to 0
        }
        // Scale the extension effect
        attackExtension *= 50.0f * scale; // Adjust this multiplier for attack distance
    }

    // Starting point of the spear (in front and below the cell)
    Point2f spearStart(
        cellPosition.x,
        cellPosition.y + (cellHeight * 1.0f)
    );

    // End point of the spear with attack extension
    Point2f spearEnd(
        spearStart.x + directionFactor * (spearLength * 1.5f + attackExtension),
        spearStart.y - spearLength * 0.3f  // Angled downward
    );

    // Draw the spear shaft
    line(canvas,
         Point(static_cast<int>(spearStart.x), static_cast<int>(spearStart.y)),
         Point(static_cast<int>(spearEnd.x), static_cast<int>(spearEnd.y)),
         Scalar(0, 0, 0), // Black color
         max(1, static_cast<int>(spearThickness)), // Ensure thickness is at least 1
         LINE_AA);

    // Draw the spearhead (triangle)
    Point spearheadPoints[3];
    spearheadPoints[0] = Point(static_cast<int>(spearEnd.x), static_cast<int>(spearEnd.y)); // Tip
    spearheadPoints[1] = Point(static_cast<int>(spearEnd.x - directionFactor * spearheadHeight),
                              static_cast<int>(spearEnd.y - spearheadWidth/2)); // Top corner
    spearheadPoints[2] = Point(static_cast<int>(spearEnd.x - directionFactor * spearheadHeight),
                              static_cast<int>(spearEnd.y + spearheadWidth/2)); // Bottom corner

    // Draw filled triangle
    fillConvexPoly(canvas, spearheadPoints, 3, Scalar(0, 0, 0), LINE_AA);
}

// Function to create blood splash effect at a specific position
// Modified to make blood drops appear more forward from the hit position and point AWAY from the spear
// Now creates only one larger blood drop
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

// Function to update blood drops physics (without rotation updates)
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

// Function to draw health bar
void drawHealthBar(Mat& canvas, const Point2f& position, float cellWidth, float health, float maxHealth) {
    float healthBarWidth = cellWidth * 1.2f;
    float healthBarHeight = 5.0f;
    Point2f healthBarPos(
        position.x - healthBarWidth / 2,
        position.y - 50.0f // Position above the cell
    );

    // Draw health bar background (empty bar - red)
    rectangle(canvas,
        Point(static_cast<int>(healthBarPos.x), static_cast<int>(healthBarPos.y)),
        Point(static_cast<int>(healthBarPos.x + healthBarWidth), static_cast<int>(healthBarPos.y + healthBarHeight)),
        Scalar(0, 0, 150), FILLED, LINE_AA);

    // Draw health bar fill based on current health percentage (green)
    float healthPercentage = health / maxHealth;
    rectangle(canvas,
        Point(static_cast<int>(healthBarPos.x), static_cast<int>(healthBarPos.y)),
        Point(static_cast<int>(healthBarPos.x + healthBarWidth * healthPercentage),
              static_cast<int>(healthBarPos.y + healthBarHeight)),
        Scalar(0, 150, 0), FILLED, LINE_AA);

    // Draw health bar border
    rectangle(canvas,
        Point(static_cast<int>(healthBarPos.x), static_cast<int>(healthBarPos.y)),
        Point(static_cast<int>(healthBarPos.x + healthBarWidth),
              static_cast<int>(healthBarPos.y + healthBarHeight)),
        Scalar(0, 0, 0), 1, LINE_AA);
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

// Function to draw the cell directly on the canvas
void drawCell(Mat& canvas, const Cell& cell, const map<string, float>& config, float scale,
              float time) {
    // Get cell dimensions
    float cellWidth = config.at("cell_width") * scale;
    float cellHeight = config.at("cell_height") * scale;

    Point2f cellPos(cell.position.x, cell.position.y);
    bool faceRight = cell.faceRight;
    Vec3b cellColor = cell.color;
    float tailPhaseOffset = cell.tailPhaseOffset;
    float aggressionLevel = cell.aggressionLevel;

    // Scale factor for left/right flipped features
    float directionFactor = faceRight ? 1.0f : -1.0f;

    // 1) Cell Body (Ellipse)
    ellipse(canvas, Point(static_cast<int>(cellPos.x), static_cast<int>(cellPos.y)),
            Size(static_cast<int>(cellWidth), static_cast<int>(cellHeight)),
            0, 0, 360, Scalar(cellColor[0], cellColor[1], cellColor[2]), FILLED, LINE_AA);

    // 2) Eye - simplified anger expression
    float eyeSize = config.at("eye_size") * scale;
    float eyeEcc = config.at("eye_ecc");
    float eyeAngle = config.at("eye_angle") * directionFactor; // Flip angle if needed
    float eyeYOffset = config.at("eye_y_off");
    float eyeXOffset = config.at("eye_x_off") * directionFactor; // Flip X offset

    float ea = eyeSize / 2;
    float eb = ea * (1 - eyeEcc);
    Point2f eyeCenter(
        cellPos.x + eyeXOffset * cellWidth,
        cellPos.y + (eyeYOffset - 0.5) * cellHeight
    );

    // Draw the full eye
    ellipse(canvas, Point(static_cast<int>(eyeCenter.x), static_cast<int>(eyeCenter.y)),
            Size(static_cast<int>(ea), static_cast<int>(eb)),
            eyeAngle, 0, 360, Scalar(0, 0, 0), FILLED, LINE_AA);

    // Draw a rectangle to cover the top part of the eye based on aggression level
    if (aggressionLevel > 0.0f) {
        // Calculate the height of the cover rectangle based on aggression
        float coverHeight = eb * aggressionLevel * 0.9f; // Up to 90% of eye height

        // Calculate the coordinates for the cover rectangle
        // We position it to cover the top part of the eye
        Point2f rectCenter(eyeCenter.x, eyeCenter.y - eb/2 + coverHeight/2 - 2.5f);

        // Create a rotated rectangle that aligns with the eye angle
        RotatedRect rotatedRect(Point2f(rectCenter), Size2f(ea * 2.2f + 2.0f, coverHeight + 2.0f), eyeAngle);

        // Get the four corners of the rotated rectangle
        Point2f vertices[4];
        rotatedRect.points(vertices);

        // Draw a filled polygon using the vertices
        Point points[4];
        for (int i = 0; i < 4; ++i) {
            points[i] = Point(static_cast<int>(vertices[i].x), static_cast<int>(vertices[i].y));
        }

        fillConvexPoly(canvas, points, 4, Scalar(cellColor[0], cellColor[1], cellColor[2]), LINE_AA);
    }

    // 3) Mouth (Bezier Curve) - modified to show aggression by curving downward
    // The control points adjust based on aggression level
    vector<Point2f> mouthControlPoints = {
        Point2f(cellPos.x + config.at("mouth_x0") * cellWidth * directionFactor,
                cellPos.y + config.at("mouth_y0") * cellHeight),
        Point2f(cellPos.x + config.at("mouth_x1") * cellWidth * directionFactor,
                cellPos.y + (config.at("mouth_y1") - 0.25f * aggressionLevel) * cellHeight), // Middle point goes lower
        Point2f(cellPos.x + config.at("mouth_x2") * cellWidth * directionFactor,
                cellPos.y + config.at("mouth_y2") * cellHeight)
    };

    vector<Point2f> mouthCurve;
    for (float t = 0; t <= 1; t += 0.04f) {
        mouthCurve.push_back(bezierPoint(mouthControlPoints, t));
    }

    // Draw mouth with thickness based on aggression (angrier = thicker mouth line)
    int mouthThickness = max(1, int(1 + aggressionLevel * 2 * scale));
    for (size_t i = 1; i < mouthCurve.size(); ++i) {
        line(canvas,
            Point(static_cast<int>(mouthCurve[i-1].x), static_cast<int>(mouthCurve[i-1].y)),
            Point(static_cast<int>(mouthCurve[i].x), static_cast<int>(mouthCurve[i].y)),
            Scalar(0, 0, 0), mouthThickness, LINE_AA);
    }

    // 4) Animated Tail with wave effect
    // Base tail points (similar to the original)
    float tailLength = 2.5f * cellWidth;
    float tailWaveAmplitude = 0.25f * cellHeight;
    float tailWaveFrequency = 3.0f;  // Number of waves along tail
    float tailWaveSpeed = 5.0f;      // Speed of wave movement

    // Create a more detailed tail curve with wave effect
    vector<Point2f> tailCurve;
    int numTailPoints = 30; // More points for smoother tail

    // Calculate starting point and direction vector for the tail
    Point2f tailStart(cellPos.x - directionFactor * cellWidth * 0.5f, cellPos.y);
    Point2f tailDirection(-directionFactor, 0); // Tail extends left/right depending on direction

    for (int i = 0; i <= numTailPoints; i++) {
        float t = static_cast<float>(i) / numTailPoints;

        // Calculate position along the tail (decreasing exponential to make it taper)
        float x = tailStart.x + tailDirection.x * t * tailLength;
        float y = tailStart.y + tailDirection.y * t * tailLength;

        // Calculate wave effect perpendicular to tail direction
        float wavePhase = time * tailWaveSpeed + tailPhaseOffset + t * tailWaveFrequency * 2 * CV_PI;

        float perpX = -tailDirection.y; // Perpendicular to tail direction
        float perpY = tailDirection.x;

        // Amplitude decreases toward the end of the tail
        float localAmplitude = tailWaveAmplitude * (t);
        float waveOffset = sin(wavePhase) * localAmplitude;

        Point2f pointWithWave(x + perpX * waveOffset, y + perpY * waveOffset);
        tailCurve.push_back(pointWithWave);
    }

    // Draw the tail curve with fixed thickness
    for (size_t i = 6; i < tailCurve.size(); ++i) {
        // Always use a safe fixed thickness for tail segments
        int tailThickness = 1; // Minimum thickness of 1

        line(canvas,
            Point(static_cast<int>(tailCurve[i-1].x), static_cast<int>(tailCurve[i-1].y)),
            Point(static_cast<int>(tailCurve[i].x), static_cast<int>(tailCurve[i].y)),
            Scalar(0, 0, 0), tailThickness, LINE_AA);
    }

    // Draw the spear for player-controlled cells
    if (cell.isPlayerControlled) {
        drawSpear(canvas, cellPos, faceRight, scale, cellWidth, cellHeight,
                 cell.isAttacking, cell.attackTime);
    }

    // Draw blood drops for this cell
    drawBloodDrops(canvas, cell.bloodDrops);

    // Draw health bar
    drawHealthBar(canvas, cellPos, cellWidth, cell.health, cell.maxHealth);

    // Add player identification text
    if (cell.isPlayerControlled) {
        string playerText = "player " + to_string(cell.playerNumber);
        Point textPos(static_cast<int>(cell.position.x - 25),
                      static_cast<int>(cell.position.y - config.at("cell_height") * scale - 30));
        putText(canvas, playerText, textPos, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1, LINE_AA);
    }
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

// === Main Program ===
int main() {
    // Constants
    const float scale = 0.4f;  // Scale factor to make cells smaller
    const int numCells = 20;   // Total number of cells (including 2 player-controlled cells)
    const float maxSpeed = 5.0f;
    const float accelerationStep = 0.5f;
    const float drag = 0.98f; // Water resistance
    const float randomMoveProbability = 0.05f; // Probability of random movement change
    const float randomMoveStrength = 0.3f; // Strength of random movements
    const Vec3b baseColor(200, 230, 255); // Base cell color (BGR)
    const Vec3b player2Color(200, 255, 220); // Different color for player 2 (more greenish)

    // New constants for aggression behavior
    const float aggressionChangeProbability = 0.01f; // Probability of changing aggression
    const float aggressionChangeAmount = 0.1f; // How much aggression can change at once
    const float maxAggression = 1.0f; // Maximum aggression level
    const float minAggression = 0.0f; // Minimum aggression level

    // New constants for attack behavior
    const float attackDuration = 1.0f; // Complete attack animation in 1 second
    const float attackDamage = 8.0f;  // Base damage for attacks

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

    // Create player 1 cell (WASD controls, F attack)
    cells.emplace_back(Point2f(canvasSize.width/3, canvasSize.height/2), true, 1, baseColor, 0.0f, 0.0f);

    // Create player 2 cell (Arrow key controls, / attack)
    cells.emplace_back(Point2f(canvasSize.width*2/3, canvasSize.height/2), true, 2, player2Color, phaseDist(gen), 0.0f);

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
    float frameTime = 1.0f / 30.0f; // Target 30 FPS

    // Keyboard code storage
    int key;

    while (true) {
        try {
            // Calculate elapsed time for animation
            auto currentTime = chrono::high_resolution_clock::now();
            float time = chrono::duration<float>(currentTime - startTime).count();
            float deltaTime = chrono::duration<float>(currentTime - lastUpdateTime).count();
            lastUpdateTime = currentTime;

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
                                if (checkSpearCollision(cell, target, scale, config.at("cell_width"), hitPosition, spearTipPosition)) {
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
            putText(canvas, "Player 1: WASD to move, F to attack, Q/E to change aggression",
                    Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1, LINE_AA);
            putText(canvas, "Player 2: Arrow keys to move, / to attack",
                    Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1, LINE_AA);

            imshow("Multi-Cell Simulation", canvas);

            // Handle keyboard input for both players
            key = waitKey(30); // Wait for 30ms
            if (key == 27) { // ESC key
                break;
            }

            // Player 1 controls (WASD, Q/E for aggression, J for attack)
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
            if (key == 'f' || key == 'F') {
                if (!cells[0].isAttacking) {
                    cells[0].isAttacking = true;
                    cells[0].attackTime = 0.0f;
                }
            }

            // Player 2 controls (Arrow keys for movement, / for attack)
            // Note: Arrow key codes can vary across platforms; if these don't work,
            // you may need to adjust the key codes for your system
            if (key == 82 || key == 2490368) { // UP arrow
                cells[1].acceleration.y -= accelerationStep;
            }
            if (key == 84 || key == 2621440) { // DOWN arrow
                cells[1].acceleration.y += accelerationStep;
            }
            if (key == 81 || key == 2424832) { // LEFT arrow
                cells[1].acceleration.x -= accelerationStep;
                cells[1].faceRight = false;
            }
            if (key == 83 || key == 2555904) { // RIGHT arrow
                cells[1].acceleration.x += accelerationStep;
                cells[1].faceRight = true;
            }
            if (key == '/' || key == 47) {
                if (!cells[1].isAttacking) {
                    cells[1].isAttacking = true;
                    cells[1].attackTime = 0.0f;
                }
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
