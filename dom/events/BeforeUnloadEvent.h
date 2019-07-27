




#ifndef mozilla_dom_BeforeUnloadEvent_h_
#define mozilla_dom_BeforeUnloadEvent_h_

#include "mozilla/dom/BeforeUnloadEventBinding.h"
#include "mozilla/dom/Event.h"
#include "nsIDOMBeforeUnloadEvent.h"

namespace mozilla {
namespace dom {

class BeforeUnloadEvent : public Event,
                          public nsIDOMBeforeUnloadEvent
{
public:
  BeforeUnloadEvent(EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    WidgetEvent* aEvent)
    : Event(aOwner, aPresContext, aEvent)
  {
  }

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE
  {
    return BeforeUnloadEventBinding::Wrap(aCx, this, aGivenProto);
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_EVENT

  
  NS_DECL_NSIDOMBEFOREUNLOADEVENT

protected:
  ~BeforeUnloadEvent() {}

  nsString mText;
};

} 
} 

#endif 
