



#ifndef GFXMEMCOWSURFACEWRAPPER
#define GFXMEMCOWSURFACEWRAPPER

#include "gfxReusableSurfaceWrapper.h"

class gfxImageSurface;





class gfxReusableImageSurfaceWrapper : public gfxReusableSurfaceWrapper {
public:
  explicit gfxReusableImageSurfaceWrapper(gfxImageSurface* aSurface);
protected:
  ~gfxReusableImageSurfaceWrapper();

public:
  const unsigned char* GetReadOnlyData() const MOZ_OVERRIDE;
  gfxImageFormat Format() MOZ_OVERRIDE;
  gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface) MOZ_OVERRIDE;
  void ReadLock() MOZ_OVERRIDE;
  void ReadUnlock() MOZ_OVERRIDE;

  Type GetType()
  {
    return TYPE_IMAGE;
  }

private:
  nsRefPtr<gfxImageSurface>         mSurface;
};

#endif 
