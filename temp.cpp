#include <iostream>
#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <map>

using namespace std;
using namespace cv;

// 初始化HSV阈值的上下限
int lowH = 0, lowS = 0, lowV = 0;
int highH = 180, highS = 255, highV = 255;

#define HEIGHT 480
#define EPSILON_RATIO 0.05 //目前这个参数表现良好
#define MIN_AREA_RATIO 0.1  //目前这个参数表现良好
int ROI [] {150,150,300,180};

void emptytrackbar(){}

std::pair<std::vector<cv::Point>, std::vector<cv::Point>> findBox(cv::Mat& img_bin, cv::Mat& frame) {
    if (img_bin.empty()) {
        std::cout << "img_bin cannot be empty" << std::endl;
        throw std::invalid_argument("img_bin cannot be empty");
    }

    int min_area = EPSILON_RATIO * img_bin.total();

    std::map<int, std::vector<cv::Point>> ret;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(img_bin, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return std::make_pair(std::vector<cv::Point>(), std::vector<cv::Point>());
    }

    for (size_t i = 0; i < contours.size(); i++) {
        if (cv::contourArea(contours[i]) >= min_area) {
            double epsilon = EPSILON_RATIO * cv::arcLength(contours[i], true);
            std::vector<cv::Point> approx;
            cv::approxPolyDP(contours[i], approx, epsilon, true);

            if (approx.size() == 4) {
                ret[i] = approx;
            }
        }
    }

    for (size_t i = 0; i < contours.size(); i++) {
        if (ret.count(i) && ret[i].size() == 4 && hierarchy[i][3] != -1 && ret.count(hierarchy[i][3]) && ret[hierarchy[i][3]].size() == 4 && hierarchy[i][2] == -1) {
            if (!frame.empty()) {
                cv::drawContours(frame, contours, i, cv::Scalar(0, 0, 255), 2);
                cv::drawContours(frame, contours, hierarchy[i][3], cv::Scalar(0, 0, 255), 2);
                cv::polylines(frame, ret[i], true, cv::Scalar(0, 255, 255), 10);
                cv::polylines(frame, ret[hierarchy[i][3]], true, cv::Scalar(0, 255, 255), 10);
            }

            return std::make_pair(ret[hierarchy[i][3]], ret[i]);
        }
    }

    return std::make_pair(std::vector<cv::Point>(), std::vector<cv::Point>());
}

int main(int mainnum, char *mainstr[]){
    if(mainnum != 3){
        puts("请输入 ./image_processor /dev/video0 230");
        // 连接到电脑的第一个摄像头，设置曝光值为230
        return -1;
    }
    // 一个接口类型
    VideoCapture vi(mainstr[1], CAP_V4L2); // 取决于 系统 摄像头类型 特定需求
    if(!vi.isOpened()){
        puts("无法打开摄像头");
        return -1;
    }
    double baoguangdu = stod(mainstr[2]);
    vi.set(CAP_PROP_AUTO_EXPOSURE, 3.0); // 先打开自动曝光
    vi.set(CAP_PROP_AUTO_EXPOSURE, 1.0); // 再关闭自动曝光
    vi.set(CAP_PROP_EXPOSURE, baoguangdu); // 最后设置曝光参数
    if(!vi.set(CAP_PROP_FRAME_WIDTH, 640) || !vi.set(CAP_PROP_FRAME_HEIGHT, 480)){
        puts("无法设置图像大小"); // set 返回值都是 bool
        return -1;
    }
    if(!vi.set(CAP_PROP_FPS, 40)){
        puts("无法设置摄像头帧率");
        return -1;
    }
    namedWindow("qwq", WINDOW_AUTOSIZE);
    createTrackbar("Low H", "qwq", &lowH, 180, emptytrackbar);
    createTrackbar("High H", "qwq", &highH, 180, emptytrackbar);
    createTrackbar("Low S", "qwq", &lowS, 255, emptytrackbar);
    createTrackbar("High S", "qwq", &highS, 255, emptytrackbar);
    createTrackbar("Low V", "qwq", &lowV, 255, emptytrackbar);
    createTrackbar("High V", "qwq", &highV, 255, emptytrackbar);
    int MIN_AREA = int(MIN_AREA_RATIO * (ROI[2] - ROI[0]) * (ROI[3] - ROI[1])); // 0.1 * 150 * 180 降噪？
    while(1){
        Mat zhen0;
        vi.read(zhen0); //和 vi >> zhen0 等价
        if(zhen.empty()){
            puts("无法读取摄像头帧");
            break;
        }
        Rect roi(ROI[0], ROI[1], ROI[2], ROI[3]); // 150 150 300 180 好像真要截了 ohhhhh
        Mat zhen = zhen0(roi);
        GaussianBlur(zhen, zhen, Size(5, 5), 0); // Size(核的宽度，核的高度), 标准差
        Mat hsv;
        cvtColor(zhen, hsv, COLOR_BGR2HSV);
        Mat mask;
        inrange(hsv, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), mask);
        Mat binary;
        bitwise_not(mask, mask);
        bitwise_and(zhen, zhen, binary, mask);
        cvtColor(binary, binary, COLOR_BGR2GRAY);
        imshow("gray", binary);
        threshold(binary, binary, 120, 255, THRESH_BINARY);
        // 灰度图 结果 阈值 最大值 阈值类型（ >= 阈值的点会变成最大值）
        // 这阈值待定
        auto boxex = findBox(binary, zhen);
        
    }
    return 0;
}

        // draw the contours
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(binary_image, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
        cv::Mat drawing = cv::Mat::zeros(binary_image.size(), CV_8UC3);
        for (size_t i = std::max(0, static_cast<int>(contours.size()) - 2); i < contours.size(); i++)
        {
            cv::Scalar color = cv::Scalar(0, 255, 0);
            cv::drawContours(drawing, contours, (int)i, color, 2, cv::LINE_8, hierarchy, 0);
        }

        cv::imshow("Binary Image", binary_image);
        cv::imshow("Original Image", frame);

        // 按下Esc键退出循环
        char key = (char)cv::waitKey(30);
        if (key == 27)
            break;
    }

    // 释放摄像头
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
