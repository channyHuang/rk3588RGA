# HGRknnDetect
Yolov5/Yolov8目标检测库。支持v5和v8。支持大图像(如1920x1080)分片成小图像(如640x640)开多线程检测再合并成大图像输出。

## 接口：
```c++
// 初始化，输入权重模型路径名称，加载模型
bool init(const char* pModelString);
// 分片初始化
bool init_crop(const char* pModelString);
// 反初始化，释放模型 
bool deinit();
bool deinit_crop();
// 检测，输入图像数据及图像宽高，输出检测信息
stDetectResult* detect(char* pChar, int nWidth, int nHeight);
// 分片检测
stDetectResult* detect_crop(char* pChar, int src_width, int src_height);
// 打印性能统计信息到控制台
void printProfile();

// 设置目标检测置信度阈值，即时生效 
void setThreshold(float fThreshold);
// 设置目标检测置信度阈值，每个类别使用不同的置信度，即时生效，上述单置信度为基础过滤
void setThresholdList(float* fThresholdList, int nThresholdNum = 3);
// 设置模型对应的目标总类型数 
void setClassNum(int nClassNum);
// 设置输出branch数
void setBranchNum(int nBranchNum = 3);
// 设置根目录
void setRootDirectory(const char* pChar);
// 设置Anchor文件路径和名称，仅yolov5有效
void setAnchorFile(const char* pChar);
// 设置分片长宽
void setCropSize(int nWidth, int nHeight);
```

## 编译设置
见`CMakeLists.txt`。
```sh
# 设置True为Yolov5，False为Yolov8
set(v5 False)
# 设置True为生成so库，False为生成可执行文件，可执行文件依赖OpenCV
set(Lib True)
```

## c++调用样例 
见`main.cpp`。
```c++
    bool useOld = false;
    // 初始化加载rknn模型，旧方法使用单线程多核，新方法使用多线程多核　
    if (useOld) {
        init("yolov8.rknn");
    } else {
        init_crop("yolov8.rknn");
    }
    // 设置总检测种类数
    setClassNum(8);
    // 设置最低检测阈值
    setThreshold(0.1);
    // 对每个类别单独设置阈值
    float list[13] = {0.5, 0.5, 0.55, 0.5, 0.15, 0.5, 0.2, 0.15};
    setThresholdList(list, 8);
    // 读取视频开始检测
    cv::Mat img = cv::imread("./input.jpg");
    cv::VideoCapture capture;
    capture.open("rgb_test_video.mp4");
    printf("==================== start read frame\n");

    struct timeval time;
    gettimeofday(&time, nullptr);
    auto startTime = time.tv_sec * 1000 + time.tv_usec / 1000;
    auto beforeTime = startTime;
    int frames = 1;
    cv::Mat final_img = cv::Mat::zeros(1080, 1920, CV_8UC3);
    stDetectResult *stResult;
    while (capture.isOpened())
    {
        if (capture.read(img) == false)
            break;
        if (img.cols != 1920 || img.rows != 1080) continue;
        // 目标检测
        if (useOld)
            // 旧方法整张图直接检测，如果图像比模型输入尺寸大，会按比例缩小，缩小不足的会填充。但如果图像比模型输入𢙪小会报错；
            stResult = detect((char*)img.data, img.cols, img.rows);
        else 
            // 新方法把整张图像分片成小图像再检测，把结果再缩放回原图像画框最后输出
            stResult = detect_crop((char*)img.data, img.cols, img.rows);

        memcpy(final_img.data, stResult->pFrame, img.cols * img.rows * 3);
        cv::imshow("res", final_img);
        cv::waitKey(1);
        // 计算帧率
        if (frames % 30 == 0)
        {
            gettimeofday(&time, nullptr);
            auto currentTime = time.tv_sec * 1000 + time.tv_usec / 1000;
            printf("30 帧内平均帧率:\t %f fps/s\n", 30.0 / float(currentTime - beforeTime) * 1000.0);
            beforeTime = currentTime;
        }
        if (frames > 3000000) {
            frames = 1;
        }
        frames++;
    }
    capture.release();
    if (useOld) deinit();
    else deinit_crop();
```

## Python调用样例
见`DetectInC.py`。
```py
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
```
