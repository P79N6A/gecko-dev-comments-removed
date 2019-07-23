

































#ifndef CAIRO_DDRAW_PRIVATE_H
#define CAIRO_DDRAW_PRIVATE_H

#include "cairo-ddraw.h"
#include "cairoint.h"

#ifdef CAIRO_DDRAW_USE_GL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#define CAIRO_DDRAW_FILL_ACCELERATION

#undef CAIRO_DDRAW_CREATE_SIMILAR
#undef CAIRO_DDRAW_CLONE_SIMILAR

#ifdef CAIRO_DDRAW_USE_GL



#define CAIRO_DDRAW_FONT_ACCELERATION
#undef CAIRO_DDRAW_COMPOSITE_ACCELERATION

#endif 

#ifdef CAIRO_DDRAW_USE_GL
#define CAIRO_DDRAW_FILL_THRESHOLD    32
#else
#define CAIRO_DDRAW_FILL_THRESHOLD    1024
#endif

typedef struct _cairo_ddraw_surface cairo_ddraw_surface_t;

struct _cairo_ddraw_surface {

  



  
  cairo_surface_t base;

  
  cairo_format_t format;

  
  cairo_ddraw_surface_t *root;

  
  cairo_point_int_t origin;

  
  cairo_surface_t *image;

  
  uint32_t data_offset;

  
  cairo_rectangle_int_t extents;

  
  cairo_region_t clip_region;

  
  LPDIRECTDRAWCLIPPER lpddc;

  



  
  LPDIRECTDRAW lpdd;

  
  LPDIRECTDRAWSURFACE lpdds;

  



  
  LPDIRECTDRAWCLIPPER installed_clipper;

#ifdef CAIRO_DDRAW_USE_GL

  
  GLuint gl_id;

#endif 

  



  
  cairo_bool_t locked : 1;

#ifdef CAIRO_DDRAW_USE_GL

  
  cairo_bool_t dirty : 1;

#endif 

  


  
  
  cairo_bool_t has_clip_region : 1;

  
  cairo_bool_t has_image_clip : 1;

  
  cairo_bool_t image_clip_invalid : 1;

  
  cairo_bool_t clip_list_invalid : 1;
};

#endif 
