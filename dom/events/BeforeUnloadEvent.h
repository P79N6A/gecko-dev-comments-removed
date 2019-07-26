




#ifndef mozilla_dom_BeforeUnloadEvent_h_
#define mozilla_dom_BeforeUnloadEvent_h_

#include "nsIDOMBeforeUnloadEvent.h"
#include "nsDOMEvent.h"
#include "mozilla/dom/BeforeUnloadEventBinding.h"

namespace mozilla {
namespace dom {

class BeforeUnloadEvent : public nsDOMEvent,
                          public nsIDOMBeforeUnloadEvent
{
public:
  BeforeUnloadEvent(EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    WidgetEvent* aEvent)
    : nsDOMEvent(aOwner, aPresContext, aEvent)
  {
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return BeforeUnloadEventBinding::Wrap(aCx, aScope, this);
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMBEFOREUNLOADEVENT

protected:
  nsString mText;
};

} 
} 

#endif 
