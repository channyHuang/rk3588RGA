#ifndef _RKNN_YOLOV8_DEMO_POSTPROCESS_H_
#define _RKNN_YOLOV8_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include <iostream>

#include "../3rdparty/rknpu2/include/rknn_api.h"
#include "../utils/common.h"
#include "../utils/image_utils.h"

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 128
// #define OBJ_CLASS_NUM 80
#define NMS_THRESH 0.45
// #define BOX_THRESH 0.63
extern int OBJ_CLASS_NUM;
extern float BOX_THRESH;
extern float BOX_THRESH_LIST[13];
// class rknn_app_context_t;
extern int g_nOutputBranchNum;
extern std::string g_sRootDirectory;
extern std::string g_sLabelFileName;

typedef struct {
    image_rect_t box;
    float prop;
    int cls_id;
} object_detect_result;

typedef struct {
    int id;
    int count;
    object_detect_result results[OBJ_NUMB_MAX_SIZE];
} object_detect_result_list;

int init_post_process();
void deinit_post_process();
char *coco_cls_to_name(int cls_id);
int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, const float* conf_threshold_list, float nms_threshold, object_detect_result_list *od_results);

void deinitPostProcess();
#endif //_RKNN_YOLOV8_DEMO_POSTPROCESS_H_
