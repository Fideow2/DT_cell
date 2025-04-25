#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <iostream>
#include <cmath>

using namespace std;
using namespace cv;

void emptytrackbar(int, void *){}
int lowH = 0, lowS = 0, lowV = 0;
int highH = 180, highS = 255, highV = 255;

double vectorlen(double x, double y){
    return sqrt(x * x + y * y);
}
double vectordianji(double x1, double y1, double x2, double y2){
    return x1 * x2 + y1 * y2;
}
double vectorjiajiao(double x1, double y1, double x2, double y2){
    double ang = acos(vectordianji(x1, y1, x2, y2) / (vectorlen(x1, y1) * vectorlen(x2, y2))) * 180 / CV_PI;
    return min(ang, 180 - ang);
}
double linejiajiao(Vec4i l1, Vec4i l2){
    double x1 = l1[2] - l1[0];
    double y1 = l1[3] - l1[1];
    double x2 = l2[2] - l2[0];
    double y2 = l2[3] - l2[1];
    return vectorjiajiao(x1, y1, x2, y2);
}
double linelen(Vec4i l){
    return vectorlen(l[2] - l[0], l[3] - l[1]);
}

Mat img0;
Vec4i l3;
bool hebing(Vec4i l1, Vec4i l2){
    // 在窗口 hebing 中显示 l1 和 l2
    // Mat hebing = img0.clone();
    // line(hebing, Point(l1[0], l1[1]), Point(l1[2], l1[3]), Scalar(0, 255, 0), 2, LINE_AA);
    // line(hebing, Point(l2[0], l2[1]), Point(l2[2], l2[3]), Scalar(255, 0, 0), 2, LINE_AA);
    // imshow("hebing", hebing);
    // cout << "尝试合并：" << l1[0] << " " << l1[1] << " " << l1[2] << " " << l1[3] << " " << l2[0] << " " << l2[1] << " " << l2[2] << " " << l2[3] << endl;
    // waitKey(0);
    int x1 = l1[0], y1 = l1[1], x2 = l1[2], y2 = l1[3];
    int x3 = l2[0], y3 = l2[1], x4 = l2[2], y4 = l2[3];
    l3 = Vec4i(x1, y1, x3, y3); // 这玩应 debug 了半天
    Vec4i l4 = Vec4i(x1, y1, x4, y4);
    Vec4i l5 = Vec4i(x2, y2, x3, y3);
    Vec4i l6 = Vec4i(x2, y2, x4, y4);
    double angle = 15; // 可以调整
    if(linejiajiao(l1, l2) > angle) return 0;
    // puts("角度合适");
    // 找到 l3,l4,l5,l6 中最长的线
    double len1 = linelen(l1), len2 = linelen(l2), len3 = linelen(l3), len4 = linelen(l4), len5 = linelen(l5), len6 = linelen(l6);
    double maxlen = max(max(len3, len4), max(len5, len6));
    if(maxlen == len3) l3 = Vec4i(x1, y1, x3, y3);
    if(maxlen == len4) l3 = Vec4i(x1, y1, x4, y4);
    if(maxlen == len5) l3 = Vec4i(x2, y2, x3, y3);
    if(maxlen == len6) l3 = Vec4i(x2, y2, x4, y4);
    // cout << "最长线为：" << l3[0] << " " << l3[1] << " " << l3[2] << " " << l3[3] << endl;
    if(linejiajiao(l1, l3) > angle || linejiajiao(l2, l3) > angle) return 0;
    // puts("最长线角度合适");
    if(len1 > maxlen) maxlen = len1, l3 = l1;
    if(len2 > maxlen) maxlen = len2, l3 = l2;
    if(len1 + len2 + 30 < maxlen) return 0;
    // puts("长度合适");
    // 在窗口 hebing 中显示 l3
    // line(hebing, Point(l3[0], l3[1]), Point(l3[2], l3[3]), Scalar(0, 0, 255), 2, LINE_AA);
    // imshow("hebing", hebing);
    // puts("合并成功");
    // cout << "合并后：" << l3[0] << " " << l3[1] << " " << l3[2] << " " << l3[3] << endl;
    // waitKey(0);
    return 1;
}

