#include <iostream>
#include <mutex>

#include "rknn_api.h"
#include "./utils/file_utils.h"
#ifdef ENABLE_V5
#include "./yolov5/yolov5.h"
#else
#include "./yolov8/yolov8.h"
#endif

struct StImgWithId {
    char* pChar = nullptr;
    size_t nWidth = 0;
    size_t nHeigth = 0;
    int id = 0;
    bool bRotate = false;
    size_t nAddWidth = 0;
    size_t nAddHeight = 0;
    object_detect_result_list od_results;
};

class YoloManager {
public:
    YoloManager(const std::string &sModelPath);
    ~YoloManager();
    
    int init(rknn_context *ctx, bool bShareWeight);
    void deinit(bool bShareWeight = true);
    rknn_context *get_ctx();
    StImgWithId infer(StImgWithId &stImgWithId);

    image_buffer_t src_img, dst_img;

private:
    std::mutex m_mutex;
    std::string m_sModelPath;
    rknn_app_context_t app_ctx;
    rknn_input m_rknnInputs[1];
    int m_nModelChannel, m_nModelWidth, m_nModelHeight;
    int m_nImgWidth, m_nImgHeight;

    int ret;
    unsigned char* pModel = nullptr;

    
    object_detect_result_list od_results;
    
};
