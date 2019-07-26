




#ifndef GFX_TYPES_H
#define GFX_TYPES_H

#include <stdint.h>
#include "mozilla/TypedEnum.h"

typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_user_data_key cairo_user_data_key_t;

typedef void (*thebes_destroy_func_t) (void *data);





typedef double gfxFloat;




















MOZ_BEGIN_ENUM_CLASS(gfxBreakPriority)
  eNoBreak       = 0,
  eWordWrapBreak,
  eNormalBreak
MOZ_END_ENUM_CLASS(gfxBreakPriority)





MOZ_BEGIN_ENUM_CLASS(gfxImageFormat)
  ARGB32, 
  RGB24,  
  A8,     
  A1,     
  RGB16_565,  
  Unknown
MOZ_END_ENUM_CLASS(gfxImageFormat)

MOZ_BEGIN_ENUM_CLASS(gfxSurfaceType)
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
  D2D,
  Max
MOZ_END_ENUM_CLASS(gfxSurfaceType)

MOZ_BEGIN_ENUM_CLASS(gfxContentType)
  COLOR       = 0x1000,
  ALPHA       = 0x2000,
  COLOR_ALPHA = 0x3000,
  SENTINEL    = 0xffff
MOZ_END_ENUM_CLASS(gfxContentType)






MOZ_BEGIN_ENUM_CLASS(gfxMemoryLocation)
  IN_PROCESS_HEAP,
  IN_PROCESS_NONHEAP,
  OUT_OF_PROCESS
MOZ_END_ENUM_CLASS(gfxMemoryLocation)

#endif 
