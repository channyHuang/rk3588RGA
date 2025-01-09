#include <iostream>

#include "image_rga.h"
#include "im2d.hpp"
#include "RgaUtils.h"

#include <math.h>
#include <memory>
#include <string.h>

// return [(height, width)]
std::vector<std::pair<int, int>> calculateOffset(int src_width, int src_height, int crop_width, int crop_height) {
    std::vector<std::pair<int, int>> offset;
    int height_num = std::ceil(src_width * 1.0f / crop_width);
    int width_num = std::ceil(src_height * 1.0f / crop_height);
    int width_pad = 0, height_pad = 0;
    if (height_num > 1) {
        height_pad = (height_num * crop_height - src_height) / (height_num - 1);
    }
    if (width_num > 1) {
        width_pad = (width_num * crop_width - src_width) / (width_num - 1);
    }
    int height_offset = 0, width_offset = 0;
    int edge_flag[2] = {false, false};
    while (height_offset < src_height) {
        edge_flag[1] = false;
        if (height_offset + crop_height >= src_height) {
            height_offset = src_height - crop_height;
            edge_flag[0] = true;
        } 
        while (width_offset < src_width) {
            if (width_offset + crop_width > src_width) {
                width_offset = src_width - crop_width;
                edge_flag[1] = true;
            }
            offset.push_back(std::make_pair(height_offset, width_offset));
            if (edge_flag[1]) break;
            width_offset += crop_width - width_pad;
        }
        if (edge_flag[0]) break;
        height_offset += crop_height - height_pad;
        width_offset = 0;
    }
    return offset;
}

rga_buffer_t rga_image(char* pChar, int nWidth, int nHeight) {
    rga_buffer_t img;
    memset(&img, 0, sizeof(img));
    int format = RK_FORMAT_RGB_888;
    int buf_size = nWidth * nHeight * 3;
    char* buf = (char *)malloc(buf_size);
    memcpy(buf, pChar, buf_size);
    rga_buffer_handle_t handle = importbuffer_virtualaddr(buf, buf_size);
    if (handle == 0) {
        printf("importbuffer failed!\n");
        return img;
    }
    img = wrapbuffer_handle(handle, nWidth, nHeight, format);

    // if (handle)
    //     releasebuffer_handle(handle);

    return img;
}

void release(rga_buffer_t& img, char* buf) {
    if (img.handle) {
        releasebuffer_handle(img.handle);
    }
    if (buf) {
        free(buf);
    }
}

char* rga_image_buf(char* pChar, int nWidth, int nHeight) {
    rga_buffer_t img;
    memset(&img, 0, sizeof(img));
    int format = RK_FORMAT_RGB_888;
    int buf_size = nWidth * nHeight * 3;
    char* buf = (char *)malloc(buf_size);
    memcpy(buf, pChar, buf_size);
    rga_buffer_handle_t handle = importbuffer_virtualaddr(buf, buf_size);
    if (handle == 0) {
        printf("importbuffer failed!\n");
        return buf;
    }
    img = wrapbuffer_handle(handle, nWidth, nHeight, format);

    // if (handle)
    //     releasebuffer_handle(handle);

    return buf;
}

rga_buffer_t rga_crop(rga_buffer_t &src_img, int dst_width, int dst_height, int nOffsetWidth, int nOffsetHeight) {
    int ret = 0;
    rga_buffer_t dst_img;
    memset(&dst_img, 0, sizeof(dst_img));

    im_rect src_rect, dst_rect;
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));
    src_rect.x = nOffsetWidth;
    src_rect.y = nOffsetHeight;
    src_rect.width = dst_width;
    src_rect.height = dst_height;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = dst_width;
    dst_rect.height = dst_height;

    int dst_format = RK_FORMAT_RGB_888;
    int dst_buf_size = dst_width * dst_height * 3;
    char *dst_buf = (char *)malloc(dst_buf_size);
    memset(dst_buf, 0x80, dst_buf_size);
    rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
     if (dst_handle == 0) {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);

    ret = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    ret = improcess(src_img, dst_img, {}, src_rect, dst_rect, {}, IM_SYNC);
    if (ret == IM_STATUS_SUCCESS) {
        // printf("%s running success!\n", LOG_TAG);
    } else {
        // printf("%s running failed, %s\n", LOG_TAG, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

release_buffer:
//     if (dst_handle)
//         releasebuffer_handle(dst_handle);

    // cv::Mat final_img = cv::Mat::zeros(dst_height, dst_width, CV_8UC3);
    // memcpy(final_img.data, dst_buf, dst_buf_size);
    // cv::imwrite("rga_crop.jpg", final_img);

    return dst_img;
}

