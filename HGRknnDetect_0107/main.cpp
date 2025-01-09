#include "libExport.h"
#include <sys/time.h>

#include "image_rga.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

int main() {
    bool useOld = false;
    if (useOld) {
        init("yolov8.rknn");
    } else {
        init_crop("yolov8.rknn");
    }
    setClassNum(8);
    setThreshold(0.1);
    float list[13] = {0.5, 0.5, 0.55, 0.5, 0.15, 0.5, 0.2, 0.15};
    setThresholdList(list, 8);

    cv::Mat img = cv::imread("./input.jpg");
    cv::VideoCapture capture;
    capture.open("rgb_test_video.mp4");
    printf("==================== start read frame\n");

    struct timeval time;
    gettimeofday(&time, nullptr);
    auto startTime = time.tv_sec * 1000 + time.tv_usec / 1000;
    auto beforeTime = startTime;
    int frames = 1;
    cv::Mat final_img = cv::Mat::zeros(1080, 1920, CV_8UC3);
    stDetectResult *stResult;
    while (capture.isOpened())
    {
        if (capture.read(img) == false)
            break;
        if (img.cols != 1920 || img.rows != 1080) continue;
        
        if (useOld)
            stResult = detect((char*)img.data, img.cols, img.rows);
        else 
            stResult = detect_crop((char*)img.data, img.cols, img.rows);

        memcpy(final_img.data, stResult->pFrame, img.cols * img.rows * 3);
        cv::imshow("res", final_img);
        cv::waitKey(1);

        if (frames % 30 == 0)
        {
            gettimeofday(&time, nullptr);
            auto currentTime = time.tv_sec * 1000 + time.tv_usec / 1000;
            printf("30 帧内平均帧率:\t %f fps/s\n", 30.0 / float(currentTime - beforeTime) * 1000.0);
            beforeTime = currentTime;
        }
        if (frames > 3000000) {
            frames = 1;
        }
        frames++;
    }
    capture.release();
    if (useOld) deinit();
    else deinit_crop();
    return 0;
}