





#ifndef MOZILLA_GFX_TEXTUREHOSTBASIC_H_
#define MOZILLA_GFX_TEXTUREHOSTBASIC_H_

#include "CompositableHost.h"
#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/layers/TextureHost.h"
#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {




class TextureSourceBasic
{
public:
  virtual ~TextureSourceBasic() {}
  virtual gfx::SourceSurface* GetSurface(gfx::DrawTarget* aTarget) = 0;
};

} 
} 

#endif 
