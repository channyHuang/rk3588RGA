#include "Detector.h"

#include "image_rga.h"
#include "math.h"

// #include "opencv2/core/core.hpp"
// #include "opencv2/highgui/highgui.hpp"
// #include "opencv2/imgproc/imgproc.hpp"

#define PRINT printf("--------------[%d] %s\n", __LINE__, __FILE__);
const char *pVersion = "version 1.0.1";

Detector* Detector::m_pInstance = nullptr;

Detector::Detector() {}
// 释放资源
Detector::~Detector() {
    release();
    if (m_pInstance != nullptr) {
        delete m_pInstance;
    }
    m_pInstance = nullptr;
}
// 初始化，输入权重模型路径名称，加载模型
bool Detector::init(const char* model_path) {
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    int ret = init_post_process();
    if (ret != 0) {
        printf("init_post_process fail! ret = %d model_path=%s\n", ret, model_path);
        return false;
    }
#ifdef ENABLE_V5
    ret = init_yolov5_model(model_path, &rknn_app_ctx);
#else
    ret = init_yolov8_model(model_path, &rknn_app_ctx);
#endif
    if (ret != 0) {
        release();
        return false;
    }
    memset(&src_image, 0, sizeof(image_buffer_t));
    updateFillSize();
    return true;
}

bool Detector::init_crop(const char* model_path, int nThreadNum) {
    memset(&src_image, 0, sizeof(image_buffer_t));
    int ret = init_post_process();
    if (ret != 0) {
        printf("init_post_process fail! ret = %d model_path=%s\n", ret, model_path);
        return false;
    }
    // 线程池
    // m_pool = RknnPool<YoloManager, StImgWithId, StImgWithId>(model_path, nThreadNum);
    m_pool.setPathAndNum(model_path, nThreadNum);
    ret = m_pool.init();
    if (ret != 0) {
        printf("RknnPool init failed! model_path = %s\n", model_path);
        return false;
    }

    return true;
}

bool Detector::deinit() {
    release();
    return true;
}

bool Detector::deinit_crop() {
    memset(&src_image, 0, sizeof(image_buffer_t));
    deinit_post_process();
    m_pool.deinit();
    return true;
}

// 释放传输到外部的检测结果资源
void resetStDetectResult(stDetectResult &result) {
    if (result.nDetectNum == 0) return;
    result.nDetectNum = 0;
    delete []result.pBoxes;
    delete []result.pProb;
    delete []result.pClasses;
}
// 复制图像数据到输入图像结构
bool Detector::copyImageData(char* pChar, int nWidth, int nHeight, image_buffer_t& image) {
    int sw_out_size = nWidth * nHeight * 3;
    if ((image.virt_addr != NULL) && (image.width != nWidth || image.height != nHeight)) {
        // printf("image size changed, reallocate memory %d...\n", sw_out_size);
        free(image.virt_addr);
        memset(&image, 0, sizeof(image_buffer_t));
        image.virt_addr = NULL;
    }
    
    image.width = nWidth;
    image.height = nHeight;
    image.size = sw_out_size;
    image.format = IMAGE_FORMAT_RGB888;
    // unsigned char* sw_out_buf = image.virt_addr;
    if (image.virt_addr == NULL) {
        // printf("copyImageData malloc %d\n", sw_out_size);
        image.virt_addr = (unsigned char*)malloc(sw_out_size * sizeof(unsigned char));
        // ret = set_image_dma_buf_alloc(&image);
    }
    if (image.virt_addr == NULL) {
        printf("sw_out_buf is NULL, ret = %d\n", ret);
        return false;
    }
    memcpy(image.virt_addr, pChar, sw_out_size);

    return true;
}

