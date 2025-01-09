#include "StreamManager.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#define PRINT printf("--------------[%d] %s\n", __LINE__, __FILE__);
const char *pVersion = "version 1.0.1";

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

void funCallback(void* pStCb, std::shared_ptr<MediaBuffer> pBuffer) {
    if (pBuffer == nullptr || pBuffer->getMediaBufferType() != BUFFER_TYPE_VIDEO) {
        return;
    }
    StCallback *pStCallback = static_cast<StCallback*>(pStCb);
    StreamManager* pManager = pStCallback->pManager;
    
    std::shared_ptr<VideoBuffer> pFrameBuf = static_pointer_cast<VideoBuffer>(pBuffer);

    void* pFrame = pFrameBuf->getActiveData();
    size_t size = pFrameBuf->getActiveSize();
    pFrameBuf->invalidateDrmBuf();
    uint32_t nWidth = pFrameBuf->getImagePara().hstride;
    uint32_t nHeight = pFrameBuf->getImagePara().vstride;

    // cv::Mat mat(cv::Size(nWidth, nHeight), CV_8UC3, pFrame);
    // mat = cv::imread("/home/firefly/rknn_installs/HGRknnDetect_0107/input.jpg");
    // pFrameBuf->setActiveData(mat.data);
    // cv::imshow("origin", mat);
    // cv::waitKey(1);
    pManager->AddFrame((unsigned char*)pFrame, nWidth, nHeight, size);
}

StreamManager* StreamManager::m_pInstance = nullptr;

StreamManager::StreamManager() {
    for (int i = 0; i < MAXQUEUESIZE; ++i) {
        m_pFrameList[i] = new Frame();
    }
}
// 释放资源
StreamManager::~StreamManager() {
    for (int i = 0; i < MAXQUEUESIZE; ++i) {
        if (m_pFrameList[i] != nullptr) {
            delete m_pFrameList[i];
            m_pFrameList[i] = nullptr;
        }
    }
    if (m_pInstance != nullptr) {
        delete m_pInstance;
    }
    m_pInstance = nullptr;
}

void StreamManager::AddFrame(unsigned char* pChar, int nWidth, int nHeight, int size) {
    std::lock_guard<std::mutex> locker(m_quMutex);
    m_pFrameList[m_nFrameIdx]->pChar = pChar;
    m_pFrameList[m_nFrameIdx]->col = nWidth;
    m_pFrameList[m_nFrameIdx]->row = nHeight;
    m_pFrameList[m_nFrameIdx]->size = size;
    if (m_quFrames.size() >= 3) {
        m_quFrames.pop();
    }
    m_quFrames.push(m_pFrameList[m_nFrameIdx]);
    m_nFrameIdx = 1 - m_nFrameIdx;
}

