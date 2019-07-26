





#ifndef mozilla_layers_GeckoContentController_h
#define mozilla_layers_GeckoContentController_h

#include "FrameMetrics.h"               
#include "Units.h"                      
#include "mozilla/Assertions.h"         
#include "nsISupportsImpl.h"

class Task;

namespace mozilla {
namespace layers {

class GeckoContentController
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GeckoContentController)

  



  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) = 0;

  





  virtual void HandleDoubleTap(const CSSIntPoint& aPoint, int32_t aModifiers) = 0;

  




  virtual void HandleSingleTap(const CSSIntPoint& aPoint, int32_t aModifiers) = 0;

  



  virtual void HandleLongTap(const CSSIntPoint& aPoint, int32_t aModifiers) = 0;

  




  virtual void SendAsyncScrollDOMEvent(bool aIsRoot,
                                       const CSSRect &aContentRect,
                                       const CSSSize &aScrollableSize) = 0;

  



  virtual void PostDelayedTask(Task* aTask, int aDelayMs) = 0;

  




  virtual bool GetRootZoomConstraints(bool* aOutAllowZoom,
                                      CSSToScreenScale* aOutMinZoom,
                                      CSSToScreenScale* aOutMaxZoom)
  {
    return false;
  }

  




  virtual void NotifyTransformBegin(const ScrollableLayerGuid& aGuid) {}
  virtual void NotifyTransformEnd(const ScrollableLayerGuid& aGuid) {}

  GeckoContentController() {}
  virtual ~GeckoContentController() {}
};

}
}

#endif 