// 检测，输入图像数据及图像宽高，输出检测信息
stDetectResult* Detector::detect(char* pChar, int nWidth, int nHeight) {
    bool suc = copyImageData(pChar, nWidth, nHeight, src_image);
    if (!suc) {
        return &stResult;
    }

    // 检测及后处理
#ifdef ENABLE_V5
    ret = inference_yolov5_model(&rknn_app_ctx, &src_image, &od_results, &dst_img);
#else
    ret = inference_yolov8_model(&rknn_app_ctx, &src_image, &od_results, &dst_img);
#endif
    if (ret != 0)
    {
        printf("inference_yolov5_model fail! ret=%d\n", ret);
        return &stResult;
    }
    
    // 填充传输到外部的检测结果
    resetStDetectResult(stResult);
    if (od_results.count > 0) {
        stResult.nDetectNum = od_results.count;
        stResult.pClasses = new int[od_results.count];
        stResult.pBoxes = new int[od_results.count << 2];
        stResult.pProb = new float[od_results.count];
    }

    // 画框和概率
    char text[256];
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result *det_result = &(od_results.results[i]);
        // printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
        //     det_result->box.left, det_result->box.top,
        //     det_result->box.right, det_result->box.bottom,
        //     det_result->prop);
        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;

        draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

        sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);

        stResult.pClasses[i] = det_result->cls_id;
        stResult.pBoxes[(i << 2)] = det_result->box.left;
        stResult.pBoxes[(i << 2) + 1] = det_result->box.top;
        stResult.pBoxes[(i << 2) + 2] = det_result->box.right;
        stResult.pBoxes[(i << 2) + 3] = det_result->box.bottom;
        stResult.pProb[i] = det_result->prop;
    }
    
    // write_image("out.png", &src_image);
    if (stResult.pFrame != nullptr && (stResult.nWidth != src_image.width || stResult.nHeight != src_image.height)) {
        free(stResult.pFrame);
        stResult.pFrame = nullptr;
    }
    if (stResult.pFrame == nullptr) {
        // stResult.pFrame = new unsigned char[src_image.size];
        stResult.pFrame = (unsigned char*)malloc(src_image.size * sizeof(unsigned char));
    }
    memcpy(stResult.pFrame, src_image.virt_addr, src_image.size * sizeof(unsigned char));
    stResult.nWidth = src_image.width;
    stResult.nHeight = src_image.height;
    return &stResult;
}

