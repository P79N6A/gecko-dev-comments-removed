





#ifndef GFX_SHARED_IMAGESURFACE_H
#define GFX_SHARED_IMAGESURFACE_H

#include "gfxBaseSharedMemorySurface.h"

class gfxSharedImageSurface : public gfxBaseSharedMemorySurface<gfxImageSurface, gfxSharedImageSurface>
{
  typedef gfxBaseSharedMemorySurface<gfxImageSurface, gfxSharedImageSurface> Super;
  friend class gfxBaseSharedMemorySurface<gfxImageSurface, gfxSharedImageSurface>;
private:
    gfxSharedImageSurface(const gfxIntSize& aSize, long aStride, 
                          gfxImageFormat aFormat, 
                          const mozilla::ipc::Shmem& aShmem)
      : Super(aSize, aStride, aFormat, aShmem)
    {}
};

#endif 
