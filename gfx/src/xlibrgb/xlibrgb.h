
































































#ifndef __XLIB_RGB_H__
#define __XLIB_RGB_H__


#undef FUNCPROTO
#define FUNCPROTO 15

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Intrinsic.h>

_XFUNCPROTOBEGIN






#ifdef USE_MOZILLA_TYPES

#include "prtypes.h"
#include "prcpucfg.h"
#include "nscore.h"

#ifdef _IMPL_XLIBRGB_API
#define NS_XLIBRGB_API(type) NS_EXPORT_(type)
#else
#define NS_XLIBRGB_API(type) NS_IMPORT_(type)
#endif

#define NS_TO_XXLIB_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)
#else
typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;
typedef short int16;

#define NS_XLIBRGB_API(type) type
#endif 

typedef struct _XlibRgbCmap XlibRgbCmap;
typedef struct _XlibRgbHandle XlibRgbHandle;

struct _XlibRgbCmap {
  unsigned int colors[256];
  unsigned char lut[256]; 
};


typedef enum
{
  XLIB_RGB_DITHER_NONE,
  XLIB_RGB_DITHER_NORMAL,
  XLIB_RGB_DITHER_MAX
} XlibRgbDither;

typedef struct
{
  const char *handle_name;
  int         pseudogray; 
  int         install_colormap;
  int         disallow_image_tiling;
  int         disallow_mit_shmem;
  int         verbose;
  XVisualInfo xtemplate;
  long        xtemplate_mask;
} XlibRgbArgs;

NS_XLIBRGB_API(XlibRgbHandle *)
xxlib_rgb_create_handle (Display *display, Screen *screen, 
                         XlibRgbArgs *args);
                                  
NS_XLIBRGB_API(void)
xxlib_rgb_destroy_handle (XlibRgbHandle *handle);

NS_XLIBRGB_API(unsigned long)
xxlib_rgb_xpixel_from_rgb (XlibRgbHandle *handle, uint32 rgb);

NS_XLIBRGB_API(void)
xxlib_rgb_gc_set_foreground (XlibRgbHandle *handle, GC gc, uint32 rgb);

NS_XLIBRGB_API(void)
xxlib_rgb_gc_set_background (XlibRgbHandle *handle, GC gc, uint32 rgb);

NS_XLIBRGB_API(void)
xxlib_draw_rgb_image (XlibRgbHandle *handle, Drawable drawable,
                      GC gc,
                      int x,
                      int y,
                      int width,
                      int height,
                      XlibRgbDither dith,
                      unsigned char *rgb_buf,
                      int rowstride);

NS_XLIBRGB_API(void)
xxlib_draw_rgb_image_dithalign (XlibRgbHandle *handle, Drawable drawable,
                                GC gc,
                                int x,
                                int y,
                                int width,
                                int height,
                                XlibRgbDither dith,
                                unsigned char *rgb_buf,
                                int rowstride,
                                int xdith,
                                int ydith);

NS_XLIBRGB_API(void)
xxlib_draw_rgb_32_image (XlibRgbHandle *handle, Drawable drawable,
                         GC gc,
                         int x,
                         int y,
                         int width,
                         int height,
                         XlibRgbDither dith,
                         unsigned char *buf,
                         int rowstride);

NS_XLIBRGB_API(void)
xxlib_draw_gray_image (XlibRgbHandle *handle, Drawable drawable,
                       GC gc,
                       int x,
                       int y,
                       int width,
                       int height,
                       XlibRgbDither dith,
                       unsigned char *buf,
                       int rowstride);

NS_XLIBRGB_API(XlibRgbCmap *)
xxlib_rgb_cmap_new (XlibRgbHandle *handle, uint32 *colors, int n_colors);

NS_XLIBRGB_API(void)
xxlib_rgb_cmap_free (XlibRgbHandle *handle, XlibRgbCmap *cmap);

NS_XLIBRGB_API(void)
xxlib_draw_indexed_image (XlibRgbHandle *handle, Drawable drawable,
                          GC gc,
                          int x,
                          int y,
                          int width,
                          int height,
                          XlibRgbDither dith,
                          unsigned char *buf,
                          int rowstride,
                          XlibRgbCmap *cmap);

NS_XLIBRGB_API(void)
xxlib_draw_xprint_scaled_rgb_image( XlibRgbHandle *handle,
                                    Drawable drawable,
                                    long paper_resolution,
                                    long image_resolution,
                                    GC gc,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    XlibRgbDither dith,
                                    unsigned char *rgb_buf,
                                    int rowstride);



NS_XLIBRGB_API(Bool)
xxlib_rgb_ditherable (XlibRgbHandle *handle);

NS_XLIBRGB_API(void)
xxlib_rgb_set_verbose (XlibRgbHandle *handle, Bool verbose);

NS_XLIBRGB_API(void)
xxlib_rgb_set_min_colors (XlibRgbHandle *handle, int min_colors);

NS_XLIBRGB_API(Colormap)
xxlib_rgb_get_cmap (XlibRgbHandle *handle);

NS_XLIBRGB_API(Visual *)
xxlib_rgb_get_visual (XlibRgbHandle *handle);

NS_XLIBRGB_API(XVisualInfo *)
xxlib_rgb_get_visual_info (XlibRgbHandle *handle);

NS_XLIBRGB_API(int)
xxlib_rgb_get_depth (XlibRgbHandle *handle);


NS_XLIBRGB_API(Display *)
xxlib_rgb_get_display (XlibRgbHandle *handle);

NS_XLIBRGB_API(Screen *)
xxlib_rgb_get_screen (XlibRgbHandle *handle);

NS_XLIBRGB_API(unsigned long)
xxlib_get_prec_from_mask(unsigned long);

NS_XLIBRGB_API(unsigned long)
xxlib_get_shift_from_mask(unsigned long);


#define XXLIBRGB_DEFAULT_HANDLE ("xxlib-default")

NS_XLIBRGB_API(Bool)
xxlib_register_handle(const char *name, XlibRgbHandle *handle);

NS_XLIBRGB_API(Bool)
xxlib_deregister_handle(const char *name);

NS_XLIBRGB_API(XlibRgbHandle *)
xxlib_find_handle(const char *name);

_XFUNCPROTOEND

#endif 

