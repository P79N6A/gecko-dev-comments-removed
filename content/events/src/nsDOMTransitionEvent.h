



#ifndef nsDOMTransitionEvent_h_
#define nsDOMTransitionEvent_h_

#include "nsDOMEvent.h"
#include "nsIDOMTransitionEvent.h"
#include "nsString.h"
#include "mozilla/dom/TransitionEventBinding.h"

class nsTransitionEvent;

class nsDOMTransitionEvent : public nsDOMEvent,
                             public nsIDOMTransitionEvent
{
public:
  nsDOMTransitionEvent(mozilla::dom::EventTarget* aOwner,
                       nsPresContext *aPresContext,
                       nsTransitionEvent *aEvent);
  ~nsDOMTransitionEvent();

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

  void InitTransitionEvent(const nsAString& aType,
                           bool aCanBubble,
                           bool aCancelable,
                           const nsAString& aPropertyName,
                           float aElapsedTime,
                           const mozilla::dom::Optional<nsAString>& aPseudoElement,
                           mozilla::ErrorResult& aRv)
  {
    aRv = InitTransitionEvent(aType, aCanBubble, aCancelable, aPropertyName,
                              aElapsedTime, aPseudoElement.WasPassed() ?
                                aPseudoElement.Value() : EmptyString());
  }
private:
  nsTransitionEvent* TransitionEvent() {
    NS_ABORT_IF_FALSE(mEvent->eventStructType == NS_TRANSITION_EVENT,
                      "unexpected struct type");
    return static_cast<nsTransitionEvent*>(mEvent);
  }
};

#endif 
