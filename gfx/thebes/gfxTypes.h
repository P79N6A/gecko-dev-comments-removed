




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

    enum gfxSurfaceType {
        gfxSurfaceTypeImage,
        gfxSurfaceTypePDF,
        gfxSurfaceTypePS,
        gfxSurfaceTypeXlib,
        gfxSurfaceTypeXcb,
        gfxSurfaceTypeGlitz,           
        gfxSurfaceTypeQuartz,
        gfxSurfaceTypeWin32,
        gfxSurfaceTypeBeOS,
        gfxSurfaceTypeDirectFB,        
        gfxSurfaceTypeSVG,
        gfxSurfaceTypeOS2,
        gfxSurfaceTypeWin32Printing,
        gfxSurfaceTypeQuartzImage,
        gfxSurfaceTypeScript,
        gfxSurfaceTypeQPainter,
        gfxSurfaceTypeRecording,
        gfxSurfaceTypeVG,
        gfxSurfaceTypeGL,
        gfxSurfaceTypeDRM,
        gfxSurfaceTypeTee,
        gfxSurfaceTypeXML,
        gfxSurfaceTypeSkia,
        gfxSurfaceTypeSubsurface,
        gfxSurfaceTypeD2D,
        gfxSurfaceTypeMax
    };

    enum gfxContentType {
        GFX_CONTENT_COLOR       = 0x1000,
        GFX_CONTENT_ALPHA       = 0x2000,
        GFX_CONTENT_COLOR_ALPHA = 0x3000,
        GFX_CONTENT_SENTINEL    = 0xffff
    };

    




    enum gfxMemoryLocation {
      GFX_MEMORY_IN_PROCESS_HEAP,
      GFX_MEMORY_IN_PROCESS_NONHEAP,
      GFX_MEMORY_OUT_OF_PROCESS
    };

#endif
