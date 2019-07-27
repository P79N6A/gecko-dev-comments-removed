






#ifndef mozilla_layers_Composer2D_h
#define mozilla_layers_Composer2D_h

#include "gfxTypes.h"
#include "nsISupportsImpl.h"
















class nsIWidget;

namespace mozilla {
namespace layers {

class Layer;

class Composer2D {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Composer2D)

protected:
  
  virtual ~Composer2D() {}

public:
  







  virtual bool TryRenderWithHwc(Layer* aRoot, nsIWidget* aWidget,
                                bool aGeometryChanged) = 0;

  



  virtual bool Render(nsIWidget* aWidget) = 0;

  



  virtual bool HasHwc() = 0;
};

} 
} 

#endif 
