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

bool init_old(const char* pModelString)
{
    return Detector::getInstance()->init_old(pModelString);
}

bool init(const char* pModelString)
{
    return Detector::getInstance()->init(pModelString);
}

stDetectResult* detect_old(char* pChar, int nWidth, int nHeight)
{
    return Detector::getInstance()->detect_old(pChar, nWidth, nHeight);
}

stDetectResult* detect(char* pChar, int src_width, int src_height) {
    return Detector::getInstance()->detect(pChar, src_width, src_height);
}

bool deinit_old()
{
    return Detector::getInstance()->deinit_old();
}

bool deinit()
{
    return Detector::getInstance()->deinit();
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