// 拉流初始化
void* StreamManager::HG_GetRtspClient(const char* pUri, int rtptype) {
    if (strncmp(pUri, "rtsp", strlen("rtsp")) == 0) {
        m_pRtspClient = make_shared<ModuleRtspClient>(pUri, (rtptype == 0 ? RTSP_STREAM_TYPE_UDP : RTSP_STREAM_TYPE_TCP), true, false);
        m_pRtspClient->setProductor(nullptr);
        // m_pRtspClient->setBufferCount(20);
        ret = m_pRtspClient->init();
        if (ret < 0) {
            ff_error("Failed to init rtsp client\n");
            return nullptr;
        }
    } else {
        
    }

    ImagePara stInputImagePara = m_pRtspClient->getOutputImagePara();

    if ((stInputImagePara.v4l2Fmt == V4L2_PIX_FMT_MJPEG) || 
        (stInputImagePara.v4l2Fmt == V4L2_PIX_FMT_H264) || 
        (stInputImagePara.v4l2Fmt == V4L2_PIX_FMT_HEVC)) {
            m_pMppDec = make_shared<ModuleMppDec>(stInputImagePara);
            m_pMppDec->setProductor(m_pRtspClient);
            // m_pMppDec->setBufferCount(10);
            ret = m_pMppDec->init();
            if (ret < 0) {
                ff_error("Failed to init MppDec\n");
                return nullptr;
            }
        } else {
            return nullptr;
        }

    stInputImagePara = m_pMppDec->getOutputImagePara();
    ImagePara stOutputImagePara = stInputImagePara;
    stOutputImagePara.width = stInputImagePara.width;
    stOutputImagePara.height = stInputImagePara.height;
    stOutputImagePara.hstride = stOutputImagePara.width;
    stOutputImagePara.vstride = stOutputImagePara.height;
    stOutputImagePara.v4l2Fmt = V4L2_PIX_FMT_BGR24;
    m_pRga = make_shared<ModuleRga>(stInputImagePara, stOutputImagePara, RGA_ROTATE_NONE);
    m_pRga->setProductor(m_pMppDec);
    // m_pRga->setBufferCount(2);
    ret = m_pRga->init();
    if (ret < 0) {
        ff_error("rga init failed\n");
        return nullptr;
    }

    m_pcallback = new StCallback();
    m_pcallback->pManager = this;
    m_pRga->setOutputDataCallback(m_pcallback, funCallback);

    m_pRtspClient->start();

    return nullptr;
}

void StreamManager::HG_CloseClient(void* pHandle) {
    m_pRtspClient->stop();
    delete m_pcallback;
}

Frame* StreamManager::HG_ReadFrame(void* pHandle) {
    std::lock_guard<std::mutex> locker(m_quMutex);
    if (m_quFrames.empty()) {
        return nullptr;
    }
    Frame* pFrame = m_quFrames.front();
    m_quFrames.pop();
    return pFrame;
}

FrameInfo* StreamManager::HG_GetFrameInfo(void* pHandle) {
    info.videoW = m_stInputPara.width;
    info.videoH = m_stInputPara.height;
    info.format = m_stInputPara.v4l2Fmt;
    return &info;
}

// ======================================

bool StreamManager::HG_SetFrameInfo(const char* pPlayId, 
                    const int nWidth, const int nHeight, 
                    const int nPort, const int nEncodeType) {
    m_stPushPara.width = nWidth;
    m_stPushPara.height = nHeight;
    m_stPushPara.hstride = ALIGN(nWidth, 8);
    m_stPushPara.vstride = ALIGN(nHeight, 8);
    m_stPushPara.v4l2Fmt = v4l2GetFmtByName("BGR24");
    // m_stPushPara.v4l2Fmt = V4L2_PIX_FMT_BGR24;
    // m_stPushPara.v4l2Fmt = V4L2_PIX_FMT_NV12;
    // m_stPushPara.v4l2Fmt = V4L2_PIX_FMT_BGR32;
    
    m_sPushPath = pPlayId;
    m_nPort = nPort;
    printf("HG_SetFrameInfo\n");
}

