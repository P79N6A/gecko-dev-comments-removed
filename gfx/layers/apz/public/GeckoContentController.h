





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

  




  virtual void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                       const uint32_t& aScrollGeneration) = 0;

  





  virtual void HandleDoubleTap(const CSSPoint& aPoint,
                               int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) = 0;

  




  virtual void HandleSingleTap(const CSSPoint& aPoint,
                               int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) = 0;

  



  virtual void HandleLongTap(const CSSPoint& aPoint,
                             int32_t aModifiers,
                             const ScrollableLayerGuid& aGuid) = 0;

  







  virtual void HandleLongTapUp(const CSSPoint& aPoint,
                               int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) = 0;

  




  virtual void SendAsyncScrollDOMEvent(bool aIsRoot,
                                       const CSSRect &aContentRect,
                                       const CSSSize &aScrollableSize) = 0;

  



  virtual void PostDelayedTask(Task* aTask, int aDelayMs) = 0;

  




  virtual bool GetRootZoomConstraints(ZoomConstraints* aOutConstraints)
  {
    return false;
  }

  










  virtual bool GetTouchSensitiveRegion(CSSRect* aOutRegion)
  {
    return false;
  }

  enum APZStateChange {
    


    TransformBegin,
    


    TransformEnd,
    



    StartTouch,
    


    StartPanning,
    



    EndTouch,
    APZStateChangeSentinel
  };

  






  virtual void NotifyAPZStateChange(const ScrollableLayerGuid& aGuid,
                                    APZStateChange aChange,
                                    int aArg = 0) {}

  GeckoContentController() {}

protected:
  
  virtual ~GeckoContentController() {}
};

}
}

#endif 
