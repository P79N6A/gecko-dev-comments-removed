





#ifndef mozilla_layers_GeckoContentController_h
#define mozilla_layers_GeckoContentController_h

#include "Layers.h"

namespace mozilla {
namespace layers {

class GeckoContentController {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GeckoContentController)

  



  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) = 0;

  GeckoContentController() {};
  virtual ~GeckoContentController() {};
};

}
}

#endif 
