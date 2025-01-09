#include "libExportStream.h"

#include <fstream>
#include "StreamManager.h"

extern "C"
// ======================================
void* HG_GetRtspClient(const char* pUri, const int nRtpType) {
    return StreamManager::getInstance()->HG_GetRtspClient(pUri, nRtpType);
}

void HG_CloseClient(void* pHandle) {
    StreamManager::getInstance()->HG_CloseClient(pHandle);
}

Frame* HG_ReadFrame(void* pHandle) {
    return StreamManager::getInstance()->HG_ReadFrame(pHandle);
}

FrameInfo* HG_GetFrameInfo(void* pHandle) {
    return StreamManager::getInstance()->HG_GetFrameInfo(pHandle);
}

// ======================================
bool HG_SetFrameInfo(const char* pPlayId, const int nWidth, const int nHeight,
                    const int nPort, const int nEncodeType) {
    return StreamManager::getInstance()->HG_SetFrameInfo(pPlayId, nWidth, nHeight, nPort, nEncodeType);
}

bool HG_StartServer() {
    return StreamManager::getInstance()->HG_StartServer();
}

bool HG_PutFrame(const char* pPlayId, const unsigned char* pBuffer, const unsigned int size) {
    return StreamManager::getInstance()->HG_PutFrame(pPlayId, pBuffer, size);
}

void HG_StopSever() {
    StreamManager::getInstance()->HG_StopSever();
}

// ======================================
void HG_CreateRtspSink(const char* pPlayId) {
    StreamManager::getInstance()->HG_CreateRtspSink(pPlayId);
}

void HG_CreateRtpSink(const char* pPeerURL) {
    StreamManager::getInstance()->HG_CreateRtpSink(pPeerURL);
}

void HG_StopSink(const char* playId) {
    StreamManager::getInstance()->HG_StopSink(playId);
}

float HG_GetVersion() {
    return StreamManager::getInstance()->HG_GetVersion();
}