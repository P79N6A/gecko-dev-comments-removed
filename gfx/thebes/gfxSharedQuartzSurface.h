





#ifndef GFX_SHARED_QUARTZSURFACE_H
#define GFX_SHARED_QUARTZSURFACE_H

#include "gfxBaseSharedMemorySurface.h"
#include "gfxQuartzSurface.h"

class gfxSharedQuartzSurface : public gfxBaseSharedMemorySurface<gfxQuartzSurface, gfxSharedQuartzSurface>
{
  typedef gfxBaseSharedMemorySurface<gfxQuartzSurface, gfxSharedQuartzSurface> Super;
  friend class gfxBaseSharedMemorySurface<gfxQuartzSurface, gfxSharedQuartzSurface>;
private:
    gfxSharedQuartzSurface(const gfxIntSize& aSize, long aStride, 
                           gfxImageFormat aFormat, 
                           const mozilla::ipc::Shmem& aShmem)
      : Super(aSize, aStride, aFormat, aShmem)
    {}
};

#endif 
