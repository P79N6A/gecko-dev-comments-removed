




#ifndef GFX_IMAGETYPES_H
#define GFX_IMAGETYPES_H

namespace mozilla {

enum ImageFormat {
  




  PLANAR_YCBCR,

  




  GRALLOC_PLANAR_YCBCR,

  




  SHARED_RGB,

  












  CAIRO_SURFACE,

  


  MAC_IOSURFACE,

  


  REMOTE_IMAGE_BITMAP,

  


  SHARED_TEXTURE,

  


  REMOTE_IMAGE_DXGI_TEXTURE,

  



  D3D9_RGB32_TEXTURE

};


enum StereoMode {
  STEREO_MODE_MONO,
  STEREO_MODE_LEFT_RIGHT,
  STEREO_MODE_RIGHT_LEFT,
  STEREO_MODE_BOTTOM_TOP,
  STEREO_MODE_TOP_BOTTOM
};


} 

#endif
