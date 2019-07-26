





#ifndef mozilla_dom_DOMWheelEvent_h__
#define mozilla_dom_DOMWheelEvent_h__

#include "nsIDOMWheelEvent.h"
#include "nsDOMMouseEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/WheelEventBinding.h"

namespace mozilla {
namespace dom {

class DOMWheelEvent : public nsDOMMouseEvent,
                      public nsIDOMWheelEvent
{
public:
  DOMWheelEvent(mozilla::dom::EventTarget* aOwner,
                nsPresContext* aPresContext,
                WidgetWheelEvent* aWheelEvent);

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

  double DeltaX();
  double DeltaY();
  double DeltaZ();
  uint32_t DeltaMode();
};

} 
} 

#endif 