double getarea(Vec4i ansi, Vec4i ansj, Vec4i ansk, double ansxik, double ansyik, double ansxjk, double ansyjk){
        // Mat outimg = Mat::zeros(Size(600, 600), CV_8UC3);
        if(linelen(ansi) < linelen(ansj)) swap(ansi, ansj), swap(ansxik, ansxjk), swap(ansyik, ansyjk);
        int xx, yy;
        if(abs(ansi[0] - ansxik) + abs(ansi[1] - ansyik) > abs(ansi[2] - ansxik) + abs(ansi[3] - ansyik)) xx = ansi[0], yy = ansi[1];
        else xx = ansi[2], yy = ansi[3];
        int xxx = ansxjk + xx - ansxik, yyy = ansyjk + yy - ansyik;
        // Scalar color = Scalar(0, 255, 0);
        // line(outimg, Point(ansxik, ansyik), Point(xx, yy), color, 2, LINE_AA);
        // line(outimg, Point(xx, yy), Point(xxx, yyy), color, 2, LINE_AA);
        // line(outimg, Point(xxx, yyy), Point(ansxjk, ansyjk), color, 2, LINE_AA);
        // line(outimg, Point(ansxjk, ansyjk), Point(ansxik, ansyik), color, 2, LINE_AA);
        double area = vectorlen(abs(xx - ansxjk), abs(yy - ansyjk)) * vectorlen(abs(xxx - ansxik), abs(yyy - ansyik)) / 2;
        // cout << "Area: " << area << endl;
        // putText(outimg, "Area: " + to_string(area), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2, LINE_AA);
        // imshow("outimg", outimg);
        // waitKey(0);
    return area;
}

