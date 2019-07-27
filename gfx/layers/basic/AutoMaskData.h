




#ifndef GFX_AUTOMASKDATA_H_
#define GFX_AUTOMASKDATA_H_

#include "mozilla/layers/LayersSurfaces.h"  

namespace mozilla {
namespace layers {






class MOZ_STACK_CLASS AutoMoz2DMaskData {
public:
  AutoMoz2DMaskData() { }
  ~AutoMoz2DMaskData() { }

  void Construct(const gfx::Matrix& aTransform,
                 gfx::SourceSurface* aSurface)
  {
    MOZ_ASSERT(!IsConstructed());
    mTransform = aTransform;
    mSurface = aSurface;
  }

  gfx::SourceSurface* GetSurface()
  {
    MOZ_ASSERT(IsConstructed());
    return mSurface.get();
  }

  const gfx::Matrix& GetTransform()
  {
    MOZ_ASSERT(IsConstructed());
    return mTransform;
  }

private:
  bool IsConstructed()
  {
    return !!mSurface;
  }

  gfx::Matrix mTransform;
  RefPtr<gfx::SourceSurface> mSurface;

  AutoMoz2DMaskData(const AutoMoz2DMaskData&) = delete;
  AutoMoz2DMaskData& operator=(const AutoMoz2DMaskData&) = delete;
};

} 
} 

#endif 
