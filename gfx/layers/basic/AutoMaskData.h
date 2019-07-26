




#ifndef GFX_AUTOMASKDATA_H_
#define GFX_AUTOMASKDATA_H_

#include "gfxASurface.h"
#include "mozilla/layers/LayersSurfaces.h"  

namespace mozilla {
namespace layers {











class MOZ_STACK_CLASS AutoMaskData {
public:
  AutoMaskData() { }
  ~AutoMaskData() { }

  






  void Construct(const gfx::Matrix& aTransform,
                 gfxASurface* aSurface);

  void Construct(const gfx::Matrix& aTransform,
                 const SurfaceDescriptor& aSurface);

  
  gfxASurface* GetSurface();
  const gfx::Matrix& GetTransform();

private:
  bool IsConstructed();

  gfx::Matrix mTransform;
  nsRefPtr<gfxASurface> mSurface;
  Maybe<AutoOpenSurface> mSurfaceOpener;

  AutoMaskData(const AutoMaskData&) MOZ_DELETE;
  AutoMaskData& operator=(const AutoMaskData&) MOZ_DELETE;
};

}
}

#endif 
