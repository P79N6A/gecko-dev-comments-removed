



#ifndef mozilla_dom_AnimationEvent_h_
#define mozilla_dom_AnimationEvent_h_

#include "nsDOMEvent.h"
#include "nsIDOMAnimationEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/AnimationEventBinding.h"

class nsAString;

namespace mozilla {
namespace dom {

class AnimationEvent : public nsDOMEvent,
                       public nsIDOMAnimationEvent
{
public:
  AnimationEvent(EventTarget* aOwner,
                 nsPresContext* aPresContext,
                 InternalAnimationEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIDOMANIMATIONEVENT

  static already_AddRefed<AnimationEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const AnimationEventInit& aParam,
              ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return AnimationEventBinding::Wrap(aCx, aScope, this);
  }

  
  
  

  float ElapsedTime();
};

} 
} 

#endif 
