# HGStream
推拉流库。使用ffmedia让mpp用起来以提高帧率。支持c++/Python调用。

接口：
```c++
// ======================================
// 拉流初始化，输入uri, nRtpType = 0:udp, 1:tcp
void* HG_GetRtspClient(const char* pUri, const int nRtpType = 0);
// 结束拉流
void HG_CloseClient(void* pHandle);
// 获取拉流帧
Frame* HG_ReadFrame(void* pHandle);
// 获取拉流帧信息
FrameInfo* HG_GetFrameInfo(void* pHandle);

// ======================================
// 设置推流参数
// nEncodeType: 0-h264， 1-h265
bool HG_SetFrameInfo(const char* pPlayId, const int nWidth = 1920, const int nHeight = 1080,
                                            const int nPort = 8888, const int nEncodeType = 0);
// 开始推流
bool HG_StartServer();
// 结束推流
void HG_StopSever();
// 推流帧
bool HG_PutFrame(const char* playId, const unsigned char* buffer, const unsigned int size);
```

# 编译设置
见`CMakeLists.txt`。
```sh
# 设置True为生成so库，False为生成可执行文件，可执行文件依赖OpenCV
set(Lib True)
```

# 调用样例
## c++样例：
```c++
// 拉流
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
```

```c++
// 推流
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
```

## Python样例