stDetectResult* Detector::detect_crop(char* pChar, int nWidth, int nHeight) {
    bool suc = copyImageData(pChar, nWidth, nHeight, src_image);
    if (!suc) {
        printf("input frame convert to image failed\n");
        return &stResult;
    }
    
    if (m_nImgWidth != nWidth || m_nImgHeight != nHeight) {
        m_vOffset.clear();
        m_vOffset = calculateOffset(nWidth, nHeight, m_nCropWidth, m_nCropHeight);
        m_nImgHeight = nHeight;
        m_nImgWidth = nWidth;
        printf("------ crop from (%d %d) to (%d %d) * %d\n", nWidth, nHeight, m_nCropWidth, m_nCropHeight, m_vOffset.size());
    }

    char *buf[m_vOffset.size()];
    for (int i = 0; i < m_vOffset.size(); ++i) {
        buf[i] = rga_crop_and_resize_fill(pChar, nWidth, nHeight, m_nModelWidth, m_nModelHeight, m_vOffset[i].second, m_vOffset[i].first, m_nCropWidth, m_nCropHeight );
        
        StImgWithId stImgWithId;
        stImgWithId.id = i;
        stImgWithId.nHeigth = m_nModelHeight;
        stImgWithId.nWidth = m_nModelWidth;
        stImgWithId.pChar = buf[i];
        stImgWithId.bRotate = (m_nCropWidth < m_nCropHeight ? true : false);
        stImgWithId.nAddWidth = ALIGN(m_nCropWidth, 16) - m_nCropWidth;
        stImgWithId.nAddHeight = ALIGN(m_nCropHeight, 16) - m_nCropHeight;
        m_pool.put(stImgWithId);
    }

    if (m_vOffset.size() > m_vStResults.size()) {
        m_vStResults.resize(m_vOffset.size());
    }
    for (int i = 0; i < m_vStResults.size(); ++i) {
        resetStDetectResult(m_vStResults[i]);
    }
    resetStDetectResult(stResult);
    
    for (int i = 0; i < m_vOffset.size(); ++i) {  
        StImgWithId res_stImgWithId;
        suc = m_pool.get(res_stImgWithId);
        if (suc != 0) {
            i--;
            continue;
        }
        // 填充传输到外部的检测结果
        if (res_stImgWithId.od_results.count > 0) {
            m_vStResults[i].nDetectNum = res_stImgWithId.od_results.count;
            m_vStResults[i].pClasses = new int[res_stImgWithId.od_results.count];
            m_vStResults[i].pBoxes = new int[res_stImgWithId.od_results.count << 2];
            m_vStResults[i].pProb = new float[res_stImgWithId.od_results.count];
        }
        stResult.nDetectNum += res_stImgWithId.od_results.count;

        // cv::Mat subimg = cv::Mat::zeros(m_nModelHeight, m_nModelWidth, CV_8UC3);
        // memcpy(subimg.data, res_stImgWithId.pChar, m_nModelHeight * m_nModelWidth * 3);
        // cv::Mat sizeimg;
        // cv::resize(subimg, sizeimg, cv::Size(m_nFillHeight, m_nFillWidth));
        // cv::Mat rotimg;
        // cv::rotate(sizeimg, rotimg, 2);

        // 画框和概率
        char text[256];
        for (int j = 0; j < res_stImgWithId.od_results.count; j++)
        {
            object_detect_result *det_result = &(res_stImgWithId.od_results.results[j]);
            // printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
            //     det_result->box.left, det_result->box.top,
            //     det_result->box.right, det_result->box.bottom,
            //     det_result->prop);
            int x1 = det_result->box.left;
            int y1 = det_result->box.top;
            int x2 = det_result->box.right;
            int y2 = det_result->box.bottom;
            // cv::rectangle(subimg, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 3);
            // cv::rectangle(sizeimg, cv::Point(x1 * m_nFillHeight * 1.f/ m_nModelWidth, y1* m_nFillWidth * 1.f / m_nModelHeight), 
            //                 cv::Point(x2 * m_nFillHeight * 1.f / m_nModelWidth, y2* m_nFillWidth * 1.f / m_nModelHeight), cv::Scalar(0, 0, 255), 3);
            if (res_stImgWithId.bRotate) {
                // resize model size -> fill size
                x1 = (std::floor(x1 * m_nFillHeight * 1.f/ m_nModelWidth));
                y1 = (std::floor(y1 * m_nFillWidth * 1.f / m_nModelHeight) );
                x2 = (std::ceil(x2 * m_nFillHeight * 1.f / m_nModelWidth));
                y2 = (std::ceil(y2 * m_nFillWidth * 1.f / m_nModelHeight));
                // cv::rectangle(sizeimg, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 3);
                // rotate
                x1 = y1;
                y1 = m_nFillHeight - x2;
                x2 = y2;
                y2 = y1 + std::abs(x2 - x1);

                // cv::rectangle(rotimg, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 3);
                // offset
                x1 += m_vOffset[i].second;
                y1 += m_vOffset[i].first;
                x2 += m_vOffset[i].second;
                y2 += + m_vOffset[i].first;
            } else {
                x1 = (x1 * m_nFillWidth * 1.f/ m_nModelWidth + m_vOffset[i].second);
                y1 = (y1 * m_nFillHeight * 1.f / m_nModelHeight + m_vOffset[i].first);
                x2 = (x2 * m_nFillWidth * 1.f / m_nModelWidth + m_vOffset[i].second);
                y2 = (y2 * m_nFillHeight * 1.f / m_nModelHeight + m_vOffset[i].first);
            }

            draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

            sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
            draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);

            m_vStResults[i].pClasses[j] = det_result->cls_id;
            m_vStResults[i].pBoxes[(j << 2)] = x1;
            m_vStResults[i].pBoxes[(j << 2) + 1] = y1;
            m_vStResults[i].pBoxes[(j << 2) + 2] = x2;
            m_vStResults[i].pBoxes[(j << 2) + 3] = y2;
            m_vStResults[i].pProb[j] = det_result->prop;
        }
    }
    // write_image("out.png", &src_image);
    stResult.pClasses = new int[stResult.nDetectNum];
    stResult.pBoxes = new int[stResult.nDetectNum << 2];
    stResult.pProb = new float[stResult.nDetectNum];
    int idx = 0;
    for (int i = 0; i < m_vOffset.size(); ++i) {
        for (int j = 0; j < m_vStResults[i].nDetectNum; ++j) {
            stResult.pClasses[idx] = m_vStResults[i].pClasses[j];
            stResult.pBoxes[(idx << 2)] = m_vStResults[i].pBoxes[j << 2];
            stResult.pBoxes[(idx << 2) + 1] = m_vStResults[i].pBoxes[(j << 2) + 1];
            stResult.pBoxes[(idx << 2) + 2] = m_vStResults[i].pBoxes[(j << 2) + 2];
            stResult.pBoxes[(idx << 2) + 3] = m_vStResults[i].pBoxes[(j << 2) + 3];
            stResult.pProb[idx] = m_vStResults[i].pProb[j];
            idx++;
        }
        if (buf[i])
            free(buf[i]);
    }
    if (stResult.pFrame != nullptr && (stResult.nWidth != src_image.width || stResult.nHeight != src_image.height)) {
        free(stResult.pFrame);
        stResult.pFrame = nullptr;
    }
    if (stResult.pFrame == nullptr) {
        // stResult.pFrame = new unsigned char[src_image.size];
        stResult.pFrame = (unsigned char*)malloc(src_image.size * sizeof(unsigned char));
    }
    memcpy(stResult.pFrame, src_image.virt_addr, src_image.size * sizeof(unsigned char));
    stResult.nWidth = src_image.width;
    stResult.nHeight = src_image.height;

    return &stResult;  
}

