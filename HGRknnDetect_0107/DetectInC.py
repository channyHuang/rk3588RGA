# -*- coding:utf-8 -*-

import threading
import ctypes
import cv2
import numpy as np
import time
import os

# 目标检测返回数据结构
class stDetectResult(ctypes.Structure):
    _fields_ = [('pFrame', ctypes.c_void_p), 
                ('nDetectNum', ctypes.c_int), 
                ('nWidth', ctypes.c_int), 
                ('nHeight', ctypes.c_int), 
                ('pClasses', ctypes.POINTER(ctypes.c_int)), 
                ('pBoxes', ctypes.POINTER(ctypes.c_int)), 
                ('pProb', ctypes.POINTER(ctypes.c_float))]

# Python调用C库目标检测
class DetectInC():
    # 单例
    _instance_lock = threading.Lock()
    def __new__(cls, *args, **kwargs):
        if not hasattr(DetectInC, "_instance"):
            with DetectInC._instance_lock:
                if not hasattr(DetectInC, "_instance"):
                    DetectInC._instance = object.__new__(cls)
        return DetectInC._instance
    
    # 检测库，PC上交叉编译后push到板上
    def __init__(self) -> None:
        self.libpath = os.path.dirname(os.path.abspath(__file__)) + '/HGRknnDetect/libHGRknnDetect_v8.so'
        self.label_path = os.path.dirname(os.path.abspath(__file__)) + '/HGRknnDetect/model/'
        self.anchor_name = os.path.dirname(os.path.abspath(__file__)) + '/model/RK_anchors_0802.txt'
        self.lib = None

    # 加载检测库，调用初始化接口加载rknn模型
    def init(self, model):
        if self.lib is None:
            self.lib = ctypes.cdll.LoadLibrary(self.libpath)
        if self.lib is None:
            return False
        
        # 设置Label文件路径
        self.lib.setRootDirectory.argtype = (ctypes.POINTER(ctypes.c_char_p))
        self.lib.setRootDirectory(self.label_path.encode())
        # 设置输出branch数，默认为3
        self.lib.setBranchNum.argtype = (ctypes.c_int)
        self.lib.setBranchNum(3)
        # 设置类别数
        self.lib.setClassNum.argtype = (ctypes.c_int)
        self.lib.setClassNum(8)
        # 设置阈值
        self.lib.setThreshold.argtype = (ctypes.c_float)
        self.lib.setThreshold(ctypes.c_float(0.1))
        # 设置阈值列表
        self.lib.setThresholdList.argtype = (ctypes.POINTER(ctypes.c_float * 8), ctypes.c_int)
        threshold_list = (ctypes.c_float * 8)(0.5, 0.5, 0.55, 0.5, 0.15, 0.5, 0.2, 0.15)
        self.lib.setThresholdList(threshold_list, 8)
        # 设置anchor文件路径和名称，仅yolov5有效
        self.lib.setAnchorFile.argtype = (ctypes.c_char_p)
        self.lib.setAnchorFile(self.anchor_name.encode())
        # 初始化
        self.lib.init.argtype = (ctypes.POINTER(ctypes.c_char_p))
        self.lib.init.restype = ctypes.c_bool
        res = self.lib.init(model.encode())
        if res == False:
            print('init failed', res)
        # 设置检测接口参数类型
        self.lib.detect.argtype = (ctypes.POINTER(ctypes.c_char_p), ctypes.c_int, ctypes.c_int)
        self.lib.detect.restype = ctypes.POINTER(stDetectResult)

        self.lib.deinit.restype = ctypes.c_bool
        return res            

    def deinit(self):
        if self.lib is None:
            return False
        self.lib.deinit()

    # 单帧检测，返回[结果图，目标类别class，框box，置信度prob]
    def detect(self, frame):
        if self.lib is None:
            return None, None, None, None
        
        dataPtr = frame.ctypes.data_as(ctypes.POINTER(ctypes.c_char_p))
        detectResult = self.lib.detect(dataPtr, frame.shape[1], frame.shape[0])
        if not detectResult or detectResult.contents is None:
            print('Nothing')
            return None, None, None, None
        else:
            # 解析检测结果
            num = detectResult.contents.nDetectNum
            classes = []
            boxes = []
            prob = []
            if num > 0:
                # 目标类型
                classesPtr = ctypes.cast(detectResult.contents.pClasses, ctypes.POINTER(ctypes.c_int * num))
                classes = np.frombuffer(classesPtr.contents, dtype=np.int32, count = num)
                # 目标框
                boxesPtr = ctypes.cast(detectResult.contents.pBoxes, ctypes.POINTER(ctypes.c_int * num * 4))
                boxes_raw = np.frombuffer(boxesPtr.contents, dtype=np.int32, count = num * 4)
                boxes = []
                box = []
                for i, b in enumerate(boxes_raw):
                    box.append(b)
                    if i % 4 == 3:
                        boxes.append(box)
                        box = []
                # 目标置信度
                probPtr = ctypes.cast(detectResult.contents.pProb, ctypes.POINTER(ctypes.c_float * num))
                prob = np.frombuffer(probPtr.contents, dtype = np.float32, count = num)
            # 带框图像
            nWidth = detectResult.contents.nWidth
            nHeight = detectResult.contents.nHeight
            byteCount = nWidth * nHeight * 3
            imagePtr = ctypes.cast(detectResult.contents.pFrame, ctypes.POINTER(ctypes.c_uint8 * byteCount))
            picture = np.frombuffer(imagePtr.contents, dtype=np.ubyte, count=byteCount)
            picture = np.reshape(picture, (nHeight, nWidth, 3))
            return picture, classes, boxes, prob

    def printProfile(self):
        if self.lib is None:
            return
        self.lib.printProfile()

