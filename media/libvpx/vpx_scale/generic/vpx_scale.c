





















#include "./vpx_scale_rtcd.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_scale/vpx_scale.h"
#include "vpx_scale/yv12config.h"

typedef struct {
  int     expanded_frame_width;
  int     expanded_frame_height;

  int HScale;
  int HRatio;
  int VScale;
  int VRatio;

  YV12_BUFFER_CONFIG *src_yuv_config;
  YV12_BUFFER_CONFIG *dst_yuv_config;

} SCALE_VARS;























static
void scale1d_2t1_i
(
  const unsigned char *source,
  int source_step,
  unsigned int source_scale,
  unsigned int source_length,
  unsigned char *dest,
  int dest_step,
  unsigned int dest_scale,
  unsigned int dest_length
) {
  unsigned int i, j;
  unsigned int temp;
  int source_pitch = source_step;
  (void) source_length;
  (void) source_scale;
  (void) dest_scale;

  source_step *= 2;
  dest[0] = source[0];

  for (i = dest_step, j = source_step; i < dest_length * dest_step; i += dest_step, j += source_step) {
    temp = 8;
    temp += 3 * source[j - source_pitch];
    temp += 10 * source[j];
    temp += 3 * source[j + source_pitch];
    temp >>= 4;
    dest[i] = (char)(temp);
  }
}























static
void scale1d_2t1_ps
(
  const unsigned char *source,
  int source_step,
  unsigned int source_scale,
  unsigned int source_length,
  unsigned char *dest,
  int dest_step,
  unsigned int dest_scale,
  unsigned int dest_length
) {
  unsigned int i, j;

  (void) source_length;
  (void) source_scale;
  (void) dest_scale;

  source_step *= 2;
  j = 0;

  for (i = 0; i < dest_length * dest_step; i += dest_step, j += source_step)
    dest[i] = source[j];
}






















static
void scale1d_c
(
  const unsigned char *source,
  int source_step,
  unsigned int source_scale,
  unsigned int source_length,
  unsigned char *dest,
  int dest_step,
  unsigned int dest_scale,
  unsigned int dest_length
) {
  unsigned int i;
  unsigned int round_value = dest_scale / 2;
  unsigned int left_modifier = dest_scale;
  unsigned int right_modifier = 0;
  unsigned char left_pixel = *source;
  unsigned char right_pixel = *(source + source_step);

  (void) source_length;

  
  
  

  for (i = 0; i < dest_length * dest_step; i += dest_step) {
    dest[i] = (char)((left_modifier * left_pixel + right_modifier * right_pixel + round_value) / dest_scale);

    right_modifier += source_scale;

    while (right_modifier > dest_scale) {
      right_modifier -= dest_scale;
      source += source_step;
      left_pixel = *source;
      right_pixel = *(source + source_step);
    }

    left_modifier = dest_scale - right_modifier;
  }
}































