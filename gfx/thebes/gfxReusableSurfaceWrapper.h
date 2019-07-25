



#ifndef GFXCOWSURFACEWRAPPER
#define GFXCOWSURFACEWRAPPER

#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"

class gfxImageSurface;

















class gfxReusableSurfaceWrapper {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(gfxReusableSurfaceWrapper);
public:
  




  gfxReusableSurfaceWrapper(gfxImageSurface* aSurface);

  ~gfxReusableSurfaceWrapper();

  const unsigned char* GetReadOnlyData() const {
    NS_ABORT_IF_FALSE(mReadCount > 0, "Should have read lock");
    return mSurfaceData;
  }

  




  gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface);

  




  void ReadLock();
  void ReadUnlock();

private:
  NS_DECL_OWNINGTHREAD
  nsRefPtr<gfxImageSurface>   mSurface;
  const unsigned char*        mSurfaceData;
  PRInt32                     mReadCount;
};

#endif 
