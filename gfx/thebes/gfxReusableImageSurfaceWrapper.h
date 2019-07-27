



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
  const unsigned char* GetReadOnlyData() const override;
  gfxImageFormat Format() override;
  gfxReusableSurfaceWrapper* GetWritable(gfxImageSurface** aSurface) override;
  void ReadLock() override;
  void ReadUnlock() override;

  Type GetType() override
  {
    return TYPE_IMAGE;
  }

private:
  nsRefPtr<gfxImageSurface>         mSurface;
};

#endif 
