




#ifndef GFX_IMAGETYPES_H
#define GFX_IMAGETYPES_H

#include "mozilla/TypedEnum.h"

namespace mozilla {

enum class ImageFormat {
  




  PLANAR_YCBCR,

  




  GRALLOC_PLANAR_YCBCR,

  







  GONK_CAMERA_IMAGE,

  




  SHARED_RGB,

  












  CAIRO_SURFACE,

  


  MAC_IOSURFACE,

  



  SURFACE_TEXTURE,

  


  EGLIMAGE,

  



  D3D9_RGB32_TEXTURE,

  



  OVERLAY_IMAGE
};

enum class StereoMode {
  MONO,
  LEFT_RIGHT,
  RIGHT_LEFT,
  BOTTOM_TOP,
  TOP_BOTTOM
};

} 

#endif
