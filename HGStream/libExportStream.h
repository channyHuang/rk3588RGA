
#ifndef LIBEXPORTSTREAM_H
#define LIBEXPORTSTREAM_H

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

//  图像数据结构
typedef struct stFrame {
    // 图像宽度 
    int col = 0;
    // 图像高度
    int row = 0;
    // 图像格式
    int fmt = 0;
    unsigned int size = 0;
    // 图像 
    unsigned char* pChar = nullptr;
} Frame;

typedef struct stFrameInfo
{
	int videoW;
	int videoH;
	int format;
}FrameInfo;

// ======================================
// 拉流初始化，输入uri, nRtpType = 0:udp, 1:tcp
D_EXTERN_C D_SHARE_EXPORT void* HG_GetRtspClient(const char* pUri, const int nRtpType = 0);
// 结束拉流
D_EXTERN_C D_SHARE_EXPORT void HG_CloseClient(void* pHandle);
// 获取拉流帧
D_EXTERN_C D_SHARE_EXPORT Frame* HG_ReadFrame(void* pHandle);
// 获取拉流帧信息
D_EXTERN_C D_SHARE_EXPORT FrameInfo* HG_GetFrameInfo(void* pHandle);

// ======================================
// 设置推流参数
// nEncodeType: 0-h264， 1-h265
D_EXTERN_C D_SHARE_EXPORT bool HG_SetFrameInfo(const char* pPlayId, const int nWidth = 1920, const int nHeight = 1080,
                                            const int nPort = 8888, const int nEncodeType = 0);
// 开始推流
D_EXTERN_C D_SHARE_EXPORT bool HG_StartServer();
// 结束推流
D_EXTERN_C D_SHARE_EXPORT void HG_StopSever();
// 推流帧
D_EXTERN_C D_SHARE_EXPORT bool HG_PutFrame(const char* playId, const unsigned char* buffer, const unsigned int size);

// ======================================
// 推流初始化
D_EXTERN_C D_SHARE_EXPORT void HG_CreateRtspSink(const char* pPlayId);
D_EXTERN_C D_SHARE_EXPORT void HG_CreateRtpSink(const char* pPeerURL);
D_EXTERN_C D_SHARE_EXPORT void HG_StopSink(const char* playId);

D_EXTERN_C D_SHARE_EXPORT float HG_GetVersion();

#endif // LIBEXPORTSTREAM_H