




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

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return BeforeUnloadEventBinding::Wrap(aCx, this);
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
