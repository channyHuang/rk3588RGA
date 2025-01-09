#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <cstring>
#include <chrono>
#include <queue>
#include <mutex>

#include "module/vi/module_rtspClient.hpp"
#include "module/vp/module_mppdec.hpp"

#include "module/vi/module_memReader.hpp"
#include "module/vp/module_rga.hpp"
#include "module/vp/module_mppenc.hpp"
#include "module/vo/module_rtspServer.hpp"
#include "module/vo/module_rtmpServer.hpp"

#include "module/vo/module_fileWriter.hpp"
#include "module/vi/module_fileReader.hpp"

#include "libExportStream.h"

namespace fs = std::experimental::filesystem;

class StreamManager;

struct StCallback {
    StreamManager* pManager;
};

struct StPushCallback {
    shared_ptr<ModuleRga> rga;
    shared_ptr<VideoBuffer> vb;
};

#define MAXQUEUESIZE 3

class StreamManager {
public:
    static StreamManager *getInstance() {
        if (m_pInstance == nullptr) {
            m_pInstance = new StreamManager();
        }
        return m_pInstance;
    }
    ~StreamManager();

    // ======================================
    void* HG_GetRtspClient(const char* pUri, int rtptype = 0);
    void HG_CloseClient(void* pHandle);
    Frame* HG_ReadFrame(void* pHandle);
    FrameInfo* HG_GetFrameInfo(void* pHandle);

    // ======================================
    bool HG_SetFrameInfo(const char* pPlayId, const int nWidth = 1920, const int nHeight = 1088,
                    const int nPort = 8888, const int nEncodeType = 0);
    bool HG_StartServer();
    bool HG_PutFrame(const char* playId, const unsigned char* buffer, const unsigned int size);
    void HG_StopSever();

    // ======================================
    bool HG_CreateRtspSink(const char* pPlayId, int nPort = 554);
    void HG_StopSink(const char* playId);
    void HG_CreateRtpSink(const char* pPeerURL);
    float HG_GetVersion();

    // ======================================
    void AddFrame(unsigned char* pChar, int nWidth, int nHeight, int size);

private:
    static StreamManager *m_pInstance;
    std::mutex m_quMutex;
    
    int ret = 0;
    int m_nBufferSize = 0;
    // 拉流
    std::shared_ptr<ModuleRtspClient> m_pRtspClient = nullptr;
    std::shared_ptr<ModuleMppDec> m_pMppDec = nullptr;
    StCallback *m_pcallback = nullptr;
    StPushCallback *m_pstPushCallback = nullptr;
    ImagePara m_stInputPara;

    // 拉流返回数据
    FrameInfo info;
    Frame* m_pFrameList[MAXQUEUESIZE];
    std::queue<Frame*> m_quFrames;
    int m_nFrameIdx = 0;
    
    // 推流
    std::shared_ptr<ModuleMemReader> m_pMemReader = nullptr;
    std::shared_ptr<ModuleRga> m_pRga = nullptr;
    std::shared_ptr<ModuleMppEnc> m_pMppEnc = nullptr;
    std::shared_ptr<ModuleRtspServer> m_pRtspServer = nullptr;
    std::shared_ptr<ModuleRtmpServer> m_pRtmpServer = nullptr;
    std::shared_ptr<VideoBuffer> m_pVideoBuffer = nullptr;
    std::string m_sPushPath = "/live/0";
    int m_nPort = 554;
    ImagePara m_stPushPara = {1920, 1088, 1920, 1080, V4L2_PIX_FMT_H264};

    // 
    std::shared_ptr<ModuleFileWriter> m_pFileWriter = nullptr;

private:
    StreamManager();
};
