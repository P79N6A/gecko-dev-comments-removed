



#ifndef GFXCOWSURFACEWRAPPER
#define GFXCOWSURFACEWRAPPER

#include "gfxASurface.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"

class gfxImageSurface;

















class gfxReusableSurfaceWrapper {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(gfxReusableSurfaceWrapper)
public:
  




  gfxReusableSurfaceWrapper(gfxImageSurface* aSurface);

  ~gfxReusableSurfaceWrapper();

  const unsigned char* GetReadOnlyData() const {
    NS_ABORT_IF_FALSE(mReadCount > 0, "Should have read lock");
    return mSurfaceData;
  }

  const gfxASurface::gfxImageFormat& Format() { return mFormat; }

  




  gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface);

  




  void ReadLock();
  void ReadUnlock();

private:
  NS_DECL_OWNINGTHREAD
  nsRefPtr<gfxImageSurface>         mSurface;
  const gfxASurface::gfxImageFormat mFormat;
  const unsigned char*              mSurfaceData;
  int32_t                           mReadCount;
};

#endif 
