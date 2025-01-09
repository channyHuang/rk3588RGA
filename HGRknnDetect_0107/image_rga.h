#ifndef IMAGE_RGA_H
#define IMAGE_RGA_H

#include "im2d.h"
#include <vector>

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

std::vector<std::pair<int, int>> calculateOffset(int src_width, int src_height, int crop_width, int crop_height) ;
char* rga_image_buf(char* pChar, int nWidth, int nHeight);
rga_buffer_t rga_image(char* pChar, int nWidth, int nHeight);
rga_buffer_t rga_crop(rga_buffer_t &src_img, int dst_width, int dst_height, int nOffsetWidth, int OffsetHeight) ;
rga_buffer_t rga_resize(rga_buffer_t &src_img, int dst_width, int dst_height) ;
char* rga_crop_resize(rga_buffer_t &src_img, int dst_width, int dst_height, int nOffsetWidth, int OffsetHeight, int nCropWidth, int nCropHeight);
void release(rga_buffer_t& img);
char* rga_crop_and_resize(char* pChar, int src_width, int src_height, int dst_width, int dst_height, int offset_width, int offset_height, int crop_width, int crop_height);
char* rga_crop_and_resize_fixsize(char* pChar, int src_width, int src_height, int dst_width, int dst_height, int offset_width, int offset_height, int crop_width, int crop_height);
char* rga_crop_and_resize_fill(char* pChar, int src_width, int src_height, int dst_width, int dst_height, int offset_width, int offset_height, int crop_width, int crop_height);

#endif