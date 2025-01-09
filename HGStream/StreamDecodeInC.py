from ctypes import *
import ctypes
import os
import numpy as np
import threading
import time

#########################################################################
class stPlay(Structure):
    _fields_ = [('type', c_int), ('eTranType', c_int), ('eAudioType', c_int), ('url', c_char_p), ('szUser', c_char_p),
                ('szPassward', c_char_p), ('szFileName', c_char_p), ('wndPlay', c_void_p)]


class stFrame(Structure):
    _fields_ = [('col', c_int), ('row', c_int), ('fmt', c_int), ('size', c_uint), ('pChar', c_char_p)]


#########################################################################

class StreamDecodeInC():
    _instance_lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        if not hasattr(StreamDecodeInC, "_instance"):
            with StreamDecodeInC._instance_lock:
                if not hasattr(StreamDecodeInC, "_instance"):
                    StreamDecodeInC._instance = object.__new__(cls)
        return StreamDecodeInC._instance
    
    def __init__(self):
        self.libMediaDec = None
        self.handle = None
        self.libpath_soft = os.path.dirname(os.path.abspath(__file__)) + '/lib/libHGDecoder.so'
        self.libpath_hard = os.path.dirname(os.path.abspath(__file__)) + '/playerlib/libmediaserver.so'
        # libtype 0 - hard, others - soft
        self.libtype = 0
        self.listpod_result = []
        self.maxList_pod_result = 60
        self.start_update_pod_result()
    
    # isOpened == 0: open success
    def start(self, url, libtype = 0):
        if self.handle is not None:
            return self.handle, 0
        self.libtype = libtype
        if self.libMediaDec is None:
            self.libMediaDec = cdll.LoadLibrary(self.libpath_hard if libtype == 0 else self.libpath_soft)
        if self.libMediaDec is None:
            self.log.info('libHGDecoder load failed: type = %s, path = %s', 
                          'soft' if libtype == 0 else 'hard',
                          self.libpath_soft if libtype == 0 else self.libpath_hard)
            return None, -1
        self.log.info('libHGDecoder load type %s', libtype)
        if libtype == 0: # hard
            self.libMediaDec.HG_GetRtspClient.argtype = (POINTER(ctypes.c_char_p))
            self.libMediaDec.HG_GetRtspClient.restype = POINTER(ctypes.c_void_p)
            self.handle = self.libMediaDec.HG_GetRtspClient(url.encode())   
            self.log.info("HG_GetRtspClient = %s", self.handle)

            # self.libMediaDec.HG_StartServer.argtypes = (ctypes.c_int, ctypes.c_int)
            self.libMediaDec.HG_StartServer()
            self.log.info("HG_StartServer")
            return self.handle, 0
        else:
            ret = self.libMediaDec.HG_InitializeDecoder()

            ##new handle
            self.libMediaDec.HG_NewPlayer.restype = POINTER(ctypes.c_void_p)
            self.handle = self.libMediaDec.HG_NewPlayer()
            self.log.info("HG_NewPlayer=", self.handle)
            self.log.info("-------------self.handle=", self.handle)

            isOpened = self._openVideo(url)
            return self.handle, isOpened

    def stop(self):
        if self.handle is None or self.libMediaDec is None:
            return
        if self.libtype == 0: # hard
            self.libMediaDec.HG_CloseClient.argtype = (POINTER(ctypes.c_void_p))
            self.libMediaDec.HG_CloseClient(self.handle)
            self.libMediaDec.HG_StopSever()
        else:
            self.libMediaDec.HG_StopPlay.argtype=(POINTER(ctypes.c_void_p))
            self.libMediaDec.HG_StopPlay(self.handle)
        self.handle = None
        
    ##########################################
    def _openVideo(self, url, user = '', password = ''):
        #start play
        play = stPlay()
        play.type = 0
        play.eTranType = 0
        play.eAudioType = 0
        play.url = url.encode()##"/home/pi/develop/angery-bird.mp4".encode()
        play.szUser = user.encode()
        play.szPassward = password.encode()
        play.szFileName = 0
        play.wndPlay = 0

        self.libMediaDec.HG_StartPlay.argtypes=(POINTER(ctypes.c_void_p), stPlay)
        ret = self.libMediaDec.HG_StartPlay(self.handle, play)
        self.log.info("-------------HG_StartPlay = ", ret)

        ##set output format
        self.libMediaDec.HG_SetOutputPixelFormat.argtypes=(POINTER(ctypes.c_void_p), c_int)
        self.libMediaDec.HG_SetOutputPixelFormat(self.handle, 3)
        self.log.info("-------------HG_SetOutputPixelFormat")

        return ret
    
    # read frame in C library
    def readframe(self, handle):
        if handle is None:
            return None, 0, time.time()
        self.libMediaDec.HG_ReadFrame.argtype = (POINTER(ctypes.c_void_p))
        self.libMediaDec.HG_ReadFrame.restype = POINTER(stFrame)
        frame = self.libMediaDec.HG_ReadFrame(handle)
        frame_time = time.time()
        if not frame or not frame.contents.pChar:
            return None, 0, frame_time

        byteCount = frame.contents.col * frame.contents.row * frame.contents.channels
        bitmapSize = (frame.contents.row, frame.contents.col, frame.contents.channels)

        dataPtr = cast(frame.contents.pChar, POINTER(c_void_p))
        picturePtr = cast(dataPtr, POINTER(c_uint8 * byteCount))

        picture = np.frombuffer(picturePtr.contents, dtype=np.ubyte, count=byteCount)
        picture = np.reshape(picture, bitmapSize)
        
        return picture, 1, frame_time
    ##########################################

    def putframe(self, frame):
        if self.libMediaDec is None:
            self.log.info('libMediaDec is None in putframe')
            return
        if self.libtype == 0:
            self.libMediaDec.HG_PutFrame.argtypes = (POINTER(ctypes.c_char_p), ctypes.c_int)
            byteCount = frame.shape[0] * frame.shape[1] * 3
            dataPtr = frame.ctypes.data_as(POINTER(ctypes.c_char_p))
            self.libMediaDec.HG_PutFrame(dataPtr, byteCount)
            # self.log.info('putframe %s %s', frame.shape, time.time())
        else:
            self.log.info('not implement')

streamDecodeInC = StreamDecodeInC()

if __name__ == '__main__':
    url = "rtsp://admin:@169.254.206.11:554"

    import time
    import cv2
    handle, ret = streamDecodeInC.start(url)#, [2880, 1616])
    frames, loopTime, initTime = 0, time.time(), time.time()
    spend_time = '30 frame average fps'
    try:
        while time.time() - initTime < 60:
            org_img, ret, timestamp = streamDecodeInC.readframe(handle)
            if ret == 0:
                time.sleep(0.001)
                continue
            frames += 1
            org_img = cv2.resize(org_img, (1920, 1080))
            frame = cv2.cvtColor(org_img, cv2.COLOR_BGR2RGB)	
            if frames >= 30:
                print("30帧平均帧率:\t", round(30 / (time.time() - loopTime), 2), "帧")
                spend_time = "30 frame average fps: {:.2f}".format(round(30 / (time.time() - loopTime), 2))
                loopTime = time.time()
                frames = 0
            cv2.putText(frame, spend_time, (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
            cv2.imshow('test', frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
            streamDecodeInC.putframe(frame)
        streamDecodeInC.stop()
    except Exception as e:
        print(e)
