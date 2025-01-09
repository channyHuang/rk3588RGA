#include "libExportStream.h"

#include <thread>
#include <chrono>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

void test_rtsp() {
    std::string sUrl = "rtsp://admin:@192.168.1.155:554";
    void *pHandle = HG_GetRtspClient(sUrl.c_str());
    
    int nFrame = 0;
    while (true) {
        Frame* pFrame = HG_ReadFrame(pHandle);
        if (pFrame == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        cv::Mat img = cv::Mat::zeros(pFrame->row, pFrame->col, CV_8UC3);
        memcpy(img.data, pFrame->pChar, pFrame->size);
        cv::imshow("origin", img);
        cv::waitKey(1);
        nFrame++;
        if (nFrame >= 1000) {
            printf("===========\n");
            break;
        }
    }
    HG_CloseClient(pHandle);
}

void test_server() {
    std::string sPlayId = "/live/0";
    
    HG_SetFrameInfo(sPlayId.c_str(), 640, 480);
    HG_StartServer();
    int frame = 0;
    char name[256] = {0};
    while (frame < 1000) {
        memset(name, 0, 256);
        sprintf(name, "./../images/%05d.jpg", frame);
        cv::Mat img = cv::imread(name);
        cv::resize(img, img, cv::Size(640, 480));
        HG_PutFrame(sPlayId.c_str(), img.data, img.cols * img.rows * 3);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        frame += 1;
    }
    HG_StopSever();
}

int main(int argc, char**argv) {
    if (argc < 2) {
        test_rtsp();  
        return 0;
    }
    if (strcmp(argv[1], "server") == 0) {
        test_server();  
        return 0;
    }

    return 0;
}