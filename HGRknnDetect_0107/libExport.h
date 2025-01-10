
#ifndef DLLEXPORT_H
#define DLLEXPORT_H

#include <string.h>
#include <string>

#ifdef __cplusplus
#define D_EXTERN_C extern "C"
#else
#define D_EXTERN_C
#endif

#define __SHARE_EXPORT

#ifdef __SHARE_EXPORT
#define D_SHARE_EXPORT D_DECL_EXPORT
#else
#define D_SHARE_EXPORT D_DECL_IMPORT
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
#define D_CALLTYPE __stdcall
#define D_DECL_EXPORT __declspec(dllexport)
#define D_DECL_IMPORT
#else
#define __stdcall
#define D_CALLTYPE
#define D_DECL_EXPORT __attribute__((visibility("default")))
#define D_DECL_IMPORT __attribute__((visibility("default")))
#endif

// 目标检测返回数据结构
typedef struct stDetectResult {
    // 带目标框的返回图像 
    unsigned char* pFrame = nullptr;
    // 检测到的目标数量
    int nDetectNum = 0;
    // 返回的图像宽度 
    int nWidth = 0;
    // 返回的图像高度
    int nHeight = 0;
    // 返回的目标类型数组[cls1, cls2, ...]
    int* pClasses = nullptr;
    // 返回的目标框数组[left1, top1, right1, bottom1, left2, ...]
    int* pBoxes = nullptr;
    // 返回的目标置信度
    float* pProb = nullptr;
};

// 初始化，输入权重模型路径名称，加载模型
D_EXTERN_C D_SHARE_EXPORT bool init_old(const char* pModelString);
// 分片初始化
D_EXTERN_C D_SHARE_EXPORT bool init(const char* pModelString);
// 反初始化，释放模型 
D_EXTERN_C D_SHARE_EXPORT bool deinit_old();
D_EXTERN_C D_SHARE_EXPORT bool deinit();
// 检测，输入图像数据及图像宽高，输出检测信息
D_EXTERN_C D_SHARE_EXPORT stDetectResult* detect_old(char* pChar, int nWidth, int nHeight);
// 分片检测
D_EXTERN_C D_SHARE_EXPORT stDetectResult* detect(char* pChar, int src_width, int src_height);
// 打印性能统计信息到控制台
D_EXTERN_C D_SHARE_EXPORT void printProfile();

// 设置目标检测置信度阈值，即时生效 
D_EXTERN_C D_SHARE_EXPORT void setThreshold(float fThreshold);
// 设置目标检测置信度阈值，每个类别使用不同的置信度，即时生效，上述单置信度为基础过滤
D_EXTERN_C D_SHARE_EXPORT void setThresholdList(float* fThresholdList, int nThresholdNum = 3);
// 设置模型对应的目标总类型数 
D_EXTERN_C D_SHARE_EXPORT void setClassNum(int nClassNum);
// 设置输出branch数
D_EXTERN_C D_SHARE_EXPORT void setBranchNum(int nBranchNum = 3);
// 设置根目录
D_EXTERN_C D_SHARE_EXPORT void setRootDirectory(const char* pChar);
// 设置Anchor文件路径和名称，仅yolov5有效
D_EXTERN_C D_SHARE_EXPORT void setAnchorFile(const char* pChar);
// 设置分片长宽
D_EXTERN_C D_SHARE_EXPORT void setCropSize(int nWidth, int nHeight);

// 多batch
D_EXTERN_C D_SHARE_EXPORT stDetectResult** detectBatch(char** pChar, int nBatch, int nWidth, int nHeight);
// 写日志 
D_EXTERN_C D_SHARE_EXPORT void hglog(const char* pString);

#endif // DLLEXPORT_H