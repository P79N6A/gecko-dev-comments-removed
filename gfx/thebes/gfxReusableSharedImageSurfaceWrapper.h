



#ifndef GFXSHMCOWSURFACEWRAPPER
#define GFXSHMCOWSURFACEWRAPPER

#include "gfxReusableSurfaceWrapper.h"
#include "mozilla/layers/ISurfaceAllocator.h"

class gfxSharedImageSurface;





class gfxReusableSharedImageSurfaceWrapper : public gfxReusableSurfaceWrapper {
public:
  gfxReusableSharedImageSurfaceWrapper(mozilla::layers::ISurfaceAllocator* aAllocator,
                                       gfxSharedImageSurface* aSurface);
  ~gfxReusableSharedImageSurfaceWrapper();

  const unsigned char* GetReadOnlyData() const MOZ_OVERRIDE;
  gfxASurface::gfxImageFormat Format() MOZ_OVERRIDE;
  gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface) MOZ_OVERRIDE;
  void ReadLock() MOZ_OVERRIDE;
  void ReadUnlock() MOZ_OVERRIDE;

  


  mozilla::ipc::Shmem& GetShmem();

  




  static already_AddRefed<gfxReusableSharedImageSurfaceWrapper>
  Open(mozilla::layers::ISurfaceAllocator* aAllocator, const mozilla::ipc::Shmem& aShmem);

private:
  mozilla::layers::ISurfaceAllocator*     mAllocator;
  nsRefPtr<gfxSharedImageSurface>         mSurface;
};

#endif 
