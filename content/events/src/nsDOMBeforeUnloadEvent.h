




#ifndef nsDOMBeforeUnloadEvent_h__
#define nsDOMBeforeUnloadEvent_h__

#include "nsIDOMBeforeUnloadEvent.h"
#include "nsDOMEvent.h"
#include "mozilla/dom/BeforeUnloadEventBinding.h"

class nsDOMBeforeUnloadEvent : public nsDOMEvent,
                               public nsIDOMBeforeUnloadEvent
{
public:
  nsDOMBeforeUnloadEvent(mozilla::dom::EventTarget* aOwner,
                         nsPresContext* aPresContext,
                         mozilla::WidgetEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext, aEvent)
  {
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::BeforeUnloadEventBinding::Wrap(aCx, aScope, this);
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMBEFOREUNLOADEVENT
protected:
  nsString mText;
};

#endif 