Mat getrect(Mat img0){
    //缩小图片高度为 hei
    int hei0 = img0.rows;
    int hei = 600;
    resize(img0, img0, Size(img0.cols * hei / img0.rows, hei));
    Mat img;
    GaussianBlur(img0, img, Size(5, 5), 0); // 高斯模糊
    // 亮度减弱
    // img.convertTo(img, -1, 0, -1);S
    img.convertTo(img, -1, 2.5, -100); // 对比度增强
    // imshow("img", img);
    // return img;
    cvtColor(img, img, COLOR_BGR2GRAY);
    Mat erzhi;
    threshold(img, erzhi, 250, 255, THRESH_BINARY);
    // int white = countNonZero(erzhi);
    // Mat element = getStructuringElement(cv::MORPH_RECT,Size(10, 10),cv::Point(-1, -1));
    // erode(erzhi, erzhi, element);
    // imshow("erzhi", erzhi);
    // return erzhi;
    // 边缘检测
    Canny(erzhi, erzhi, 50, 150, 3);
    // imshow("canny", erzhi);
    // waitKey(0);
    // return erzhi;
    vector <Vec4i> lines;
    HoughLinesP(erzhi, lines, 1, CV_PI/180, 50, 20, 30);
    // 参数分别为：输入图像，输出直线，距离精度，角度精度，阈值，最小线段长度，最大线段间隔
    Mat img2 = img0.clone();
    for(size_t i = 0; i < lines.size(); ++i){
        Vec4i l = lines[i];
        line(img2, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 2, LINE_AA);
    }
    // imshow("img2", img2);
    // return img2;
    //将两条特别近的线合并
    vector <Vec4i> lines2;
    for(size_t i = 0; i < lines.size(); ++i){
        // cout << "滴滴滴，再说一遍 l3：" << l3[0] << ' ' << l3[1] << ' ' << l3[2] << ' ' << l3[3] << endl;
        Vec4i l = lines[i];
        if(linelen(l) < 10) continue; // 可以调整
        // cout << "第 " << i << " 轮\n";
        bool flag = 0;
        for(size_t j = 0; j < lines2.size(); ++j){
            Vec4i l2 = lines2[j];
            if(hebing(l, l2)){
                flag = 1;
                lines2[j] = l3;
                // cout << "lines2[" << j << "] = " << l3[0] << " " << l3[1] << " " << l3[2] << " " << l3[3] << endl;
                // puts("change");
            }
        }
        if(!flag) lines2.push_back(l);
        // if(!flag) puts("add new");
        // Mat img3 = img0.clone();
        // cout << "lines2.size() = " << lines2.size() << endl;
        // for(size_t j = 0; j < lines2.size(); ++j){
        //     Vec4i l = lines2[j];
            // cout << "lines2[" << j << "] = " << l[0] << " " << l[1] << " " << l[2] << " " << l[3] << endl;
        //     line(img3, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 2, LINE_AA);
        // }
        // imshow("img3", img3);
        // puts("已显示所有 lines2");
        // waitKey(0);
    }
    Mat img3 = img0.clone();

    for(size_t i = 0; i < lines2.size(); ++i){
        Vec4i l = lines2[i];
        line(img3, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 2, LINE_AA);
    }
    // imshow("img3", img3);
    return img3;
    // 对 lines2 中的线进行极角排序
    vector <pair <double, Vec4i> > lines3;
    for(size_t i = 0; i < lines2.size(); ++i){
        Vec4i l = lines2[i];
        double angle = atan2(l[3] - l[1], l[2] - l[0]);
        lines3.push_back(make_pair(angle, l));
    }
    sort(lines3.begin(), lines3.end(), [](pair <double, Vec4i> a, pair <double, Vec4i> b){
        return a.first < b.first;
    });
    Vec4i ansi, ansj, ansk;
    double ansxik, ansxjk, ansyik, ansyjk;
    double maxarea = 0;
    for(size_t i = 0; i < lines3.size(); ++i){
        for(size_t j = i + 1; j < lines3.size(); ++j){
            double angle1 = 15, angle2 = 30;
            if(linejiajiao(lines3[i].second, lines3[j].second) > angle1) continue;
            for(size_t k = 0; k < lines3.size(); ++k){
                // 画出 i,j,k
                // Mat img4 = img0.clone();
                // line(img4, Point(lines3[i].second[0], lines3[i].second[1]), Point(lines3[i].second[2], lines3[i].second[3]), Scalar(0, 255, 0), 2, LINE_AA);
                // line(img4, Point(lines3[j].second[0], lines3[j].second[1]), Point(lines3[j].second[2], lines3[j].second[3]), Scalar(0, 255, 0), 2, LINE_AA);
                // line(img4, Point(lines3[k].second[0], lines3[k].second[1]), Point(lines3[k].second[2], lines3[k].second[3]), Scalar(0, 255, 0), 2, LINE_AA);
                // imshow("img4", img4);
                // waitKey(0);

                if(linejiajiao(lines3[i].second, lines3[k].second) < angle2) continue;
                if(linejiajiao(lines3[j].second, lines3[k].second) < angle2) continue;
                // 求出 i和k，j和k 的交点
                int xik, yik, xjk, yjk;
                // 考虑除以 0 的情况
                if((lines3[i].second[0] - lines3[i].second[2]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[i].second[1] - lines3[i].second[3]) * (lines3[k].second[0] - lines3[k].second[2]) == 0){
                    xik = lines3[i].second[0], yik = lines3[k].second[1];
                    xjk = lines3[j].second[0], yjk = lines3[k].second[1];
                }
                if((lines3[j].second[0] - lines3[j].second[2]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[j].second[1] - lines3[j].second[3]) * (lines3[k].second[0] - lines3[k].second[2]) == 0){
                    xik = lines3[i].second[0], yik = lines3[k].second[1];
                    xjk = lines3[j].second[0], yjk = lines3[k].second[1];
                }
                else{
                    xik = (lines3[i].second[0] * lines3[i].second[3] - lines3[i].second[2] * lines3[i].second[1]) * (lines3[k].second[0] - lines3[k].second[2]) - (lines3[i].second[0] - lines3[i].second[2]) * (lines3[k].second[0] * lines3[k].second[3] - lines3[k].second[2] * lines3[k].second[1]);
                    yik = (lines3[i].second[1] * lines3[i].second[2] - lines3[i].second[3] * lines3[i].second[0]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[i].second[1] - lines3[i].second[3]) * (lines3[k].second[1] * lines3[k].second[2] - lines3[k].second[3] * lines3[k].second[0]);
                    xik /= (lines3[i].second[0] - lines3[i].second[2]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[i].second[1] - lines3[i].second[3]) * (lines3[k].second[0] - lines3[k].second[2]);
                    yik /= (lines3[i].second[0] - lines3[i].second[2]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[i].second[1] - lines3[i].second[3]) * (lines3[k].second[0] - lines3[k].second[2]);
                    xjk = (lines3[j].second[0] * lines3[j].second[3] - lines3[j].second[2] * lines3[j].second[1]) * (lines3[k].second[0] - lines3[k].second[2]) - (lines3[j].second[0] - lines3[j].second[2]) * (lines3[k].second[0] * lines3[k].second[3] - lines3[k].second[2] * lines3[k].second[1]);
                    yjk = (lines3[j].second[1] * lines3[j].second[2] - lines3[j].second[3] * lines3[j].second[0]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[j].second[1] - lines3[j].second[3]) * (lines3[k].second[1] * lines3[k].second[2] - lines3[k].second[3] * lines3[k].second[0]);
                    xjk /= (lines3[j].second[0] - lines3[j].second[2]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[j].second[1] - lines3[j].second[3]) * (lines3[k].second[0] - lines3[k].second[2]);
                    yjk /= (lines3[j].second[0] - lines3[j].second[2]) * (lines3[k].second[1] - lines3[k].second[3]) - (lines3[j].second[1] - lines3[j].second[3]) * (lines3[k].second[0] - lines3[k].second[2]);
                }
                xik = abs(xik), yik = abs(yik), xjk = abs(xjk), yjk = abs(yjk);
                // cout << "xik = " << xik << " yik = " << yik << " xjk = " << xjk << " yjk = " << yjk << endl;
                // 在 img4 画出交点
                // circle(img4, Point(xik, yik), 5, Scalar(0, 0, 255), -1);
                // circle(img4, Point(xjk, yjk), 5, Scalar(0, 0, 255), -1);
                // imshow("img4", img4);
                // waitKey(0);

                // 判断交点和线段端点的曼哈顿距离是否小于 dis
                int dis = 30;
                if(abs(xik - lines3[k].second[0]) + abs(yik - lines3[k].second[1]) > dis && abs(xik - lines3[k].second[2]) + abs(yik - lines3[k].second[3]) > dis) continue;
                if(abs(xjk - lines3[k].second[0]) + abs(yjk - lines3[k].second[1]) > dis && abs(xjk - lines3[k].second[2]) + abs(yjk - lines3[k].second[3]) > dis) continue;
                if(abs(xik - lines3[i].second[0]) + abs(yik - lines3[i].second[1]) > dis && abs(xik - lines3[i].second[2]) + abs(yik - lines3[i].second[3]) > dis) continue;
                if(abs(xjk - lines3[j].second[0]) + abs(yjk - lines3[j].second[1]) > dis && abs(xjk - lines3[j].second[2]) + abs(yjk - lines3[j].second[3]) > dis) continue;
                // 计算 i 和 j 围成的四边形的面积
                double area = getarea(lines3[i].second, lines3[j].second, lines3[k].second, xik, yik, xjk, yjk);
                // double area = abs((lines3[i].second[0] - lines3[j].second[0]) * (lines3[i].second[3] - lines3[j].second[3]) - (lines3[i].second[1] - lines3[j].second[1]) * (lines3[i].second[2] - lines3[j].second[2])) / 2;
                // cout << "area = " << area << endl;
                if(area > maxarea){
                    maxarea = area;
                    ansi = lines3[i].second;
                    ansj = lines3[j].second;
                    ansk = lines3[k].second;
                    ansxik = xik, ansyik = yik, ansxjk = xjk, ansyjk = yjk;
                }
            }
        }
    }
    Mat outimg = img0.clone();
    if(maxarea){
        if(linelen(ansi) < linelen(ansj)) swap(ansi, ansj), swap(ansxik, ansxjk), swap(ansyik, ansyjk);
        int xx, yy;
        if(abs(ansi[0] - ansxik) + abs(ansi[1] - ansyik) > abs(ansi[2] - ansxik) + abs(ansi[3] - ansyik)) xx = ansi[0], yy = ansi[1];
        else xx = ansi[2], yy = ansi[3];
        int xxx = ansxjk + xx - ansxik, yyy = ansyjk + yy - ansyik;
        Scalar color = Scalar(0, 255, 0);
        line(outimg, Point(ansxik, ansyik), Point(xx, yy), color, 2, LINE_AA);
        line(outimg, Point(xx, yy), Point(xxx, yyy), color, 2, LINE_AA);
        line(outimg, Point(xxx, yyy), Point(ansxjk, ansyjk), color, 2, LINE_AA);
        line(outimg, Point(ansxjk, ansyjk), Point(ansxik, ansyik), color, 2, LINE_AA);
        double area = vectorlen(abs(xx - ansxjk), abs(yy - ansyjk)) * vectorlen(abs(xxx - ansxik), abs(yyy - ansyik)) / 2;
        putText(outimg, "Area: " + to_string(area), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2, LINE_AA);
    }
    // imshow("outimg", outimg);
    // waitKey(0);
    resize(outimg, outimg, Size(outimg.cols * hei0 / outimg.rows, hei0));
    return outimg;
}

