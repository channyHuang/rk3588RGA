#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <cstring>
#include <chrono>
#include <queue>
#include <mutex>

namespace fs = std::experimental::filesystem;

#include "YoloManager.h"
#include "utils/image_utils.h"
#include "utils/image_drawing.h"

#include "libExport.h"
#include "RknnPool.hpp"
#ifdef ENABLE_V5
#include "./yolov5/yolov5.h"
#include "./yolov5/postprocess.h"
#else
#include "./yolov8/yolov8.h"
#include "./yolov8/postprocess.h"
#endif

// typedef struct stDetectResult {
//     char* pFrame;
//     int nDetectNum;
//     int* classes;
//     int* boxes;
//     float* prob;
// };

// 用于导出的检测类Detector
class Detector {
public:
    static Detector *getInstance() {
        if (m_pInstance == nullptr) {
            m_pInstance = new Detector();
        }
        return m_pInstance;
    }
    ~Detector();

    bool init_old(const char* model_path);
    bool init(const char*model_path, int nThreadNum = 3);
    bool deinit_old();
    bool deinit();
    stDetectResult* detect_old(char* pChar, int nWidth = 1920, int nHeight = 1080);
    stDetectResult* detect(char* pChar, int src_width, int src_height);
    void setThreshold(float fThreshold = 0.8f);
    void setThresholdList(float* fThresholdList, int nThresholdNum = 3);
    void setClassNum(int nClassNum = 13);
    void setCropSize(int nWidth = 640, int nHeight = 640);
    void printProfile();

    stDetectResult** detectBatch(char** pChar, int nBatch = 3, int nWidth = 1920, int nHeight = 1080);

    // 通用接口
    // 设置输出branch数
    void setBranchNum(int nBranchNum);
    void setRootDirectory(const char* pChar);


private:
    static Detector *m_pInstance;

    int ret = 0;
    // 输入输出图像
    image_buffer_t src_image;
    image_buffer_t dst_img;
    // 检测上下文
    rknn_app_context_t rknn_app_ctx;
    // 检测结果
    object_detect_result_list od_results;
    // 传输到外部的检测结果 
    stDetectResult stResult;
    // 回调函数
    // CBFun_Callback m_pCallback = nullptr;
    // 回调函数传入的用户数据
    void* m_pUser = nullptr;
    // 线程锁
    std::mutex mutex;
    
    bool m_bRunning = false;
    std::queue<image_buffer_t> input_images;
    std::queue<image_buffer_t> output_images;

private:
    Detector();
    void release();
    // 复制图像数据
    bool copyImageData(char* pChar, int nWidth, int nHeight, image_buffer_t& image);
    void updateFillSize();
    
    RknnPool<YoloManager, StImgWithId, StImgWithId> m_pool;

    std::vector<std::pair<int, int>> m_vOffset;
    int m_nCropWidth = 640, m_nCropHeight = 1080;
    int m_nFillWidth = 640, m_nFillHeight = 1088;
    int m_nImgWidth = 0, m_nImgHeight = 0;
    int m_nModelWidth = 1920, m_nModelHeight = 1088;

    std::vector<stDetectResult> m_vStResults;
};
