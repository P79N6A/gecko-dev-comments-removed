



#ifndef GFXSHMCOWSURFACEWRAPPER
#define GFXSHMCOWSURFACEWRAPPER

#include "gfxReusableSurfaceWrapper.h"
#include "mozilla/RefPtr.h"

class gfxSharedImageSurface;

namespace mozilla {
namespace ipc {
class Shmem;
}
namespace layers {
class ISurfaceAllocator;
}
}





class gfxReusableSharedImageSurfaceWrapper : public gfxReusableSurfaceWrapper {
public:
  gfxReusableSharedImageSurfaceWrapper(mozilla::layers::ISurfaceAllocator* aAllocator,
                                       gfxSharedImageSurface* aSurface);
protected:
  ~gfxReusableSharedImageSurfaceWrapper();

public:
  const unsigned char* GetReadOnlyData() const MOZ_OVERRIDE;
  gfxImageFormat Format() MOZ_OVERRIDE;
  gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface) MOZ_OVERRIDE;
  void ReadLock() MOZ_OVERRIDE;
  void ReadUnlock() MOZ_OVERRIDE;

  Type GetType() MOZ_OVERRIDE
  {
    return TYPE_SHARED_IMAGE;
  }

  


  mozilla::ipc::Shmem& GetShmem();

  




  static already_AddRefed<gfxReusableSharedImageSurfaceWrapper>
  Open(mozilla::layers::ISurfaceAllocator* aAllocator, const mozilla::ipc::Shmem& aShmem);

private:
  mozilla::RefPtr<mozilla::layers::ISurfaceAllocator> mAllocator;
  nsRefPtr<gfxSharedImageSurface>         mSurface;
};

#endif 
