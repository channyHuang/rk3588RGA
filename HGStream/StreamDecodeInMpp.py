import cv2
import ff_pymedia
import os
from queue import Queue
import re
import threading
import time
import datetime

# 向上取16的倍数，因rga部分操作如旋转只支持16的倍数
def align(x, a):
    return (x + a - 1) & ~(a - 1)

def find_two_numbers(n, x, y):
    a = 1
    b = n
    min_diff = 8192
    result = (0, 0)
    while a <= b:
        if n % a == 0:
            b = n // a
            diff1 = abs(a - x) + abs(b - y)
            diff2 = abs(a - y) + abs(b - x)
            if diff1 < min_diff or diff2 < min_diff:
                if diff1 < diff2:
                    result = (a, b)
                else:
                    result = (b, a)
                min_diff = min(diff1, diff2)
        a += 1
    return result

# 回调类，给回调函数传外部参数
class StCallBack():
    def __init__(self, name, module, count, mpp_queue = Queue(maxsize = 2)):
        self.name = name
        self.module = module
        self.count = count
        self.mpp_queue = mpp_queue

# 读取视频流数据线程，返回[frame, time_stamp]
class StreamDecodeInMpp():
    _instance_lock = threading.Lock()
    def __new__(cls, *args, **kwargs):
        if not hasattr(StreamDecodeInMpp, "_instance"):
            with StreamDecodeInMpp._instance_lock:
                if not hasattr(StreamDecodeInMpp, "_instance"):
                    StreamDecodeInMpp._instance = object.__new__(cls)
        return StreamDecodeInMpp._instance

    def __init__(self):
        self.handle = None
    
    # 开始拉流，output为期望得到的视频帧大小
    def start(self, url, output = '1920x1080'):
        print('StreamDecodeInMpp start ', url)
        self.url = url
        self.mpp_queue = Queue(maxsize = 2)
        self.handle = StreamDecodeInMpp.startStream(self.url, '1920x1080', self.mpp_queue)
        if self.handle is None:
            print('StreamDecodeInMpp startStream failed')
            return

    # 停止拉流
    def stop(self):
        print('StreamDecodeInMpp stop')
        self.handle.dumpPipeSummary()
        self.handle.stop()

    # 拉取每一帧，返回[frame帧数据None为失败, ret = 0失败/1成功, time.time()时间戳]
    def readframe(self):
        frame_time = time.time()
        if self.mpp_queue.empty():
            return None, 0, frame_time
        picture = self.mpp_queue.get()
        return picture, 1, frame_time

    # 推流，Python未有接口，等待C++封装
    def putframe(self, frame):
        print('Not implement yeah')

    # 静态方法，内部调用ffmedia使用ffmpeg+mpp
    @staticmethod
    def startStream(uri, output = '1280x768', mpp_queue = Queue(maxsize = 2)):
        # RTSP流， input_source即为原拉流库对应的handle
        input_source = None
        if uri.startswith("rtsp://"):
            input_source = ff_pymedia.ModuleRtspClient(uri, ff_pymedia.RTSP_STREAM_TYPE(0), True, False)
        else:
            is_stat = os.stat(uri)
            if stat.S_ISCHR(is_stat.st_mode):
                print("input source is a camera device.")
                input_source = ff_pymedia.ModuleCam(uri)
            elif stat.S_ISREG(is_stat.st_mode):
                print("input source is a regular file.")
                input_source = ff_pymedia.ModuleFileReader(uri, False);
            else:
                print("{} is not support.".format(uri))
                return None
        ret = input_source.init()
        if ret < 0:
            print('init failed')
            return None
        # MPP
        last_module = input_source
        input_para = last_module.getOutputImagePara()
        if input_para.v4l2Fmt == ff_pymedia.v4l2GetFmtByName("H264") or \
            input_para.v4l2Fmt == ff_pymedia.v4l2GetFmtByName("MJPEG")or \
            input_para.v4l2Fmt == ff_pymedia.v4l2GetFmtByName("H265"):
            dec = ff_pymedia.ModuleMppDec()
            dec.setProductor(last_module)
            ret = dec.init()
            if ret < 0:
                print("dec init failed")
                return 1
            last_module = dec
        # RGA
        outputfmt = 'BGR24'
        input_para = last_module.getOutputImagePara()
        output_para=ff_pymedia.ImagePara(input_para.width, input_para.height, input_para.hstride, input_para.vstride, ff_pymedia.v4l2GetFmtByName(outputfmt))
        match = re.match(r"(\d+)x(\d+)", output)
        if match:
            width, height = map(int, match.groups())
            output_para.width = width
            output_para.height = height
        if input_para.height != output_para.height or \
            input_para.height != output_para.height or \
            input_para.width != output_para.width or \
            input_para.v4l2Fmt != output_para.v4l2Fmt:
            rotate = ff_pymedia.RgaRotate(0)

            output_para.hstride = align(output_para.width, 16)
            output_para.vstride = align(output_para.height, 16)
            
            rga = ff_pymedia.ModuleRga(output_para, rotate)
            rga.setProductor(last_module)
            rga.setBufferCount(2)
            ret = rga.init()
            if ret < 0:
                print("rga init failed")
                return 1
            last_module = rga
        # 回调获取输出帧数据
        stCallback = StCallBack("StCallBack", None, 1, mpp_queue)
        stCallback.module = last_module.addExternalConsumer("StCallBack", stCallback, cv2_extcall_back)
        # 开启线程
        input_source.start()
        input_source.dumpPipe()
        # 返回handle供结束时关闭线程调用
        return input_source

# 回调函数
def cv2_extcall_back(obj, MediaBuffer):
    vb = ff_pymedia.VideoBuffer.from_base(MediaBuffer)
    data = vb.getActiveData()
    #flush dma buf to cpu
    vb.invalidateDrmBuf();

    try:
        img = data.reshape((vb.getImagePara().vstride, vb.getImagePara().hstride, 3))
    except ValueError:
        print("Invalid image resolution!")
        resolution = find_two_numbers(data.size//3, vb.getImagePara().hstride, vb.getImagePara().vstride)
        print("Try the recommended resolution: -o {}x{}".format(resolution[0], resolution[1]))
        exit(-1)

    for i in range(obj.count):
        if obj.mpp_queue.full():
            _ = obj.mpp_queue.get()
        obj.mpp_queue.put(img)
        # cv2.imshow('origin', img)
        # cv2.waitKey(1)

streamDecodeInMpp = StreamDecodeInMpp()

if __name__ == "__main__":
    # 开启
    streamDecodeInMpp.start('rtsp://admin:@192.168.1.12:8888/live/0')
    spend_time = "30 frame average fps: "
    loopTime = time.time()
    framenum = 0
    while time.time() - loopTime < 120:
        # 读帧
        data = streamDecodeInMpp.readframe()
        frame = data[0]
        if frame is None:
            time.sleep(0.02)
            continue
        timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        framenum += 1
        if framenum % 30 == 0:
            spend_time = "30 frame average fps: {:.2f}".format(round(framenum / (time.time() - loopTime), 2))
            if framenum >= 3000000:
                framenum = 0
                loopTime = time.time()
        cv2.putText(frame, spend_time, (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)
        cv2.putText(frame, timestamp, (30, 90), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)
        frame = cv2.resize(frame, (648, 480))
        cv2.imshow('frame', frame)
        cv2.waitKey(1)
    streamDecodeInMpp.stop()
