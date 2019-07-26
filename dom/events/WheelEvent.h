





#ifndef mozilla_dom_WheelEvent_h_
#define mozilla_dom_WheelEvent_h_

#include "nsIDOMWheelEvent.h"
#include "nsDOMMouseEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/WheelEventBinding.h"

namespace mozilla {
namespace dom {

class WheelEvent : public nsDOMMouseEvent,
                   public nsIDOMWheelEvent
{
public:
  WheelEvent(EventTarget* aOwner,
             nsPresContext* aPresContext,
             WidgetWheelEvent* aWheelEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMWHEELEVENT
  
  
  NS_FORWARD_TO_NSDOMMOUSEEVENT

  static
  already_AddRefed<WheelEvent> Constructor(const GlobalObject& aGlobal,
                                           const nsAString& aType,
                                           const WheelEventInit& aParam,
                                           ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return WheelEventBinding::Wrap(aCx, aScope, this);
  }

  double DeltaX();
  double DeltaY();
  double DeltaZ();
  uint32_t DeltaMode();
};

} 
} 

#endif 
