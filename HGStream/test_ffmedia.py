import cv2
import ff_pymedia
import os
import re
import time

framenum = 0
spend_time = "30 frame average fps: "
loopTime = time.time()

class Cv2Display():
    def __init__(self, name, module, count):
        self.name = name
        self.module = module
        self.count = count

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

def cv2_extcall_back(obj, MediaBuffer):
    global framenum
    global spend_time
    global loopTime

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
        framenum += 1
        if framenum % 30 == 0:
            spend_time = "30 frame average fps: {:.2f}".format(round(framenum / (time.time() - loopTime), 2))
            if framenum >= 3000000:
                framenum = 0
                loopTime = time.time()
        cv2.putText(img, spend_time, (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)
        cv2.imshow(obj.name + str(i), img)
    cv2.waitKey(1)

def stream_encode_decode(uri, output = '1280x768', encodetype = 1, port = 8888, outputfmt = 'BGR24'):
# def stream_encode_decode(uri, output = '1280x768', encodetype = None, outputfmt = 'BGR24'):
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
            return
    last_module = input_source
    ret = input_source.init()
    if ret < 0:
        print('init failed')
        return
        
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

    cv_display = Cv2Display("Cv2Display", None, 1)
    cv_display.module = last_module.addExternalConsumer("Cv2Display", cv_display, cv2_extcall_back)

    if encodetype is not None:
      enc = ff_pymedia.ModuleMppEnc(ff_pymedia.EncodeType(encodetype))
      enc.setProductor(last_module)
      enc.setBufferCount(8)
      enc.setDuration(0) #Use the input source timestamp
      ret = enc.init()
      if ret < 0:
          print("ModuleMppEnc init failed")
          return 1
      last_module = enc

      if port != 0:
          push_s = None
          push_s = ff_pymedia.ModuleRtspServer("/live/0", port)
          push_s.setProductor(last_module)
          push_s.setBufferCount(0)
          
          ret = push_s.init()
          if ret < 0:
              print("push server init failed")
              return 

    input_source.start()
    input_source.dumpPipe()
    text = input("wait...")
    input_source.dumpPipeSummary()
    input_source.stop()

if __name__ == "__main__":
    stream_encode_decode(uri = 'rtsp://admin:@192.168.1.155:554')