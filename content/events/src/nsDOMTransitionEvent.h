



#ifndef nsDOMTransitionEvent_h_
#define nsDOMTransitionEvent_h_

#include "nsDOMEvent.h"
#include "nsIDOMTransitionEvent.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/dom/TransitionEventBinding.h"

class nsAString;

class nsDOMTransitionEvent : public nsDOMEvent,
                             public nsIDOMTransitionEvent
{
public:
  nsDOMTransitionEvent(mozilla::dom::EventTarget* aOwner,
                       nsPresContext *aPresContext,
                       mozilla::InternalTransitionEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIDOMTRANSITIONEVENT

  static already_AddRefed<nsDOMTransitionEvent>
  Constructor(const mozilla::dom::GlobalObject& aGlobal,
              const nsAString& aType,
              const mozilla::dom::TransitionEventInit& aParam,
              mozilla::ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx,
			       JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::TransitionEventBinding::Wrap(aCx, aScope, this);
  }

  
  
  

  float ElapsedTime()
  {
    return TransitionEvent()->elapsedTime;
  }

private:
  mozilla::InternalTransitionEvent* TransitionEvent() {
    NS_ABORT_IF_FALSE(mEvent->eventStructType == NS_TRANSITION_EVENT,
                      "unexpected struct type");
    return static_cast<mozilla::InternalTransitionEvent*>(mEvent);
  }
};

#endif 
