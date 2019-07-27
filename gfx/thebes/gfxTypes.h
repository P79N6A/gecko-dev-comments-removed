




#ifndef GFX_TYPES_H
#define GFX_TYPES_H

#include <stdint.h>

typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_user_data_key cairo_user_data_key_t;

typedef void (*thebes_destroy_func_t) (void *data);





typedef double gfxFloat;




















enum class gfxBreakPriority {
  eNoBreak       = 0,
  eWordWrapBreak,
  eNormalBreak
};





enum class gfxImageFormat {
  ARGB32, 
  RGB24,  
  A8,     
  A1,     
  RGB16_565,  
  Unknown
};

enum class gfxSurfaceType {
  Image,
  PDF,
  PS,
  Xlib,
  Xcb,
  Glitz,           
  Quartz,
  Win32,
  BeOS,
  DirectFB,        
  SVG,
  OS2,
  Win32Printing,
  QuartzImage,
  Script,
  QPainter,
  Recording,
  VG,
  GL,
  DRM,
  Tee,
  XML,
  Skia,
  Subsurface,
  Max
};

enum class gfxContentType {
  COLOR       = 0x1000,
  ALPHA       = 0x2000,
  COLOR_ALPHA = 0x3000,
  SENTINEL    = 0xffff
};






enum class gfxMemoryLocation {
  IN_PROCESS_HEAP,
  IN_PROCESS_NONHEAP,
  OUT_OF_PROCESS
};

#endif