void Detector::release() {
    printf("Detector::release\n");
    deinit_post_process();
#ifdef ENABLE_V5
    ret = release_yolov5_model(&rknn_app_ctx);
#else
    ret = release_yolov8_model(&rknn_app_ctx);
#endif
    if (ret != 0)
    {
        printf("release_yolov8_model fail! ret=%d\n", ret);
    }

    // if (src_image.virt_addr != NULL)
    // {
    //     free(src_image.virt_addr);
    // }
}

// 设置目标检测置信度阈值，即时生效 
void Detector::setThreshold(float threshold) {
    printf("setThreshold %f\n", threshold);
    BOX_THRESH = threshold;
}
void Detector::setThresholdList(float* fThresholdList, int nThresholdNum) {
    printf("setThresholdList\n");
    for (int i = 0; i < nThresholdNum; ++i) {
        printf("%f ", fThresholdList[i]);
        BOX_THRESH_LIST[i] = fThresholdList[i];
    }
    printf("\n");
}
// 设置模型对应的目标总类型数
void Detector::setClassNum(int nClassNum) {
    printf("setClassNum %d\n", nClassNum);
    OBJ_CLASS_NUM = nClassNum;
}
// 打印性能统计信息到控制台
void Detector::printProfile() {
    // performance
    rknn_perf_detail perf_detail;
    ret = rknn_query(rknn_app_ctx.rknn_ctx, RKNN_QUERY_PERF_DETAIL, &perf_detail, sizeof(perf_detail));
    printf("---> %s\n", perf_detail.perf_data);  
}