static
void Scale2D
(
  
  unsigned char *source,
  int source_pitch,
  unsigned int source_width,
  unsigned int source_height,
  unsigned char *dest,
  int dest_pitch,
  unsigned int dest_width,
  unsigned int dest_height,
  unsigned char *temp_area,
  unsigned char temp_area_height,
  unsigned int hscale,
  unsigned int hratio,
  unsigned int vscale,
  unsigned int vratio,
  unsigned int interlaced
) {
  
  int i, j, k;
  int bands;
  int dest_band_height;
  int source_band_height;

  typedef void (*Scale1D)(const unsigned char * source, int source_step, unsigned int source_scale, unsigned int source_length,
                          unsigned char * dest, int dest_step, unsigned int dest_scale, unsigned int dest_length);

  Scale1D Scale1Dv = scale1d_c;
  Scale1D Scale1Dh = scale1d_c;

  void (*horiz_line_scale)(const unsigned char *, unsigned int, unsigned char *, unsigned int) = NULL;
  void (*vert_band_scale)(unsigned char *, unsigned int, unsigned char *, unsigned int, unsigned int) = NULL;

  int ratio_scalable = 1;
  int interpolation = 0;

  unsigned char *source_base; 
  unsigned char *line_src;


  source_base = (unsigned char *)source;

  if (source_pitch < 0) {
    int offset;

    offset = (source_height - 1);
    offset *= source_pitch;

    source_base += offset;
  }

  
  switch (hratio * 10 / hscale) {
    case 8:
      
      horiz_line_scale = vp8_horizontal_line_5_4_scale;
      break;
    case 6:
      
      horiz_line_scale = vp8_horizontal_line_5_3_scale;
      break;
    case 5:
      
      horiz_line_scale = vp8_horizontal_line_2_1_scale;
      break;
    default:
      
      
      ratio_scalable = 0;
      break;
  }

  switch (vratio * 10 / vscale) {
    case 8:
      
      vert_band_scale     = vp8_vertical_band_5_4_scale;
      source_band_height  = 5;
      dest_band_height    = 4;
      break;
    case 6:
      
      vert_band_scale     = vp8_vertical_band_5_3_scale;
      source_band_height  = 5;
      dest_band_height    = 3;
      break;
    case 5:
      

      if (interlaced) {
        
        vert_band_scale     = vp8_vertical_band_2_1_scale;
      } else {

        interpolation = 1;
        
        vert_band_scale     = vp8_vertical_band_2_1_scale_i;

      }

      source_band_height  = 2;
      dest_band_height    = 1;
      break;
    default:
      
      
      ratio_scalable = 0;
      break;
  }

  if (ratio_scalable) {
    if (source_height == dest_height) {
      
      for (k = 0; k < (int)dest_height; k++) {
        horiz_line_scale(source, source_width, dest, dest_width);
        source += source_pitch;
        dest   += dest_pitch;
      }

      return;
    }

    if (interpolation) {
      if (source < source_base)
        source = source_base;

      horiz_line_scale(source, source_width, temp_area, dest_width);
    }

    for (k = 0; k < (int)(dest_height + dest_band_height - 1) / dest_band_height; k++) {
      
      for (i = 0; i < source_band_height; i++) {
        

        line_src = (unsigned char *)source + i * source_pitch;

        if (line_src < source_base)
          line_src = source_base;

        horiz_line_scale(line_src, source_width,
                         temp_area + (i + 1)*dest_pitch, dest_width);
      }

      
      vert_band_scale(temp_area + dest_pitch, dest_pitch, dest, dest_pitch, dest_width);

      if (interpolation)
        memcpy(temp_area, temp_area + source_band_height * dest_pitch, dest_width);

      
      source += (unsigned long) source_band_height  * source_pitch;
      dest   += (unsigned long) dest_band_height * dest_pitch;
    }

    return;
  }

  if (hscale == 2 && hratio == 1)
    Scale1Dh = scale1d_2t1_ps;

  if (vscale == 2 && vratio == 1) {
    if (interlaced)
      Scale1Dv = scale1d_2t1_ps;
    else
      Scale1Dv = scale1d_2t1_i;
  }

  if (source_height == dest_height) {
    
    for (k = 0; k < (int)dest_height; k++) {
      Scale1Dh(source, 1, hscale, source_width + 1, dest, 1, hratio, dest_width);
      source += source_pitch;
      dest   += dest_pitch;
    }

    return;
  }

  if (dest_height > source_height) {
    dest_band_height   = temp_area_height - 1;
    source_band_height = dest_band_height * source_height / dest_height;
  } else {
    source_band_height = temp_area_height - 1;
    dest_band_height   = source_band_height * vratio / vscale;
  }

  
  Scale1Dh(source, 1, hscale, source_width + 1, temp_area, 1, hratio, dest_width);

  
  bands = (dest_height + dest_band_height - 1) / dest_band_height;

  for (k = 0; k < bands; k++) {
    
    for (i = 1; i < source_band_height + 1; i++) {
      if (k * source_band_height + i < (int) source_height) {
        Scale1Dh(source + i * source_pitch, 1, hscale, source_width + 1,
                 temp_area + i * dest_pitch, 1, hratio, dest_width);
      } else { 
        
        memcpy(temp_area + i * dest_pitch, temp_area + (i - 1)*dest_pitch, dest_pitch);
      }
    }

    
    for (j = 0; j < (int)dest_width; j++) {
      Scale1Dv(&temp_area[j], dest_pitch, vscale, source_band_height + 1,
               &dest[j], dest_pitch, vratio, dest_band_height);
    }

    
    memcpy(temp_area, temp_area + source_band_height * dest_pitch, dest_pitch);

    
    source += source_band_height * source_pitch;
    dest   += dest_band_height * dest_pitch;
  }
}

























