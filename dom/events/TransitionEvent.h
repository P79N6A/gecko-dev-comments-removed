



#ifndef mozilla_dom_TransitionEvent_h_
#define mozilla_dom_TransitionEvent_h_

#include "mozilla/EventForwards.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/TransitionEventBinding.h"
#include "nsIDOMTransitionEvent.h"

class nsAString;

namespace mozilla {
namespace dom {

class TransitionEvent : public Event,
                        public nsIDOMTransitionEvent
{
public:
  TransitionEvent(EventTarget* aOwner,
                  nsPresContext* aPresContext,
                  InternalTransitionEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_EVENT
  NS_DECL_NSIDOMTRANSITIONEVENT

  static already_AddRefed<TransitionEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const TransitionEventInit& aParam,
              ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return TransitionEventBinding::Wrap(aCx, this);
  }

  
  
  

  float ElapsedTime();

protected:
  ~TransitionEvent() {}
};

} 
} 

#endif 