Mat chuli(Mat img0){
    GaussianBlur(img0, img0, Size(5, 5), 0); // 高斯模糊
    //查找橘色的区域
    // return img0;
    Mat hsv;
    cvtColor(img0, hsv, COLOR_BGR2HSV);
    Mat mask1, mask2, mask;
    // inRange(hsv, Scalar(156, 100, 100), Scalar(180, 255, 255), mask1);
    inRange(hsv, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), mask);
    // Mat mask = mask1 | mask2;
    //转化为二值图像
    Mat erzhi;
    threshold(mask, erzhi, 250, 255, THRESH_BINARY);
    return erzhi;
    //识别轮廓
    vector <vector <Point> > contours;
    vector <Vec4i> hierarchy;
    findContours(erzhi, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    //遍历每个轮廓
    for(size_t i = 0; i < contours.size(); ++i){
        double area = contourArea(contours[i]);
        if(area < 100) continue;
        //找到轮廓的最小外接矩形
        RotatedRect rect = minAreaRect(contours[i]);
        rect.angle = 0;
        double bianchang = sqrt(area);
        rect.center.y -= bianchang * 10;
        rect.size.width += bianchang * 30;
        rect.size.height += bianchang * 30;
        Point2f vertices[4];
        rect.points(vertices);
        for(int j = 0; j < 4; ++j){
            line(img0, vertices[j], vertices[(j + 1) % 4], Scalar(0, 255, 0), 2, LINE_AA);
        }
    }
    return img0;
    
}

