


































#ifndef CAIRO_DDRAW_PRIVATE_H
#define CAIRO_DDRAW_PRIVATE_H

#include "cairo-ddraw.h"
#include "cairoint.h"

#define FILL_THRESHOLD (1024u)

typedef struct _cairo_ddraw_surface {
  cairo_surface_t base;
  cairo_format_t format;
  LPDIRECTDRAWSURFACE lpdds;
  cairo_surface_t *image;
  cairo_surface_t *alias;
  cairo_point_int_t origin;
  uint32_t data_offset;
  LPDIRECTDRAWCLIPPER lpddc;
  LPDIRECTDRAWCLIPPER installed_clipper;
  cairo_rectangle_int_t extents;
  cairo_region_t clip_region;
  cairo_bool_t locked : 1;
  cairo_bool_t has_clip_region : 1;
  cairo_bool_t has_image_clip : 1;
  cairo_bool_t has_clip_list : 1;
  cairo_bool_t image_clip_invalid : 1;
  cairo_bool_t clip_list_invalid : 1;
} cairo_ddraw_surface_t;

#endif 
