





#ifndef mozilla_dom_DOMWheelEvent_h__
#define mozilla_dom_DOMWheelEvent_h__

#include "nsIDOMWheelEvent.h"
#include "nsDOMMouseEvent.h"

namespace mozilla {
namespace dom {

class DOMWheelEvent : public nsDOMMouseEvent,
                      public nsIDOMWheelEvent
{
public:
  DOMWheelEvent(mozilla::dom::EventTarget* aOwner,
                nsPresContext* aPresContext,
                widget::WheelEvent* aWheelEvent);
  virtual ~DOMWheelEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMWHEELEVENT
  
  
  NS_FORWARD_TO_NSDOMMOUSEEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, JS::Value* aVal);
};

} 
} 

#endif 
