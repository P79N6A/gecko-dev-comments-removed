










#ifndef __VPX_SCALE_ARBITRARY_H__
#define __VPX_SCALE_ARBITRARY_H__

#include "vpx_scale/yv12config.h"

typedef struct {
  int in_width;
  int in_height;

  int out_width;
  int out_height;
  int max_usable_out_width;

  
  int nw;
  int nh;
  int nh_uv;

  
  short *l_w;
  short *l_h;
  short *l_h_uv;

  
  short *c_w;
  short *c_h;
  short *c_h_uv;

  
  unsigned char *hbuf;
  unsigned char *hbuf_uv;
} BICUBIC_SCALER_STRUCT;

int bicubic_coefficient_setup(int in_width, int in_height, int out_width, int out_height);
int bicubic_scale(int in_width, int in_height, int in_stride,
                  int out_width, int out_height, int out_stride,
                  unsigned char *input_image, unsigned char *output_image);
void bicubic_scale_frame_reset();
void bicubic_scale_frame(YV12_BUFFER_CONFIG *src, YV12_BUFFER_CONFIG *dst,
                         int new_width, int new_height);
void bicubic_coefficient_init();
void bicubic_coefficient_destroy();

#endif 