bool StreamManager::HG_StartServer() {
    printf("HG_StartServer\n");
    ImagePara stBGRPara = m_stPushPara;
    stBGRPara.v4l2Fmt = V4L2_PIX_FMT_BGR32;
    // stBGRPara.v4l2Fmt = V4L2_PIX_FMT_NV12;
    // stBGRPara.v4l2Fmt = V4L2_PIX_FMT_BGR24;
    m_pVideoBuffer = std::make_shared<VideoBuffer>(VideoBuffer::DRM_BUFFER_CACHEABLE);
    m_pVideoBuffer->allocBuffer(stBGRPara);
    printf("buffer size %d\n", m_pVideoBuffer->getSize());
    if (m_pVideoBuffer->getSize() <= 0) {
        ff_error("Failed to alloc buf\n");
        return false;
    }
    memset(m_pVideoBuffer->getData(), 0xff, m_pVideoBuffer->getSize());

    m_pMemReader = std::make_shared<ModuleMemReader>(m_stPushPara);
    ret = m_pMemReader->init();
    if (ret < 0) {
        ff_error("memory reader init failed\n");
        return false;
    }

    // copy to rga
    m_pRga = make_shared<ModuleRga>(m_stPushPara, RGA_ROTATE_NONE);
    m_pRga->setProductor(m_pMemReader);
    m_pRga->setBufferCount(2);
    m_pRga->setSrcBuffer(m_pVideoBuffer->getData());
    ret = m_pRga->init();
    if (ret < 0) {
        ff_error("Failed to init rga\n");
        return false;
    }

    EncodeType eEncodeType = ENCODE_TYPE_H264;
    m_pMppEnc = make_shared<ModuleMppEnc>(eEncodeType);
    m_pMppEnc->setProductor(m_pRga);
    m_pMppEnc->setBufferCount(8);
    ret = m_pMppEnc->init();
    if (ret < 0) {
        ff_error("Failed to init mppenc\n");
        return false;
    }

    m_pRtspServer = make_shared<ModuleRtspServer>(m_sPushPath.c_str(), 8888);
    m_pRtspServer->setProductor(m_pMppEnc);
    m_pRtspServer->setBufferCount(0);
    m_pRtspServer->init();
    if (ret < 0) {
        ff_error("Failed to init rtsp server\n");
        return false;
    }

    m_pMemReader->start();
    return true;
}

bool StreamManager::HG_PutFrame(const char* playId, const unsigned char* pBuffer, const unsigned int size) {
    m_pVideoBuffer->flushDrmBuf();
    void* pBuf = m_pVideoBuffer->getData();
    memcpy((unsigned char*)pBuf, pBuffer, size);
    m_pRga->setSrcBuffer(pBuf);

    ret = m_pMemReader->setInputBuffer(m_pVideoBuffer->getData(), m_pVideoBuffer->getSize(), m_pVideoBuffer->getBufFd());

    if (ret != 0) {
        ff_error("Failed to set the input buf\n");
        return false;
    }
    ret = m_pMemReader->waitProcess(2000);
    if (ret != 0) {
        ff_warn("Wait timeout\n");
    }
    return true;
}

void StreamManager::HG_StopSever() {
    if (m_pMemReader != nullptr) {
        m_pMemReader->setProcessStatus(ModuleMemReader::DATA_PROCESS_STATUS::PROCESS_STATUS_EXIT);
        m_pMemReader->stop();
        m_pMemReader = nullptr;
    }
}

// ======================================
bool StreamManager::HG_CreateRtspSink(const char* pPlayId, int nPort) {
    m_sPushPath = std::string(pPlayId);
    ImagePara stBGRPara = m_stPushPara;
    stBGRPara.v4l2Fmt = V4L2_PIX_FMT_BGR32;
    // stBGRPara.v4l2Fmt = V4L2_PIX_FMT_NV12;
    // stBGRPara.v4l2Fmt = V4L2_PIX_FMT_BGR24;
    m_pVideoBuffer = std::make_shared<VideoBuffer>(VideoBuffer::DRM_BUFFER_CACHEABLE);
    m_pVideoBuffer->allocBuffer(stBGRPara);
    printf("buffer size %d\n", m_pVideoBuffer->getSize());
    if (m_pVideoBuffer->getSize() <= 0) {
        ff_error("Failed to alloc buf\n");
        return false;
    }
    memset(m_pVideoBuffer->getData(), 0xff, m_pVideoBuffer->getSize());

    m_pMemReader = std::make_shared<ModuleMemReader>(m_stPushPara);
    ret = m_pMemReader->init();
    if (ret < 0) {
        ff_error("memory reader init failed\n");
        return false;
    }

    // copy to rga
    m_pRga = make_shared<ModuleRga>(m_stPushPara, RGA_ROTATE_NONE);
    m_pRga->setProductor(m_pMemReader);
    m_pRga->setBufferCount(2);
    m_pRga->setSrcBuffer(m_pVideoBuffer->getData());
    ret = m_pRga->init();
    if (ret < 0) {
        ff_error("Failed to init rga\n");
        return false;
    }

    EncodeType eEncodeType = ENCODE_TYPE_H264;
    m_pMppEnc = make_shared<ModuleMppEnc>(eEncodeType);
    m_pMppEnc->setProductor(m_pRga);
    m_pMppEnc->setBufferCount(8);
    ret = m_pMppEnc->init();
    if (ret < 0) {
        ff_error("Failed to init mppenc\n");
        return false;
    }

    m_pRtspServer = make_shared<ModuleRtspServer>(m_sPushPath.c_str(), nPort);
    m_pRtspServer->setProductor(m_pMppEnc);
    m_pRtspServer->setBufferCount(0);
    m_pRtspServer->init();
    if (ret < 0) {
        ff_error("Failed to init rtsp server\n");
        return false;
    }

    m_pMemReader->start();
    return true;
}

