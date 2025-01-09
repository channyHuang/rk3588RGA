#include "YoloManager.h"

#include "./utils/image_utils.h"
#include <string.h>

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

static unsigned char *load_data(FILE *fp, size_t ofst, size_t sz)
{
    unsigned char *data;
    int ret;

    data = NULL;

    if (NULL == fp)
    {
        return NULL;
    }

    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0)
    {
        printf("blob seek failure.\n");
        return NULL;
    }

    data = (unsigned char *)malloc(sz);
    if (data == NULL)
    {
        printf("buffer malloc failure.\n");
        return NULL;
    }
    ret = fread(data, 1, sz, fp);
    return data;
}

static unsigned char *load_model(const char *filename, int *model_size) {
    FILE *fp;
    unsigned char *data;

    fp = fopen(filename, "rb");
    if (nullptr == fp) {
        printf("Open file %s failed.\n", filename);
        return nullptr;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    data = load_data(fp, 0, size);

    fclose(fp);

    *model_size = size;
    return data;
}

YoloManager::YoloManager(const std::string &sModelPath) {
    m_sModelPath = sModelPath;
}

YoloManager::~YoloManager() {
    if (app_ctx.input_attrs != NULL)
    {
        free(app_ctx.input_attrs);
        app_ctx.input_attrs = NULL;
    }
    if (app_ctx.output_attrs != NULL)
    {
        free(app_ctx.output_attrs);
        app_ctx.output_attrs = NULL;
    }
    if (app_ctx.rknn_ctx != 0)
    {
        rknn_destroy(app_ctx.rknn_ctx);
        app_ctx.rknn_ctx = 0;
    }
}

int YoloManager::init(rknn_context *ctx, bool bShareWeight) {
    // Load Rknn Model
    int nModelSize = 0;
    pModel = load_model(m_sModelPath.c_str(), &nModelSize);

    if (bShareWeight) {
        ret = rknn_dup_context(ctx, &app_ctx.rknn_ctx);
    } else {
        ret = rknn_init(&app_ctx.rknn_ctx, pModel, nModelSize, RKNN_FLAG_COLLECT_PERF_MASK, NULL);
    }
    free(pModel);
    if (ret < 0) {
        printf("rknn_init failed %d\n", ret);
        return -1;
    }

    // NPU多核配置
    rknn_core_mask core_mask = RKNN_NPU_CORE_0_1_2;
    ret = rknn_set_core_mask(app_ctx.rknn_ctx, core_mask);
    // 多batch推理
    rknn_set_batch_core_num(app_ctx.rknn_ctx, 3);
    // SDK 版本
    rknn_sdk_version version;
    ret = rknn_query(app_ctx.rknn_ctx, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
    if (ret != RKNN_SUCC) {
        printf("rknn_init error ret=%d\n", ret);
        return -1;
    }
    printf("sdk version: %s driver version: %s\n", version.api_version, version.drv_version);
    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(app_ctx.rknn_ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret = %d\n", ret);
        return -1;
    }
    printf("Model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(app_ctx.rknn_ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(app_ctx.rknn_ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC && output_attrs[0].type != RKNN_TENSOR_FLOAT16)
    {
        app_ctx.is_quant = true;
    }
    else
    {
        app_ctx.is_quant = false;
    }
    printf("rknn_isquant %d\n", app_ctx.is_quant);

    app_ctx.io_num = io_num;
    app_ctx.input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx.input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx.output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx.output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        printf("model is NCHW input fmt\n");
        app_ctx.model_channel = input_attrs[0].dims[1];
        app_ctx.model_height = input_attrs[0].dims[2];
        app_ctx.model_width = input_attrs[0].dims[3];
    }
    else
    {
        printf("model is NHWC input fmt\n");
        app_ctx.model_height = input_attrs[0].dims[1];
        app_ctx.model_width = input_attrs[0].dims[2];
        app_ctx.model_channel = input_attrs[0].dims[3];
    }
    m_nModelWidth = app_ctx.model_width;
    m_nModelHeight = app_ctx.model_height;
    m_nModelChannel = app_ctx.model_channel;
    printf("model input height=%d, width=%d, channel=%d\n",
           app_ctx.model_height, app_ctx.model_width, app_ctx.model_channel);

    // Set Input Data
    memset(m_rknnInputs, 0, sizeof(m_rknnInputs));
    m_rknnInputs[0].index = 0;
    m_rknnInputs[0].type = RKNN_TENSOR_UINT8;
    m_rknnInputs[0].size = m_nModelWidth * m_nModelHeight * m_nModelChannel;
    m_rknnInputs[0].fmt = RKNN_TENSOR_NHWC;
    m_rknnInputs[0].pass_through = 0;

    memset(&src_img, 0, sizeof(image_buffer_t));
    return 0;
}

void YoloManager::deinit(bool bShareWeight) {
    if (app_ctx.input_attrs != NULL)
    {
        free(app_ctx.input_attrs);
        app_ctx.input_attrs = NULL;
    }
    if (app_ctx.output_attrs != NULL)
    {
        free(app_ctx.output_attrs);
        app_ctx.output_attrs = NULL;
    }
    if (bShareWeight) return;
    if (app_ctx.rknn_ctx != 0)
    {
        rknn_destroy(app_ctx.rknn_ctx);
        app_ctx.rknn_ctx = 0;
    }
    return;
}

rknn_context *YoloManager::get_ctx() {
    return &app_ctx.rknn_ctx;
}

StImgWithId YoloManager::infer(StImgWithId &stImgWithId) {
    StImgWithId res_stImgWithId;
    res_stImgWithId.id = stImgWithId.id;
    res_stImgWithId.bRotate = stImgWithId.bRotate;
    res_stImgWithId.nAddWidth = stImgWithId.nAddWidth;
    res_stImgWithId.nAddHeight = stImgWithId.nAddHeight;
    res_stImgWithId.pChar = stImgWithId.pChar;

    // 输入数据
    m_nImgWidth = stImgWithId.nWidth;
    m_nImgHeight = stImgWithId.nHeigth;
    if (m_nImgHeight <= 0 || m_nImgWidth <= 0 || stImgWithId.pChar == nullptr) {
        printf("Error in infer: width = %d or height = %d < 0\n", m_nImgWidth, m_nImgHeight);
        return res_stImgWithId;
    }

    src_img.width = m_nImgWidth;
    src_img.height = m_nImgHeight;
    src_img.size = m_nImgWidth * m_nImgHeight * 3;
    src_img.format = IMAGE_FORMAT_RGB888;
    if (src_img.virt_addr == NULL) {
        printf("malloc src_img %d\n", src_img.size);
        src_img.virt_addr = (unsigned char*)malloc(src_img.size * sizeof(unsigned char));
    } 
    if (src_img.virt_addr == NULL) {
        printf("src_img buf is NULL, ret = %d\n", ret);
        return res_stImgWithId;
    }

    memcpy(src_img.virt_addr, stImgWithId.pChar, src_img.size);
    
    // Pre Process
    int bg_color = 114;
    if (dst_img.virt_addr == NULL) {
        memset(&dst_img, 0, sizeof(image_buffer_t));
        dst_img.width = app_ctx.model_width;
        dst_img.height = app_ctx.model_height;
        dst_img.format = IMAGE_FORMAT_RGB888;
        dst_img.size = get_image_size(&dst_img);
        // dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
        set_image_dma_buf_alloc(&dst_img);
    } else if (get_image_size(&dst_img) != dst_img.size) {
        image_dma_buf_free(&dst_img);
        dst_img.virt_addr = NULL;

        memset(&dst_img, 0, sizeof(image_buffer_t));
        dst_img.width = app_ctx.model_width;
        dst_img.height = app_ctx.model_height;
        dst_img.format = IMAGE_FORMAT_RGB888;
        dst_img.size = get_image_size(&dst_img);

        set_image_dma_buf_alloc(&dst_img);
    }
    if (dst_img.virt_addr == NULL)
    {
        printf("malloc buffer size:%d fail!\n", dst_img.size);
        return res_stImgWithId;
    }

    letterbox_t letter_box;
    memset(&letter_box, 0, sizeof(letterbox_t));
    // letterbox
    ret = convert_image_with_letterbox(&src_img, &dst_img, &letter_box, bg_color);
    if (ret < 0)
    {
        printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        return res_stImgWithId;
    }
    m_rknnInputs[0].buf = dst_img.virt_addr;
    
    ret = rknn_inputs_set(app_ctx.rknn_ctx, app_ctx.io_num.n_input, m_rknnInputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return res_stImgWithId;
    }

    // 模型推理
    ret = rknn_run(app_ctx.rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return res_stImgWithId;
    }

    // 获取输出
    rknn_output outputs[app_ctx.io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < app_ctx.io_num.n_output; i++)
    {
        outputs[i].index = i;
        outputs[i].want_float = (!app_ctx.is_quant);
    }
    ret = rknn_outputs_get(app_ctx.rknn_ctx, app_ctx.io_num.n_output, outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        return res_stImgWithId;
    }

    // 后处理
    post_process(&app_ctx, outputs, &letter_box, BOX_THRESH, BOX_THRESH_LIST, NMS_THRESH, &res_stImgWithId.od_results);
    // Remeber to release rknn output
    ret = rknn_outputs_release(app_ctx.rknn_ctx, app_ctx.io_num.n_output, outputs);

out:
    if (dst_img.virt_addr != NULL)
    {
        // free(dst_img->virt_addr);
    }

    return res_stImgWithId;
}