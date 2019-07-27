






#ifndef mozilla_layers_Composer2D_h
#define mozilla_layers_Composer2D_h

#include "gfxTypes.h"
#include "nsISupportsImpl.h"
















namespace mozilla {

namespace gfx {
class Matrix;
}

namespace layers {

class Layer;

class Composer2D {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Composer2D)

protected:
  
  virtual ~Composer2D() {}

public:
  







  virtual bool TryRender(Layer* aRoot, bool aGeometryChanged) = 0;
};

} 
} 

#endif 
