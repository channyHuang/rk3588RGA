#include "libExport.h"

#include <fstream>
#include "Detector.h"

extern "C"

void hglog(const char* pString)
{
    std::ofstream off("liblog.txt", std::ios::app);
    off << pString << std::endl;
    off.close();
}

bool init(const char* pModelString)
{
    return Detector::getInstance()->init(pModelString);
}

bool init_crop(const char* pModelString)
{
    return Detector::getInstance()->init_crop(pModelString);
}

stDetectResult* detect(char* pChar, int nWidth, int nHeight)
{
    return Detector::getInstance()->detect(pChar, nWidth, nHeight);
}

stDetectResult* detect_crop(char* pChar, int src_width, int src_height) {
    return Detector::getInstance()->detect_crop(pChar, src_width, src_height);
}

bool deinit()
{
    return Detector::getInstance()->deinit();
}

bool deinit_crop()
{
    return Detector::getInstance()->deinit_crop();
}

void setThreshold(float fThreshold) {
    Detector::getInstance()->setThreshold(fThreshold);
}

void setThresholdList(float* fThresholdList, int nThresholdNum) {
    Detector::getInstance()->setThresholdList(fThresholdList, nThresholdNum);
}

void setClassNum(int nClassNum) {
    Detector::getInstance()->setClassNum(nClassNum);
}

void setCropSize(int nWidth, int nHeight) {
    Detector::getInstance()->setCropSize(nWidth, nHeight);
}

void printProfile() {
    Detector::getInstance()->printProfile();
}

stDetectResult** detectBatch(char** pChar, int nBatch, int nWidth, int nHeight) {
    return Detector::getInstance()->detectBatch(pChar, nBatch, nWidth, nHeight);
}

// 通用接口
// 设置输出branch数
void setBranchNum(int nBranchNum) {
    Detector::getInstance()->setBranchNum(nBranchNum);
}
// 设置根目录
void setRootDirectory(const char* pChar) {
    Detector::getInstance()->setRootDirectory(pChar);
}
// 设置Anchor文件路径和名称，仅yolov5有效
void setAnchorFile(const char* pChar) {}
