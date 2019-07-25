







































#include "ycbcr_to_rgb565.h"

#ifdef HAVE_YCBCR_TO_RGB565

namespace mozilla {

namespace gfx {

# if defined(MOZILLA_MAY_SUPPORT_NEON)

void __attribute((noinline)) yuv42x_to_rgb565_row_neon(uint16 *dst,
                                                       const uint8 *y,
                                                       const uint8 *u,
                                                       const uint8 *v,
                                                       int n,
                                                       int oddflag);

#endif


static PRUint16 yu2rgb565(int y, int u, int v) {
  int r;
  int g;
  int b;
  r = NS_CLAMP((74*y+102*v-14240+256)>>9, 0, 31);
  g = NS_CLAMP((74*y-25*u-52*v+8704+128)>>8, 0, 63);
  b = NS_CLAMP((74*y+129*u-17696+256)>>9, 0, 31);
  return (PRUint16)(r<<11 | g<<5 | b);
}



void yuv_to_rgb565_row_c(uint16 *dst,
                         const uint8 *y,
                         const uint8 *u,
                         const uint8 *v,
                         int x_shift,
                         int pic_x,
                         int pic_width)
{
  int x;
  for (x = 0; x < pic_width; x++)
  {
    dst[x] = yu2rgb565(y[pic_x+x],
                       u[(pic_x+x)>>x_shift],
                       v[(pic_x+x)>>x_shift]);
  }
}

NS_GFX_(void) ConvertYCbCrToRGB565(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int pic_x,
                                   int pic_y,
                                   int pic_width,
                                   int pic_height,
                                   int y_pitch,
                                   int uv_pitch,
                                   int rgb_pitch,
                                   YUVType yuv_type)
{
  int x_shift;
  int y_shift;
  x_shift = yuv_type != YV24;
  y_shift = yuv_type == YV12;
#  ifdef MOZILLA_MAY_SUPPORT_NEON
  if (yuv_type != YV24 && supports_neon())
  {
    for (int i = 0; i < pic_height; i++) {
      int yoffs;
      int uvoffs;
      yoffs = y_pitch * (pic_y+i) + pic_x;
      uvoffs = uv_pitch * ((pic_y+i)>>y_shift) + (pic_x>>x_shift);
      yuv42x_to_rgb565_row_neon((uint16*)(rgb_buf + rgb_pitch * i),
                                y_buf + yoffs,
                                u_buf + uvoffs,
                                v_buf + uvoffs,
                                pic_width,
                                pic_x&x_shift);
    }
  }
  else
#  endif
  {
    for (int i = 0; i < pic_height; i++) {
      int yoffs;
      int uvoffs;
      yoffs = y_pitch * (pic_y+i);
      uvoffs = uv_pitch * ((pic_y+i)>>y_shift);
      yuv_to_rgb565_row_c((uint16*)(rgb_buf + rgb_pitch * i),
                          y_buf + yoffs,
                          u_buf + uvoffs,
                          v_buf + uvoffs,
                          x_shift,
                          pic_x,
                          pic_width);
    }
  }
}

NS_GFX_(bool) IsConvertYCbCrToRGB565Fast(int pic_x,
                                         int pic_y,
                                         int pic_width,
                                         int pic_height,
                                         YUVType yuv_type)
{
#  if defined(MOZILLA_MAY_SUPPORT_NEON)
  return (yuv_type != YV24 && supports_neon());
#  else
  return false;
#  endif
}

} 

} 

#endif 
