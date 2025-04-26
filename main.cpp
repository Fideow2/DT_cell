#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <chrono>

using namespace cv;
using namespace std;

// Cell structure to store individual cell properties
struct Cell {
    Point2f position;
    Point2f velocity;
    Point2f acceleration;
    bool faceRight;
    bool isPlayerControlled;
    Vec3b color;  // Each cell has its own color
    float tailPhaseOffset; // Random phase offset for tail wave
    float aggressionLevel; // Aggression index (0.0 - 1.0) affects mouth and eyes
    
    Cell(Point2f pos, bool isPlayer, const Vec3b& baseColor, float phaseOffset, float aggression = 0.0f) : 
        position(pos), 
        velocity(0, 0), 
        acceleration(0, 0), 
        faceRight(true), 
        isPlayerControlled(isPlayer),
        color(baseColor),
        tailPhaseOffset(phaseOffset),
        aggressionLevel(aggression) {}
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

// Function to draw the cell directly on the canvas
void drawCell(Mat& canvas, const Point2f& cellPosition, const map<string, float>& config, bool faceRight, 
              float scale, const Vec3b& cellColor, float time, float tailPhaseOffset, float aggressionLevel) {
    // Get cell dimensions
    float cellWidth = config.at("cell_width") * scale;
    float cellHeight = config.at("cell_height") * scale;
    
    Point2f cellPos(cellPosition.x, cellPosition.y);

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
        // wavePhase = fmod(wavePhase, 2 * CV_PI); // Restrict wavePhase to (0, PI)
        // wavePhase = 0.3f / max(wavePhase, 0.01f); // Take the reciprocal

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
        int tailThickness = 1;
                
        line(canvas, 
            Point(static_cast<int>(tailCurve[i-1].x), static_cast<int>(tailCurve[i-1].y)), 
            Point(static_cast<int>(tailCurve[i].x), static_cast<int>(tailCurve[i].y)), 
            Scalar(0, 0, 0), tailThickness, LINE_AA);
    }
}

// Function to update cell physics
void updateCellPhysics(Cell& cell, const Size& canvasSize, float maxSpeed, float drag) {
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
}

// === Main Program ===
int main() {
    // Constants
    const float scale = 0.4f;  // Scale factor to make cells smaller
    const int numCells = 20;   // Total number of cells
    const float maxSpeed = 5.0f;
    const float accelerationStep = 0.5f;
    const float drag = 0.98f; // Water resistance
    const float randomMoveProbability = 0.05f; // Probability of random movement change
    const float randomMoveStrength = 0.3f; // Strength of random movements
    const Vec3b baseColor(200, 230, 255); // Base cell color (BGR)
    
    // New constants for aggression behavior
    const float aggressionChangeProbability = 0.01f; // Probability of changing aggression
    const float aggressionChangeAmount = 0.1f; // How much aggression can change at once
    const float maxAggression = 1.0f; // Maximum aggression level
    const float minAggression = 0.0f; // Minimum aggression level

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

    // Create cells - first one is player-controlled
    vector<Cell> cells;
    
    // Create player cell with base color and initial aggression level of 0
    cells.emplace_back(Point2f(canvasSize.width/2, canvasSize.height/2), true, baseColor, 0.0f, 0.0f);
    
    // Create AI-controlled cells with random positions, colors and aggression levels
    for (int i = 1; i < numCells; ++i) {
        // Create a slight color variation from the base color
        Vec3b cellColor = baseColor; // Direct copy instead of clone()
        cellColor[0] = saturate_cast<uchar>(baseColor[0] + colorDist(gen)); // Blue
        cellColor[1] = saturate_cast<uchar>(baseColor[1] + colorDist(gen)); // Green
        cellColor[2] = saturate_cast<uchar>(baseColor[2] + colorDist(gen)); // Red
        
        // Each cell has a random phase offset for its tail wave
        float phaseOffset = phaseDist(gen);
        
        // Each cell has a random initial aggression level
        float aggression = aggressionDist(gen);
        
        cells.emplace_back(Point2f(xDist(gen), yDist(gen)), false, cellColor, phaseOffset, aggression);
    }

    // For animation timing
    auto startTime = chrono::high_resolution_clock::now();

    while (true) {
        try {
            // Calculate elapsed time for animation
            auto currentTime = chrono::high_resolution_clock::now();
            float time = chrono::duration<float>(currentTime - startTime).count();
    
            // Create canvas with white background
            Mat canvas = Mat(canvasSize, CV_8UC3, Scalar(255, 255, 255));
            
            // Update and draw each cell
            for (auto& cell : cells) {
                if (cell.isPlayerControlled) {
                    // Player cell updates are controlled by keyboard
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
                
                // Update physics for each cell
                updateCellPhysics(cell, canvasSize, maxSpeed, drag);
                
                // Draw the cell with its unique color, animated tail, and aggression level
                drawCell(canvas, cell.position, config, cell.faceRight, scale, 
                         cell.color, time, cell.tailPhaseOffset, cell.aggressionLevel);
                
                // Add "you" text above player's cell
                if (cell.isPlayerControlled) {
                    Point textPos(static_cast<int>(cell.position.x - 10), 
                                  static_cast<int>(cell.position.y - config.at("cell_height") * scale));
                    putText(canvas, "you", textPos, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1, LINE_AA);
                }
            }
    
            imshow("Multi-Cell Simulation", canvas);
    
            char key = (char)waitKey(30); // Wait for 30ms
            if (key == 27) { // ESC key
                break;
            } else if (key == 'w') {
                cells[0].acceleration.y -= accelerationStep; // Accelerate up
            } else if (key == 's') {
                cells[0].acceleration.y += accelerationStep; // Accelerate down
            } else if (key == 'a') {
                cells[0].acceleration.x -= accelerationStep; // Accelerate left
                cells[0].faceRight = false; // Face left
            } else if (key == 'd') {
                cells[0].acceleration.x += accelerationStep; // Accelerate right
                cells[0].faceRight = true; // Face right
            } else if (key == 'q') {
                // Decrease player's aggression level
                cells[0].aggressionLevel = std::max(0.0f, cells[0].aggressionLevel - 0.1f);
            } else if (key == 'e') {
                // Increase player's aggression level
                cells[0].aggressionLevel = std::min(1.0f, cells[0].aggressionLevel + 0.1f);
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