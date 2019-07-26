





#ifndef mozilla_layers_GeckoContentController_h
#define mozilla_layers_GeckoContentController_h

#include "FrameMetrics.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace layers {

class GeckoContentController {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GeckoContentController)

  



  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) = 0;

  





  virtual void HandleDoubleTap(const nsIntPoint& aPoint) = 0;

  




  virtual void HandleSingleTap(const nsIntPoint& aPoint) = 0;

  



  virtual void HandleLongTap(const nsIntPoint& aPoint) = 0;

  GeckoContentController() {}
  virtual ~GeckoContentController() {}
};

}
}

#endif 
