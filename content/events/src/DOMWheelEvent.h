





#ifndef mozilla_dom_DOMWheelEvent_h__
#define mozilla_dom_DOMWheelEvent_h__

#include "nsIDOMWheelEvent.h"
#include "nsDOMMouseEvent.h"
#include "mozilla/dom/WheelEventBinding.h"

namespace mozilla {
namespace dom {

class DOMWheelEvent : public nsDOMMouseEvent,
                      public nsIDOMWheelEvent
{
public:
  DOMWheelEvent(mozilla::dom::EventTarget* aOwner,
                nsPresContext* aPresContext,
                WheelEvent* aWheelEvent);
  virtual ~DOMWheelEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMWHEELEVENT
  
  
  NS_FORWARD_TO_NSDOMMOUSEEVENT

  static
  already_AddRefed<DOMWheelEvent> Constructor(const GlobalObject& aGlobal,
                                              const nsAString& aType,
                                              const WheelEventInit& aParam,
                                              mozilla::ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::WheelEventBinding::Wrap(aCx, aScope, this);
  }

  double DeltaX()
  {
    return static_cast<WheelEvent*>(mEvent)->deltaX;
  }

  double DeltaY()
  {
    return static_cast<WheelEvent*>(mEvent)->deltaY;
  }

  double DeltaZ()
  {
    return static_cast<WheelEvent*>(mEvent)->deltaZ;
  }

  uint32_t DeltaMode()
  {
    return static_cast<WheelEvent*>(mEvent)->deltaMode;
  }
};

} 
} 

#endif 
