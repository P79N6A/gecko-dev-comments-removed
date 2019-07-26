



#ifndef GFXCOWSURFACEWRAPPER
#define GFXCOWSURFACEWRAPPER

#include "gfxImageSurface.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"






















class gfxReusableSurfaceWrapper {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(gfxReusableSurfaceWrapper)
public:
  virtual ~gfxReusableSurfaceWrapper() {}

  


  virtual const unsigned char* GetReadOnlyData() const = 0;

  


  virtual gfxASurface::gfxImageFormat Format() = 0;

  





  virtual gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface) = 0;

  








  virtual void ReadLock() = 0;
  virtual void ReadUnlock() = 0;

protected:
  NS_DECL_OWNINGTHREAD
};

#endif
