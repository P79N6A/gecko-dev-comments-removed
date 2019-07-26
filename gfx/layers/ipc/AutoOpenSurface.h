






#ifndef mozilla_layers_AutoOpenSurface_h
#define mozilla_layers_AutoOpenSurface_h 1

#include "base/basictypes.h"

#include "gfxASurface.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/layers/PLayers.h"
#include "ShadowLayers.h"

namespace mozilla {
namespace layers {







class NS_STACK_CLASS AutoOpenSurface
{
public:
  typedef gfxASurface::gfxContentType gfxContentType;

  

  AutoOpenSurface(OpenMode aMode, const SurfaceDescriptor& aDescriptor);

  ~AutoOpenSurface();

  



  gfxContentType ContentType();
  gfxIntSize Size();

  
  gfxASurface* Get();

  mozilla::gfx::DrawTarget* GetDrawTarget();

  









  gfxImageSurface* GetAsImage();


private:
  SurfaceDescriptor mDescriptor;
  nsRefPtr<gfxASurface> mSurface;
  RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;
  nsRefPtr<gfxImageSurface> mSurfaceAsImage;
  OpenMode mMode;

  AutoOpenSurface(const AutoOpenSurface&) MOZ_DELETE;
  AutoOpenSurface& operator=(const AutoOpenSurface&) MOZ_DELETE;
};

} 
} 

#endif 
