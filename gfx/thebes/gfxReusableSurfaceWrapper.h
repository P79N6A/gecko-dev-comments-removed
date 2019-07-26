



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

  


  virtual gfxImageFormat Format() = 0;

  





  virtual gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface) = 0;

  








  virtual void ReadLock() = 0;
  virtual void ReadUnlock() = 0;

  


  enum Type {
    TYPE_SHARED_IMAGE,
    TYPE_IMAGE,

    TYPE_MAX
  };

  


  virtual Type GetType() = 0;

protected:
  NS_DECL_OWNINGTHREAD
};

#endif
