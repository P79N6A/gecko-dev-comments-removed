




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

  






  void Construct(const gfxMatrix& aTransform,
                 gfxASurface* aSurface);

  void Construct(const gfxMatrix& aTransform,
                 const SurfaceDescriptor& aSurface);

  
  gfxASurface* GetSurface();
  const gfxMatrix& GetTransform();

private:
  bool IsConstructed();

  gfxMatrix mTransform;
  nsRefPtr<gfxASurface> mSurface;
  Maybe<AutoOpenSurface> mSurfaceOpener;

  AutoMaskData(const AutoMaskData&) MOZ_DELETE;
  AutoMaskData& operator=(const AutoMaskData&) MOZ_DELETE;
};

}
}

#endif 