// // 
stDetectResult** Detector::detectBatch(char** pChar, int nBatch, int nWidth, int nHeight) {
//     std::queue< std::future<stDetectResult*> > results;
//     for (int i = 0; i < nBatch; ++i) {
//         results.push(
//             pool.enqueue([&] {
//                 image_buffer_t input_image, output_image;
//                 stDetectResult stDetectRes;
//                 object_detect_result_list od_result;

//                 memset(&input_image, 0, sizeof(image_buffer_t));
//                 resetStDetectResult(stDetectRes);

//                 bool suc = copyImageData(pChar[i], nWidth, nHeight, input_image);
//                 if (!suc) {
//                     return &stDetectRes;
//                 }
//                 // 检测及后处理
//                 ret = inference_yolov8_model(&rknn_app_ctx, &input_image, &od_result, &output_image);
//                 if (ret != 0)
//                 {
//                     printf("inference_yolov8_model fail! ret=%d\n", ret);
//                     return &stDetectRes;
//                 }
//                 // 填充传输到外部的检测结果
//                 if (od_result.count > 0) {
//                     stDetectRes.nDetectNum = od_result.count;
//                     stDetectRes.pClasses = new int[od_result.count];
//                     stDetectRes.pBoxes = new int[od_result.count << 2];
//                     stDetectRes.pProb = new float[od_result.count];
//                 }
//                 // 画框和概率
//                 char text[256];
//                 for (int i = 0; i < od_result.count; i++)
//                 {
//                     object_detect_result *det_result = &(od_result.results[i]);
//                     // printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
//                     //     det_result->box.left, det_result->box.top,
//                     //     det_result->box.right, det_result->box.bottom,
//                     //     det_result->prop);
//                     int x1 = det_result->box.left;
//                     int y1 = det_result->box.top;
//                     int x2 = det_result->box.right;
//                     int y2 = det_result->box.bottom;

//                     draw_rectangle(&input_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

//                     sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
//                     draw_text(&input_image, text, x1, y1 - 20, COLOR_RED, 10);

//                     stDetectRes.pClasses[i] = det_result->cls_id;
//                     stDetectRes.pBoxes[(i << 2)] = det_result->box.left;
//                     stDetectRes.pBoxes[(i << 2) + 1] = det_result->box.top;
//                     stDetectRes.pBoxes[(i << 2) + 2] = det_result->box.right;
//                     stDetectRes.pBoxes[(i << 2) + 3] = det_result->box.bottom;
//                     stDetectRes.pProb[i] = det_result->prop;
//                 }
//                 if (stDetectRes.pFrame == nullptr) {
//                     stDetectRes.pFrame = new unsigned char[input_image.size];
//                 }
//                 memcpy(stDetectRes.pFrame, input_image.virt_addr, input_image.size * sizeof(unsigned char));
//                 stDetectRes.nWidth = input_image.width;
//                 stDetectRes.nHeight = input_image.height;

//                 return &stDetectRes;
//             })
//         );
//     }
    
    stDetectResult* res[nBatch];
//     int i = 0;
//     while (!results.empty()) {
//         auto &result = results.front();
//         results.pop();
//         res[i] = result.get();
//         i++;
//     }
    return res;
}

// 设置输出branch数
void Detector::setBranchNum(int nBranchNum) {
    g_nOutputBranchNum = nBranchNum;
    printf("setBranchNum to %d\n", nBranchNum);
}

void Detector::setRootDirectory(const char* pChar) {
    g_sRootDirectory = std::string(pChar);
}

void Detector::setCropSize(int nWidth, int nHeight) {
    m_nCropWidth = nWidth;
    m_nCropHeight = nHeight;
    printf("setCropSize to (w = %d, h = %d)\n", m_nCropWidth, m_nCropHeight);
    updateFillSize();
}

void Detector::updateFillSize() {
    if (m_nCropWidth < m_nCropHeight) {
        m_nFillWidth = ALIGN(m_nCropWidth, 16);
        m_nFillHeight = ALIGN(m_nCropHeight, 16);
    } else {
        m_nFillWidth = m_nCropWidth;
        m_nFillHeight = m_nCropHeight;
    }
}
