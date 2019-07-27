



#ifndef mozilla_dom_AnimationEvent_h_
#define mozilla_dom_AnimationEvent_h_

#include "mozilla/EventForwards.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/AnimationEventBinding.h"
#include "nsIDOMAnimationEvent.h"

class nsAString;

namespace mozilla {
namespace dom {

class AnimationEvent : public Event,
                       public nsIDOMAnimationEvent
{
public:
  AnimationEvent(EventTarget* aOwner,
                 nsPresContext* aPresContext,
                 InternalAnimationEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_EVENT
  NS_DECL_NSIDOMANIMATIONEVENT

  static already_AddRefed<AnimationEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const AnimationEventInit& aParam,
              ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return AnimationEventBinding::Wrap(aCx, this);
  }

  
  
  

  float ElapsedTime();

protected:
  ~AnimationEvent() {}
};

} 
} 

#endif 