void vpx_scale_frame
(
  YV12_BUFFER_CONFIG *src,
  YV12_BUFFER_CONFIG *dst,
  unsigned char *temp_area,
  unsigned char temp_height,
  unsigned int hscale,
  unsigned int hratio,
  unsigned int vscale,
  unsigned int vratio,
  unsigned int interlaced
) {
  int i;
  int dw = (hscale - 1 + src->y_width * hratio) / hscale;
  int dh = (vscale - 1 + src->y_height * vratio) / vscale;

  
  Scale2D((unsigned char *) src->y_buffer, src->y_stride, src->y_width, src->y_height,
          (unsigned char *) dst->y_buffer, dst->y_stride, dw, dh,
          temp_area, temp_height, hscale, hratio, vscale, vratio, interlaced);

  if (dw < (int)dst->y_width)
    for (i = 0; i < dh; i++)
      memset(dst->y_buffer + i * dst->y_stride + dw - 1, dst->y_buffer[i * dst->y_stride + dw - 2], dst->y_width - dw + 1);

  if (dh < (int)dst->y_height)
    for (i = dh - 1; i < (int)dst->y_height; i++)
      memcpy(dst->y_buffer + i * dst->y_stride, dst->y_buffer + (dh - 2) * dst->y_stride, dst->y_width + 1);

  Scale2D((unsigned char *) src->u_buffer, src->uv_stride, src->uv_width, src->uv_height,
          (unsigned char *) dst->u_buffer, dst->uv_stride, dw / 2, dh / 2,
          temp_area, temp_height, hscale, hratio, vscale, vratio, interlaced);

  if (dw / 2 < (int)dst->uv_width)
    for (i = 0; i < dst->uv_height; i++)
      memset(dst->u_buffer + i * dst->uv_stride + dw / 2 - 1, dst->u_buffer[i * dst->uv_stride + dw / 2 - 2], dst->uv_width - dw / 2 + 1);

  if (dh / 2 < (int)dst->uv_height)
    for (i = dh / 2 - 1; i < (int)dst->y_height / 2; i++)
      memcpy(dst->u_buffer + i * dst->uv_stride, dst->u_buffer + (dh / 2 - 2)*dst->uv_stride, dst->uv_width);

  Scale2D((unsigned char *) src->v_buffer, src->uv_stride, src->uv_width, src->uv_height,
          (unsigned char *) dst->v_buffer, dst->uv_stride, dw / 2, dh / 2,
          temp_area, temp_height, hscale, hratio, vscale, vratio, interlaced);

  if (dw / 2 < (int)dst->uv_width)
    for (i = 0; i < dst->uv_height; i++)
      memset(dst->v_buffer + i * dst->uv_stride + dw / 2 - 1, dst->v_buffer[i * dst->uv_stride + dw / 2 - 2], dst->uv_width - dw / 2 + 1);

  if (dh / 2 < (int) dst->uv_height)
    for (i = dh / 2 - 1; i < (int)dst->y_height / 2; i++)
      memcpy(dst->v_buffer + i * dst->uv_stride, dst->v_buffer + (dh / 2 - 2)*dst->uv_stride, dst->uv_width);
}
