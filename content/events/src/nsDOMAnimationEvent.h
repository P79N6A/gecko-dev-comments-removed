



#ifndef nsDOMAnimationEvent_h_
#define nsDOMAnimationEvent_h_

#include "nsDOMEvent.h"
#include "nsIDOMAnimationEvent.h"
#include "nsString.h"
#include "mozilla/dom/AnimationEventBinding.h"

class nsAnimationEvent;

class nsDOMAnimationEvent : public nsDOMEvent,
                            public nsIDOMAnimationEvent
{
public:
  nsDOMAnimationEvent(mozilla::dom::EventTarget* aOwner,
                      nsPresContext *aPresContext,
                      nsAnimationEvent *aEvent);
  ~nsDOMAnimationEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIDOMANIMATIONEVENT

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope)
  {
    return mozilla::dom::AnimationEventBinding::Wrap(aCx, aScope, this);
  }

  
  

  float ElapsedTime()
  {
    return AnimationEvent()->elapsedTime;
  }

  void InitAnimationEvent(const nsAString& aType,
                          bool aCanBubble,
                          bool aCancelable,
                          const nsAString& aAnimationName,
                          float aElapsedTime,
                          mozilla::ErrorResult& aRv)
  {
    aRv = InitAnimationEvent(aType, aCanBubble, aCancelable, aAnimationName,
                             aElapsedTime);
  }
private:
  nsAnimationEvent* AnimationEvent() {
    NS_ABORT_IF_FALSE(mEvent->eventStructType == NS_ANIMATION_EVENT,
                      "unexpected struct type");
    return static_cast<nsAnimationEvent*>(mEvent);
  }
};

#endif 
