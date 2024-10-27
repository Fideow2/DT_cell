#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;
using namespace cv;

// bool isrect(vector <int> lunkuo){
//     vector <int> lunkuo2;
    
// }

int main(){
    Mat img0 = imread("../img/board4.jpg");
    if(img0.empty()){
        cout << "Could not open or find the image" << endl;
        return -1;
    }
    Mat img;
    GaussianBlur(img0, img, Size(5, 5), 0); // 高斯模糊
    img.convertTo(img, -1, 2, 0); // 对比度增强
    cvtColor(img, img, COLOR_BGR2GRAY);
    Mat erzhi;
    threshold(img, erzhi, 250, 255, THRESH_BINARY);
    int white = countNonZero(erzhi);
    Mat element = getStructuringElement(cv::MORPH_RECT,Size(10, 10),cv::Point(-1, -1));
    erode(erzhi, erzhi, element);
    imshow("erzhi", erzhi);
    vector <vector<Point>> contours;
    vector <Vec4i> hierarchy;
    findContours(erzhi, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    Mat bg = Mat::zeros(img.size(), CV_8UC3);
    int maxarea = 0;
    RotatedRect maxrect;
    for(int i = 0; i < contours.size(); i++){
        int A = contourArea(contours[i]);
        int L = arcLength(contours[i], true);
        if(A < 100) continue;
        approxPolyDP(contours[i], contours[i], L * 0.05, 1);
        RotatedRect rect = minAreaRect(contours[i]);
        //比较 rect 的最大面积
        int area = rect.size.width * rect.size.height;
        if(area > maxarea){
            maxarea = area;
            maxrect = rect;
        }
    }
    if(maxarea){
        Point2f vertices[4];
        maxrect.points(vertices);
        for(int i = 0; i < 4; i++){
            line(img0, vertices[i], vertices[(i + 1) % 4], Scalar(0, 255, 0), 10);
        }
        putText(img0, "Area: " + to_string(maxarea), Point(50, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
    }
    imshow("qwq", img0);
    waitKey(0);
    return 0;
}
