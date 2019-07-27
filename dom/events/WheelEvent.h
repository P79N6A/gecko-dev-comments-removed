





#ifndef mozilla_dom_WheelEvent_h_
#define mozilla_dom_WheelEvent_h_

#include "nsIDOMWheelEvent.h"
#include "mozilla/dom/MouseEvent.h"
#include "mozilla/dom/WheelEventBinding.h"
#include "mozilla/EventForwards.h"

namespace mozilla {
namespace dom {

class WheelEvent : public MouseEvent,
                   public nsIDOMWheelEvent
{
public:
  WheelEvent(EventTarget* aOwner,
             nsPresContext* aPresContext,
             WidgetWheelEvent* aWheelEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMWHEELEVENT
  
  
  NS_FORWARD_TO_MOUSEEVENT

  static
  already_AddRefed<WheelEvent> Constructor(const GlobalObject& aGlobal,
                                           const nsAString& aType,
                                           const WheelEventInit& aParam,
                                           ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return WheelEventBinding::Wrap(aCx, this);
  }

  
  
  
  double DeltaX();
  double DeltaY();
  double DeltaZ();
  uint32_t DeltaMode();

protected:
  ~WheelEvent() {}

private:
  int32_t mAppUnitsPerDevPixel;
};

} 
} 

#endif 