rga_buffer_t rga_resize(rga_buffer_t &src_img, int dst_width, int dst_height) {
    int ret = 0;
    im_rect src_rect, dst_rect;
    rga_buffer_t dst_img;
    
    int dst_format = RK_FORMAT_RGB_888;
    int dst_buf_size = dst_width * dst_height * 3;
    char *dst_buf = (char *)malloc(dst_buf_size);
    memset(dst_buf, 0x80, dst_buf_size);
    rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
    if (dst_handle == 0) {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);

    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));
    ret = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    ret = imresize(src_img, dst_img);
    if (ret == IM_STATUS_SUCCESS) {
        // printf("%s running success!\n", LOG_TAG);
    } else {
        // printf("%s running failed, %s\n", LOG_TAG, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

release_buffer:
    // if (dst_handle)
    //     releasebuffer_handle(dst_handle);

    // cv::Mat final_img = cv::Mat::zeros(dst_height, dst_width, CV_8UC3);
    // memcpy(final_img.data, dst_buf, dst_buf_size);
    // cv::imwrite("rga_resize.jpg", final_img);

    return dst_img;
}

char* rga_crop_resize(rga_buffer_t& src_img, int dst_width, int dst_height, int offset_width, int offset_height, int crop_width, int crop_height) {
    int ret = 0;
    im_rect src_rect, crop_rect, dst_rect;
    rga_buffer_t crop_img, dst_img, rot_img;
    
    int format = RK_FORMAT_RGB_888;
    // image buf allocate 
    int crop_buf_size = crop_width * crop_height * 3;
    int dst_buf_size = dst_width * dst_height * 3;
    char *crop_buf = (char *)malloc(crop_buf_size);
    char *rot_buf = (char *)malloc(crop_buf_size);
    char *dst_buf = (char *)malloc(dst_buf_size);
    memset(crop_buf, 0x80, crop_buf_size);
    memset(dst_buf, 0x80, dst_buf_size);
    rga_buffer_handle_t crop_handle = importbuffer_virtualaddr(crop_buf, crop_buf_size);
    rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
    rga_buffer_handle_t rot_handle = importbuffer_virtualaddr(rot_buf, crop_buf_size);
    if (crop_handle == 0 || dst_handle == 0) {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }
    crop_img = wrapbuffer_handle(crop_handle, crop_width, crop_height, format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, format);
    rot_img = wrapbuffer_handle(rot_handle, dst_height, dst_width, format);

    // check rect 
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&crop_rect, 0, sizeof(crop_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_rect.x = offset_width;
    src_rect.y = offset_height;
    src_rect.width = crop_width;
    src_rect.height = crop_height;

    ret = imcheck(src_img, crop_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    ret = imcrop(src_img, crop_img, src_rect);
    if (ret != IM_STATUS_SUCCESS) {
        printf("imcrop running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }
    if (dst_width < dst_height) {
        ret = imrotate(crop_img, rot_img, 90);
        if (ret != IM_STATUS_SUCCESS) {
            printf("imrotate running failed, %s\n", imStrError((IM_STATUS)ret));
            goto release_buffer;
        }
        ret = imresize(rot_img, dst_img);
    } else {
        ret = imresize(crop_img, dst_img);
    }
    if (ret != IM_STATUS_SUCCESS) {
        printf("imcrop running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

release_buffer:
    if (crop_handle) {
        releasebuffer_handle(crop_handle);
    }
    if (rot_handle) {
        releasebuffer_handle(rot_handle);
    }
    if (dst_handle) {
        releasebuffer_handle(dst_handle);
    }

    return dst_buf;
}

char* rga_crop_and_resize(char* pChar, int src_width, int src_height, int dst_width, int dst_height, int offset_width, int offset_height, int crop_width, int crop_height) {
    int ret = 0;
    im_rect src_rect, crop_rect, dst_rect;
    rga_buffer_t src_img, crop_img, dst_img, rot_img;
    
    int format = RK_FORMAT_RGB_888;
    // image buf allocate 
    int src_buf_size = src_width * src_height * 3;
    int crop_buf_size = crop_width * crop_height * 3;
    int dst_buf_size = dst_width * dst_height * 3;
    char *src_buf = (char *)malloc(src_buf_size);
    char *crop_buf = (char *)malloc(crop_buf_size);
    char *rot_buf = (char *)malloc(crop_buf_size);
    char *dst_buf = (char *)malloc(dst_buf_size);
    memcpy(src_buf, pChar, src_buf_size);
    memset(crop_buf, 0x80, crop_buf_size);
    memset(rot_buf, 0x80, crop_buf_size);
    memset(dst_buf, 0x80, dst_buf_size);
    rga_buffer_handle_t src_handle = importbuffer_virtualaddr(src_buf, src_buf_size);
    rga_buffer_handle_t crop_handle = importbuffer_virtualaddr(crop_buf, crop_buf_size);
    rga_buffer_handle_t rot_handle = importbuffer_virtualaddr(rot_buf, crop_buf_size);
    rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
    if (src_handle == 0 || crop_handle == 0 || dst_handle == 0 || rot_handle == 0) {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }
    src_img = wrapbuffer_handle(src_handle, src_width, src_height, format);
    crop_img = wrapbuffer_handle(crop_handle, crop_width, crop_height, format);
    rot_img = wrapbuffer_handle(rot_handle, crop_height, crop_width, format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, format);

    // check rect 
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&crop_rect, 0, sizeof(crop_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_rect.x = offset_width;
    src_rect.y = offset_height;
    src_rect.width = crop_width;
    src_rect.height = crop_height;

    ret = imcheck(src_img, crop_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    ret = imcrop(src_img, crop_img, src_rect);
    if (ret != IM_STATUS_SUCCESS) {
        printf("imcrop running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }
    if (crop_width < crop_height) {
        ret = imrotate(crop_img, rot_img, IM_HAL_TRANSFORM_ROT_90);
        if (ret != IM_STATUS_SUCCESS) {
            printf("imrotate running failed, %s\n", imStrError((IM_STATUS)ret));
            goto release_buffer;
        }
        ret = imresize(rot_img, dst_img);
    } else {
        ret = imresize(crop_img, dst_img);
    }
    if (ret != IM_STATUS_SUCCESS) {
        printf("imcrop running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

release_buffer:
    if (src_handle) {
        releasebuffer_handle(src_handle);
    }
    if (crop_handle) {
        releasebuffer_handle(crop_handle);
    }
    if (rot_handle) {
        releasebuffer_handle(rot_handle);
    }
    if (dst_handle) {
        releasebuffer_handle(dst_handle);
    }
    if (src_buf) {
        free(src_buf);
    }
    if (crop_buf) {
        free(crop_buf);
    }
    if (rot_buf) {
        free(rot_buf);
    }

    return dst_buf;
}

char* rga_crop_and_resize_fill(char* pChar, int src_width, int src_height, int dst_width, int dst_height, int offset_width, int offset_height, int crop_width, int crop_height) {
    int ret = 0;
    im_rect src_rect, dst_rect;
    rga_buffer_t src_img, crop_img, rot_img, dst_img;
    bool bNeedRotate = (crop_width < crop_height ? true : false);
    int fill_width = crop_width, fill_height = crop_height;
    if (bNeedRotate) {
        fill_width = ALIGN(crop_width, 16);
        fill_height = ALIGN(crop_height, 16);
    }
    
    int format = RK_FORMAT_RGB_888;
    // image buf allocate 
    int src_buf_size = src_width * src_height * 3;
    int crop_buf_size = fill_width * fill_height * 3;
    int rot_buf_size = fill_height * fill_width * 3;
    int dst_buf_size = dst_width * dst_height * 3;
    char *src_buf = (char *)malloc(src_buf_size);
    char *crop_buf = (char *)malloc(crop_buf_size);
    char *rot_buf = (char *)malloc(rot_buf_size);
    char *dst_buf = (char *)malloc(dst_buf_size);
    memcpy(src_buf, pChar, src_buf_size);
    memset(crop_buf, 0x80, crop_buf_size);
    memset(rot_buf, 0x80, rot_buf_size);
    memset(dst_buf, 0x80, dst_buf_size);
    rga_buffer_handle_t src_handle = importbuffer_virtualaddr(src_buf, src_buf_size);
    rga_buffer_handle_t crop_handle = importbuffer_virtualaddr(crop_buf, crop_buf_size);
    rga_buffer_handle_t rot_handle = importbuffer_virtualaddr(rot_buf, rot_buf_size);
    rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
    if (src_handle == 0 || crop_handle == 0 || dst_handle == 0 || rot_handle == 0) {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }
    src_img = wrapbuffer_handle(src_handle, src_width, src_height, format);
    crop_img = wrapbuffer_handle(crop_handle, fill_width, fill_height, format);
    rot_img = wrapbuffer_handle(rot_handle, fill_height, fill_width, format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, format);

    // check rect 
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_rect.x = offset_width;
    src_rect.y = offset_height;
    src_rect.width = crop_width;
    src_rect.height = crop_height;

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = crop_width;
    dst_rect.height = crop_height;
    ret = imcheck(src_img, crop_img, src_rect, dst_rect);
    // crop
    ret = improcess(src_img, crop_img, {}, src_rect, dst_rect, {}, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS) {
        printf("imcrop running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    if (bNeedRotate) {
        ret = imrotate(crop_img, rot_img, IM_HAL_TRANSFORM_ROT_90);
        if (ret != IM_STATUS_SUCCESS) {
            printf("imrotate running failed, %s\n", imStrError((IM_STATUS)ret));
            goto release_buffer;
        }
        ret = imresize(rot_img, dst_img);
    }
    else {
        ret = imresize(crop_img, dst_img);
    }
    if (ret != IM_STATUS_SUCCESS) {
        printf("imresize running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

release_buffer:
    if (src_handle) {
        releasebuffer_handle(src_handle);
    }
    if (crop_handle) {
        releasebuffer_handle(crop_handle);
    }
    if (rot_handle) {
        releasebuffer_handle(rot_handle);
    }
    if (dst_handle) {
        releasebuffer_handle(dst_handle);
    }
    if (src_buf) {
        free(src_buf);
    }
    if (crop_buf) {
        free(crop_buf);
    }
    if (rot_buf) {
        free(rot_buf);
    }

    return dst_buf;
}


char* rga_crop_and_resize_fixsize(char* pChar, int src_width, int src_height, int dst_width, int dst_height, int offset_width, int offset_height, int crop_width, int crop_height) {
    // printf("rga_crop_and_resize_fixsize %d-%d -> %d-%d\n", src_width, src_height, crop_width, crop_height);
    int fill_width = crop_width, fill_height = 1088;
    int ret = 0;
    im_rect src_rect, crop_rect, dst_rect;
    rga_buffer_t src_img, crop_img, dst_img, rot_img, fill_img;
    
    int format = RK_FORMAT_RGB_888;
    // image buf allocate 
    int src_buf_size = src_width * src_height * 3;
    int crop_buf_size = crop_width * crop_height * 3;
    int fill_buf_size = fill_width * fill_height * 3;
    int dst_buf_size = dst_width * dst_height * 3;
    char *src_buf = (char *)malloc(src_buf_size);
    char *crop_buf = (char *)malloc(crop_buf_size);
    // char *fill_buf = (char *)malloc(fill_buf_size);
    char *rot_buf = (char *)malloc(fill_buf_size);
    char *dst_buf = (char *)malloc(dst_buf_size);
    memcpy(src_buf, pChar, src_buf_size);
    memset(crop_buf, 0x80, crop_buf_size);
    // memset(fill_buf, 0x80, fill_buf_size);
    memset(rot_buf, 0x80, fill_buf_size);
    memset(dst_buf, 0x80, dst_buf_size);
    rga_buffer_handle_t src_handle = importbuffer_virtualaddr(src_buf, src_buf_size);
    rga_buffer_handle_t crop_handle = importbuffer_virtualaddr(crop_buf, crop_buf_size);
    // rga_buffer_handle_t fill_handle = importbuffer_virtualaddr(crop_buf, fill_buf_size);
    rga_buffer_handle_t rot_handle = importbuffer_virtualaddr(rot_buf, fill_buf_size);
    rga_buffer_handle_t dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
    if (src_handle == 0 || crop_handle == 0 || dst_handle == 0 || rot_handle == 0) {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }
    src_img = wrapbuffer_handle(src_handle, src_width, src_height, format);
    crop_img = wrapbuffer_handle(crop_handle, crop_width, crop_height, format);
    // fill_img = wrapbuffer_handle(fill_handle, fill_width, fill_height, format);
    rot_img = wrapbuffer_handle(rot_handle, fill_height, fill_width, format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, format);

    // check rect 
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&crop_rect, 0, sizeof(crop_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_rect.x = offset_width;
    src_rect.y = offset_height;
    src_rect.width = crop_width;
    src_rect.height = crop_height;

    // ret = imcheck(src_img, fill_img, src_rect, dst_rect);
    // if (IM_STATUS_NOERROR != ret) {
    //     printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
    //     goto release_buffer;
    // }

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = crop_width;
    dst_rect.height = crop_height;
    ret = imcheck(src_img, crop_img, src_rect, dst_rect);
    // crop
    ret = imcrop(src_img, crop_img, src_rect);
    if (ret != IM_STATUS_SUCCESS) {
        printf("imcrop running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    ret = imrotate(crop_img, rot_img, IM_HAL_TRANSFORM_ROT_90);
    if (ret != IM_STATUS_SUCCESS) {
        printf("imrotate running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }
    ret = imresize(rot_img, dst_img);

    if (ret != IM_STATUS_SUCCESS) {
        printf("imcrop running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

release_buffer:
    if (src_handle) {
        releasebuffer_handle(src_handle);
    }
    if (crop_handle) {
        releasebuffer_handle(crop_handle);
    }
    if (rot_handle) {
        releasebuffer_handle(rot_handle);
    }
    if (dst_handle) {
        releasebuffer_handle(dst_handle);
    }
    if (src_buf) {
        free(src_buf);
    }
    if (crop_buf) {
        free(crop_buf);
    }
    if (rot_buf) {
        free(rot_buf);
    }

    return dst_buf;
}
