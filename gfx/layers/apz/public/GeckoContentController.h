





#ifndef mozilla_layers_GeckoContentController_h
#define mozilla_layers_GeckoContentController_h

#include "FrameMetrics.h"               
#include "Units.h"                      
#include "mozilla/Assertions.h"         
#include "mozilla/EventForwards.h"      
#include "nsISupportsImpl.h"

class Task;

namespace mozilla {
namespace layers {

class GeckoContentController
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GeckoContentController)

  




  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) = 0;

  




  virtual void RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                                const mozilla::CSSPoint& aDestination) = 0;

  




  virtual void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                       const uint32_t& aScrollGeneration) = 0;

  





  virtual void HandleDoubleTap(const CSSPoint& aPoint,
                               Modifiers aModifiers,
                               const ScrollableLayerGuid& aGuid) = 0;

  




  virtual void HandleSingleTap(const CSSPoint& aPoint,
                               Modifiers aModifiers,
                               const ScrollableLayerGuid& aGuid) = 0;

  



  virtual void HandleLongTap(const CSSPoint& aPoint,
                             Modifiers aModifiers,
                             const ScrollableLayerGuid& aGuid,
                             uint64_t aInputBlockId) = 0;

  




  virtual void SendAsyncScrollDOMEvent(bool aIsRootContent,
                                       const CSSRect &aContentRect,
                                       const CSSSize &aScrollableSize) = 0;

  




  virtual void PostDelayedTask(Task* aTask, int aDelayMs) = 0;

  










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

  


  virtual void NotifyMozMouseScrollEvent(const FrameMetrics::ViewID& aScrollId, const nsString& aEvent)
  {}

  GeckoContentController() {}
  virtual void Destroy() {}

protected:
  
  virtual ~GeckoContentController() {}
};

}
}

#endif 
