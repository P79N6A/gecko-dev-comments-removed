






#ifndef mozilla_layers_Composer2D_h
#define mozilla_layers_Composer2D_h

#include "gfxTypes.h"
#include "nsISupportsImpl.h"
















struct gfxMatrix;

namespace mozilla {
namespace layers {

class Layer;

class Composer2D {
  NS_INLINE_DECL_REFCOUNTING(Composer2D)

public:
  virtual ~Composer2D() {}

  













  virtual bool TryRender(Layer* aRoot, const gfxMatrix& aWorldTransform) = 0;
};

} 
} 

#endif 