void StreamManager::HG_CreateRtpSink(const char* pPeerURL) {
    printf("HG_CreateRtpSink\n");
    ImagePara stBGRPara = m_stPushPara;
    stBGRPara.v4l2Fmt = V4L2_PIX_FMT_BGR32;
    // stBGRPara.v4l2Fmt = V4L2_PIX_FMT_NV12;
    // stBGRPara.v4l2Fmt = V4L2_PIX_FMT_BGR24;
    m_pVideoBuffer = std::make_shared<VideoBuffer>(VideoBuffer::DRM_BUFFER_CACHEABLE);
    m_pVideoBuffer->allocBuffer(stBGRPara);
    printf("buffer size %d\n", m_pVideoBuffer->getSize());
    if (m_pVideoBuffer->getSize() <= 0) {
        ff_error("Failed to alloc buf\n");
        return;
    }
    memset(m_pVideoBuffer->getData(), 0xff, m_pVideoBuffer->getSize());

    m_pMemReader = std::make_shared<ModuleMemReader>(m_stPushPara);
    ret = m_pMemReader->init();
    if (ret < 0) {
        ff_error("memory reader init failed\n");
        return ;
    }

    // copy to rga
    m_pRga = make_shared<ModuleRga>(m_stPushPara, RGA_ROTATE_NONE);
    m_pRga->setProductor(m_pMemReader);
    m_pRga->setBufferCount(2);
    m_pRga->setSrcBuffer(m_pVideoBuffer->getData());
    ret = m_pRga->init();
    if (ret < 0) {
        ff_error("Failed to init rga\n");
        return ;
    }

    EncodeType eEncodeType = ENCODE_TYPE_H264;
    m_pMppEnc = make_shared<ModuleMppEnc>(eEncodeType);
    m_pMppEnc->setProductor(m_pRga);
    m_pMppEnc->setBufferCount(8);
    ret = m_pMppEnc->init();
    if (ret < 0) {
        ff_error("Failed to init mppenc\n");
        return ;
    }

    m_pRtmpServer = make_shared<ModuleRtmpServer>(m_sPushPath.c_str(), 8888);
    m_pRtmpServer->setProductor(m_pMppEnc);
    m_pRtmpServer->setBufferCount(0);
    m_pRtmpServer->init();
    if (ret < 0) {
        ff_error("Failed to init rtsp server\n");
        return ;
    }

    m_pMemReader->start();
}

void StreamManager::HG_StopSink(const char* playId) {
    if (m_pMemReader != nullptr) {
        m_pMemReader->setProcessStatus(ModuleMemReader::DATA_PROCESS_STATUS::PROCESS_STATUS_EXIT);
        m_pMemReader->stop();
        m_pMemReader = nullptr;
    }
}

float StreamManager::HG_GetVersion() {
    return 1.01;
}