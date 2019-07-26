



#ifndef GFXCOWSURFACEWRAPPER
#define GFXCOWSURFACEWRAPPER

#include "gfxASurface.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "mozilla/Atomics.h"
#include "mozilla/layers/ISurfaceAllocator.h"

class gfxSharedImageSurface;

















class gfxReusableSurfaceWrapper {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(gfxReusableSurfaceWrapper)
public:
  



  gfxReusableSurfaceWrapper(mozilla::layers::ISurfaceAllocator* aAllocator, gfxSharedImageSurface* aSurface);

  ~gfxReusableSurfaceWrapper();

  const unsigned char* GetReadOnlyData() const;

  mozilla::ipc::Shmem& GetShmem();

  




  static already_AddRefed<gfxReusableSurfaceWrapper>
  Open(mozilla::layers::ISurfaceAllocator* aAllocator, const mozilla::ipc::Shmem& aShmem);

  gfxASurface::gfxImageFormat Format();

  





  gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface);

  








  void ReadLock();
  void ReadUnlock();

private:
  NS_DECL_OWNINGTHREAD
  mozilla::layers::ISurfaceAllocator*     mAllocator;
  nsRefPtr<gfxSharedImageSurface>         mSurface;
};

#endif 
