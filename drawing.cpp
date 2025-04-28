#include "drawing.h"
#include <cmath>

using namespace cv;
using namespace std;

// Static shield image
static Mat shieldImage;

// Load shield image
void loadShieldImage() {
    char image_dir[] = "../assets/shield.png";
    shieldImage = imread(image_dir, IMREAD_UNCHANGED);
    if(shieldImage.empty())
        throw runtime_error("\n Could not load shield image: " + string(image_dir));
    // if (shieldImage.empty()) {
    //     cerr << "Warning: Could not load shield image (assets/shield.png)" << endl;
    //     // Create a default shield if image can't be loaded
    //     shieldImage = Mat(100, 100, CV_8UC4, Scalar(0, 0, 128, 200));
    //     ellipse(shieldImage, Point(50, 50), Size(40, 40), 0, 0, 360, Scalar(0, 0, 255, 255), 3);
    // }
}

// Get shield image reference
Mat& getShieldImage() {
    return shieldImage;
}

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
         max(1, static_cast<int>(spearThickness)),
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

// Function to draw shield
void drawShield(cv::Mat& canvas, const Point2f& cellPosition, bool faceRight, float scale,
               float cellWidth, float cellHeight, bool isShielding, float shieldTime, bool hasParried) {
    if (!isShielding && !hasParried) return;

    // Scale factor for left/right direction
    float directionFactor = faceRight ? -1.0f : 1.0f; // Shield is on opposite side of facing direction

    // Shield size and position parameters
    float shieldWidth = 100.0f * scale;
    float shieldHeight = 140.0f * scale;

    // Position shield on the opposite side of facing direction
    Point2f shieldPos(
        cellPosition.x - directionFactor * cellWidth * 0.6f,
        cellPosition.y + cellHeight * 0.4f
    );

    // Check if we have the shield image or use a placeholder
    if (getShieldImage().empty()) {
        // Draw a simple elliptical shield if no image
        Scalar shieldColor = hasParried ? Scalar(0, 255, 255, 200) : Scalar(0, 0, 255, 150); // Yellow for parry, blue for normal
        ellipse(canvas,
            Point(static_cast<int>(shieldPos.x), static_cast<int>(shieldPos.y)),
            Size(static_cast<int>(shieldWidth/2), static_cast<int>(shieldHeight/2)),
            0, 0, 360, shieldColor, FILLED, LINE_AA);

        // Draw shield border
        ellipse(canvas,
            Point(static_cast<int>(shieldPos.x), static_cast<int>(shieldPos.y)),
            Size(static_cast<int>(shieldWidth/2), static_cast<int>(shieldHeight/2)),
            0, 0, 360, Scalar(50, 50, 50), 2, LINE_AA);
    } else {
        // Prepare the shield image with appropriate scaling and rotation
        Mat shieldImg = getShieldImage().clone();

        // Resize the shield image to the appropriate size
        Mat resizedShield;
        resize(shieldImg, resizedShield, Size(static_cast<int>(shieldWidth), static_cast<int>(shieldHeight)));

        // Create a region of interest in the canvas
        Rect roi(
            static_cast<int>(shieldPos.x - shieldWidth/2),
            static_cast<int>(shieldPos.y - shieldHeight/2),
            static_cast<int>(shieldWidth),
            static_cast<int>(shieldHeight)
        );

        // Ensure the ROI is within the canvas bounds
        roi = roi & Rect(0, 0, canvas.cols, canvas.rows);

        if (roi.width > 0 && roi.height > 0) {
            // If we have a 4-channel image with alpha, handle transparency
            if (resizedShield.channels() == 4) {
                // Apply alpha blending
                for (int y = 0; y < roi.height; ++y) {
                    for (int x = 0; x < roi.width; ++x) {
                        int srcX = x + roi.x - (static_cast<int>(shieldPos.x - shieldWidth/2));
                        int srcY = y + roi.y - (static_cast<int>(shieldPos.y - shieldHeight/2));

                        if (srcX >= 0 && srcX < resizedShield.cols && srcY >= 0 && srcY < resizedShield.rows) {
                            Vec4b& shieldPixel = resizedShield.at<Vec4b>(srcY, srcX);
                            Vec3b& canvasPixel = canvas.at<Vec3b>(y + roi.y, x + roi.x);

                            // Apply alpha blending and a parry glow effect if needed
                            float alpha = shieldPixel[3] / 255.0f;
                            if (hasParried) {
                                // Add a yellow glow for parry effect
                                canvasPixel[0] = saturate_cast<uchar>((1 - alpha) * canvasPixel[0] + alpha * min(255.0f, shieldPixel[0] * 1.5f));
                                canvasPixel[1] = saturate_cast<uchar>((1 - alpha) * canvasPixel[1] + alpha * min(255.0f, shieldPixel[1] * 1.5f));
                                canvasPixel[2] = saturate_cast<uchar>((1 - alpha) * canvasPixel[2] + alpha * 255); // Strong red component
                            } else {
                                canvasPixel[0] = saturate_cast<uchar>((1 - alpha) * canvasPixel[0] + alpha * shieldPixel[0]);
                                canvasPixel[1] = saturate_cast<uchar>((1 - alpha) * canvasPixel[1] + alpha * shieldPixel[1]);
                                canvasPixel[2] = saturate_cast<uchar>((1 - alpha) * canvasPixel[2] + alpha * shieldPixel[2]);
                            }
                        }
                    }
                }
            } else {
                // If no alpha channel, just copy the shield image
                Mat shieldRoi = resizedShield(Rect(0, 0, roi.width, roi.height));
                shieldRoi.copyTo(canvas(roi));
            }
        }
    }

    // Add visual feedback for parry timing - flash a ring around the shield
    if (hasParried) {
        int pulseRadius = static_cast<int>(shieldWidth * (1.0f + 0.3f * sin(shieldTime * 10.0f)));
        circle(canvas,
               Point(static_cast<int>(shieldPos.x), static_cast<int>(shieldPos.y)),
               pulseRadius,
               Scalar(0, 255, 255), // Yellow
               3, LINE_AA);
    }
    
    // Add visual feedback for perfect parry window (first 50ms)
    if (isShielding && shieldTime < 0.05f) {
        circle(canvas,
               Point(static_cast<int>(shieldPos.x), static_cast<int>(shieldPos.y)),
               static_cast<int>(shieldWidth * 0.7f),
               Scalar(0, 255, 255), // Yellow for perfect parry window
               2, LINE_AA);
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

    // Draw blood drops for this cell
    drawBloodDrops(canvas, cell.bloodDrops);

    // Draw the shield if the cell is shielding
    if (cell.isShielding || cell.hasParried) {
        drawShield(canvas, cellPos, faceRight, scale, cellWidth, cellHeight,
                 cell.isShielding, cell.parryTime, cell.hasParried);
    }

    // Draw the spear for player-controlled cells - only draw if not shielding
    if (cell.isPlayerControlled && !cell.isShielding) {
        drawSpear(canvas, cellPos, faceRight, scale, cellWidth, cellHeight,
                 cell.isAttacking, cell.attackTime);
    }

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
