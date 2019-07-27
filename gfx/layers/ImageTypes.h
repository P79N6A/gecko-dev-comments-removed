




#ifndef GFX_IMAGETYPES_H
#define GFX_IMAGETYPES_H

#include "mozilla/TypedEnum.h"

namespace mozilla {

MOZ_BEGIN_ENUM_CLASS(ImageFormat)
  




  PLANAR_YCBCR,

  




  GRALLOC_PLANAR_YCBCR,

  




  SHARED_RGB,

  












  CAIRO_SURFACE,

  


  MAC_IOSURFACE,

  


  REMOTE_IMAGE_BITMAP,

  



  SURFACE_TEXTURE,

  


  EGLIMAGE,

  


  REMOTE_IMAGE_DXGI_TEXTURE,

  



  D3D9_RGB32_TEXTURE,

  



  OVERLAY_IMAGE
MOZ_END_ENUM_CLASS(ImageFormat)

MOZ_BEGIN_ENUM_CLASS(StereoMode)
  MONO,
  LEFT_RIGHT,
  RIGHT_LEFT,
  BOTTOM_TOP,
  TOP_BOTTOM
MOZ_END_ENUM_CLASS(StereoMode)

} 

#endif