detectInC = DetectInC()

def test_single_image():
    detectInC.init('./model/yolov5n.rknn')
    sttime = time.time()
    image = cv2.imread('20240602_171238_806.jpg')
    res_image, classes, boxes, probs = detectInC.detect(image)
    print('spend time:', time.time() - sttime)
    cv2.imshow('res', res_image)
    cv2.waitKeyEx(5000)
    cv2.destroyAllWindows()

def test_image_folder():
    model_path = './model/yolov5n.rknn'
    image_folder = '/home/images/'

    detectInC.init(model_path)

    loopTime = time.time()
    framenum = 0
    spend_time = "30 frame average fps: "
    img_list = os.listdir(image_folder)
    for image_name in img_list:
        image = cv2.imread(image_folder + image_name)
        res_image, classes, boxes, probs = detectInC.detect(image)
        framenum += 1
        if res_image is not None:
            if framenum >= 30:
                spend_time = "30 frame average fps: {:.2f}".format(round(30 / (time.time() - loopTime), 2))
                loopTime = time.time()
                framenum = 0
            cv2.putText(res_image, spend_time, (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)
            cv2.imshow('res', res_image)
            cv2.waitKey(1)
    detectInC.printProfile()
    detectInC.deinit()
    cv2.destroyAllWindows()

def test_video():
    model_path = './model/yolov5n.rknn'
    uri = "rtsp://admin:@169.254.206.11"

    detectInC.init(model_path)

    cap = cv2.VideoCapture(uri)
    loopTime = time.time()
    framenum = 0
    spend_time = "30 frame average fps: "
    while cap.isOpened():
        ret, image = cap.read()
        image = cv2.resize(image, (1920, 1080))
        if ret == False:
            break
        res_image, classes, boxes, probs = detectInC.detect(image)
        framenum += 1
        if res_image is not None:
            if framenum >= 30:
                spend_time = "30 frame average fps: {:.2f}".format(round(30 / (time.time() - loopTime), 2))
                loopTime = time.time()
                framenum = 0
            cv2.putText(res_image, spend_time, (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)
            cv2.imshow('res', res_image)
            cv2.waitKey(1)
    detectInC.printProfile()
    detectInC.deinit()
    cv2.destroyAllWindows()

if __name__ == '__main__':
    # test_single_image()    
    # test_image_folder()
    test_video()