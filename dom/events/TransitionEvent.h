



#ifndef mozilla_dom_TransitionEvent_h_
#define mozilla_dom_TransitionEvent_h_

#include "nsDOMEvent.h"
#include "nsIDOMTransitionEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/TransitionEventBinding.h"

class nsAString;

namespace mozilla {
namespace dom {

class TransitionEvent : public nsDOMEvent,
                        public nsIDOMTransitionEvent
{
public:
  TransitionEvent(EventTarget* aOwner,
                  nsPresContext* aPresContext,
                  InternalTransitionEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIDOMTRANSITIONEVENT

  static already_AddRefed<TransitionEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const TransitionEventInit& aParam,
              ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return TransitionEventBinding::Wrap(aCx, aScope, this);
  }

  
  
  

  float ElapsedTime();
};

} 
} 

#endif 
