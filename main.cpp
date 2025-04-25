#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <map>

using namespace cv;
using namespace std;

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
void drawCell(Mat& canvas, const Point& cellPosition, const map<string, float>& config, bool faceRight) {
    // Get cell dimensions
    float cellWidth = config.at("cell_width");
    float cellHeight = config.at("cell_height");
    Vec3b cellColor = Vec3b(200, 230, 255); // Default color (BGR)

    // Scale factor for left/right flipped features
    float directionFactor = faceRight ? 1.0f : -1.0f;
    
    // 1) Cell Body (Ellipse)
    ellipse(canvas, cellPosition, Size(cellWidth, cellHeight), 0, 0, 360, 
            Scalar(cellColor[0], cellColor[1], cellColor[2]), FILLED, LINE_AA);
    
    // 2) Eye
    float eyeSize = config.at("eye_size");
    float eyeEcc = config.at("eye_ecc");
    float eyeAngle = config.at("eye_angle") * directionFactor; // Flip angle if needed
    float eyeYOffset = config.at("eye_y_off");
    float eyeXOffset = config.at("eye_x_off") * directionFactor; // Flip X offset
    
    float ea = eyeSize / 2;
    float eb = ea * (1 - eyeEcc);
    Point eyeCenter(
        cellPosition.x + eyeXOffset * cellWidth,
        cellPosition.y + (eyeYOffset - 0.5) * cellHeight
    );
    ellipse(canvas, eyeCenter, Size(ea, eb), eyeAngle, 0, 360, Scalar(0, 0, 0), FILLED, LINE_AA);
    
    // 3) Mouth (Bezier Curve)
    vector<Point2f> mouthControlPoints = {
        Point2f(cellPosition.x + config.at("mouth_x0") * cellWidth * directionFactor, 
                cellPosition.y + config.at("mouth_y0") * cellHeight),
        Point2f(cellPosition.x + config.at("mouth_x1") * cellWidth * directionFactor, 
                cellPosition.y + config.at("mouth_y1") * cellHeight),
        Point2f(cellPosition.x + config.at("mouth_x2") * cellWidth * directionFactor, 
                cellPosition.y + config.at("mouth_y2") * cellHeight)
    };
    vector<Point2f> mouthCurve;
    for (float t = 0; t <= 1; t += 0.04f) {
        mouthCurve.push_back(bezierPoint(mouthControlPoints, t));
    }
    for (size_t i = 1; i < mouthCurve.size(); ++i) {
        line(canvas, mouthCurve[i - 1], mouthCurve[i], Scalar(0, 0, 0), config.at("mouth_width"), LINE_AA);
    }
    
    // 4) Tail
    vector<Point2f> tailControlPoints = {
        Point2f(cellPosition.x + config.at("tail_x0") * cellWidth * directionFactor, 
                cellPosition.y + config.at("tail_y0") * cellHeight),
        Point2f(cellPosition.x + config.at("tail_x1") * cellWidth * directionFactor, 
                cellPosition.y + config.at("tail_y1") * cellHeight),
        Point2f(cellPosition.x + config.at("tail_x2") * cellWidth * directionFactor, 
                cellPosition.y + config.at("tail_y2") * cellHeight),
        Point2f(cellPosition.x + config.at("tail_x3") * cellWidth * directionFactor, 
                cellPosition.y + config.at("tail_y3") * cellHeight)
    };
    vector<Point2f> tailCurve;
    for (float t = 0; t <= 1; t += 0.02f) {
        tailCurve.push_back(bezierPoint(tailControlPoints, t));
    }
    for (size_t i = 1; i < tailCurve.size(); ++i) {
        line(canvas, tailCurve[i - 1], tailCurve[i], Scalar(0, 0, 0), config.at("tail_width"), LINE_AA);
    }
}

// === Main Program ===
int main() {
    map<string, float> config = {
        {"cell_width", 100.f}, {"cell_height", 60.f}, // Absolute size for the cell
        {"eye_size", 20.f}, {"eye_ecc", 0.1f}, {"eye_angle", 15.f},
        {"eye_y_off", 0.2f}, {"eye_x_off", 0.5f},
        {"mouth_x0", 0.6f}, {"mouth_y0", 0.55f},
        {"mouth_x1", 0.7f}, {"mouth_y1", 0.65f},
        {"mouth_x2", 0.85f}, {"mouth_y2", 0.55f},
        {"mouth_width", 3.f},
        {"tail_x0", -1.0f}, {"tail_y0", 0.0f},
        {"tail_x1", -1.5f}, {"tail_y1", 0.5f},
        {"tail_x2", -2.0f}, {"tail_y2", 0.0f},
        {"tail_x3", -2.5f}, {"tail_y3", 0.5f},
        {"tail_width", 3.f}
    };

    Size canvasSize(800, 600);
    Point offset(0, 0);

    // Physics variables
    Point2f velocity(0, 0);
    Point2f acceleration(0, 0);
    const float maxSpeed = 10.0f;
    const float accelerationStep = 1.0f;
    const float drag = 0.98f; // Water resistance

    // Direction state
    bool faceRight = true;

    while (true) {
        // Update physics
        velocity += acceleration;
        velocity.x = std::clamp(velocity.x, -maxSpeed, maxSpeed);
        velocity.y = std::clamp(velocity.y, -maxSpeed, maxSpeed);
        offset.x += static_cast<int>(velocity.x);
        offset.y += static_cast<int>(velocity.y);
        acceleration = Point2f(0, 0); // Reset acceleration
        velocity *= drag; // Apply drag

        // Create canvas with white background
        Mat canvas = Mat(canvasSize, CV_8UC3, Scalar(255, 255, 255));
        
        // Calculate cell position
        Point cellPosition(canvasSize.width / 2 + offset.x, canvasSize.height / 2 + offset.y);
        
        // Draw cell directly on the canvas
        drawCell(canvas, cellPosition, config, faceRight);

        imshow("Cell Viewer", canvas);

        char key = (char)waitKey(30); // Wait for 30ms
        if (key == 27) { // ESC key
            break;
        } else if (key == 'w') {
            acceleration.y -= accelerationStep; // Accelerate up
        } else if (key == 's') {
            acceleration.y += accelerationStep; // Accelerate down
        } else if (key == 'a') {
            acceleration.x -= accelerationStep; // Accelerate left
            faceRight = false; // Face left
        } else if (key == 'd') {
            acceleration.x += accelerationStep; // Accelerate right
            faceRight = true; // Face right
        }
    }

    destroyAllWindows();
    return 0;
}