int main(){
	Mat zhen;
    namedWindow("video", WINDOW_AUTOSIZE);
    cv::createTrackbar("Low H", "video", &lowH, 180, emptytrackbar);
    cv::createTrackbar("High H", "video", &highH, 180, emptytrackbar);
    cv::createTrackbar("Low S", "video", &lowS, 255, emptytrackbar);
    cv::createTrackbar("High S", "video", &highS, 255, emptytrackbar);
    cv::createTrackbar("Low V", "video", &lowV, 255, emptytrackbar);
    cv::createTrackbar("High V", "video", &highV, 255, emptytrackbar);
	// VideoWriter shuchu("../img/newkoulan1.mp4",shuru.get(CAP_PROP_FOURCC),frame_fps,Size(frame_width,frame_height),true);
    //让视频循环播放
    while(1){
    	VideoCapture shuru("../img/koulan3.mp4");
        while (1) {
            shuru.read(zhen);
            if(zhen.empty()) break;
            int hei = 300;
            resize(zhen, zhen, Size(zhen.cols * hei / zhen.rows, hei));
            zhen = chuli(zhen);
            // shuchu.write(zhen);
            imshow("video", zhen);
            int c = waitKey(10);
            if (c == 27) return 0;
        }
    	shuru.release();
    }
	// shuchu.release();
    return 0;
}