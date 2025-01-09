# rk3588RGA
using rga in 3588 codes and notes

## HGStream
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

## HGRknnDetect
Yolov5/Yolov8目标检测库。支持v5和v8。支持大图像(如1920x1080)分片成小图像(如640x640)开多线程检测再合并成大图像输出。

接口：
```c++
// 初始化，输入权重模型路径名称，加载模型
bool init(const char* pModelString);
// 分片初始化
bool init_crop(const char* pModelString);
// 反初始化，释放模型 
bool deinit();
bool deinit_crop();
// 检测，输入图像数据及图像宽高，输出检测信息
stDetectResult* detect(char* pChar, int nWidth, int nHeight);
// 分片检测
stDetectResult* detect_crop(char* pChar, int src_width, int src_height);
// 打印性能统计信息到控制台
void printProfile();

// 设置目标检测置信度阈值，即时生效 
void setThreshold(float fThreshold);
// 设置目标检测置信度阈值，每个类别使用不同的置信度，即时生效，上述单置信度为基础过滤
void setThresholdList(float* fThresholdList, int nThresholdNum = 3);
// 设置模型对应的目标总类型数 
void setClassNum(int nClassNum);
// 设置输出branch数
void setBranchNum(int nBranchNum = 3);
// 设置根目录
void setRootDirectory(const char* pChar);
// 设置Anchor文件路径和名称，仅yolov5有效
void setAnchorFile(const char* pChar);
// 设置分片长宽
void setCropSize(int nWidth, int nHeight);
```
