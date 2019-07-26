




#include "mozilla/dom/AnimationEvent.h"
#include "mozilla/ContentEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

AnimationEvent::AnimationEvent(EventTarget* aOwner,
                               nsPresContext* aPresContext,
                               InternalAnimationEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext,
               aEvent ? aEvent : new InternalAnimationEvent(false, 0))
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}

NS_INTERFACE_MAP_BEGIN(AnimationEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMAnimationEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(AnimationEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(AnimationEvent, nsDOMEvent)


already_AddRefed<AnimationEvent>
AnimationEvent::Constructor(const GlobalObject& aGlobal,
                            const nsAString& aType,
                            const AnimationEventInit& aParam,
                            ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<AnimationEvent> e = new AnimationEvent(t, nullptr, nullptr);
  bool trusted = e->Init(t);

  aRv = e->InitEvent(aType, aParam.mBubbles, aParam.mCancelable);

  InternalAnimationEvent* internalEvent = e->mEvent->AsAnimationEvent();
  internalEvent->animationName = aParam.mAnimationName;
  internalEvent->elapsedTime = aParam.mElapsedTime;
  internalEvent->pseudoElement = aParam.mPseudoElement;

  e->SetTrusted(trusted);
  return e.forget();
}

NS_IMETHODIMP
AnimationEvent::GetAnimationName(nsAString& aAnimationName)
{
  aAnimationName = mEvent->AsAnimationEvent()->animationName;
  return NS_OK;
}

NS_IMETHODIMP
AnimationEvent::GetElapsedTime(float* aElapsedTime)
{
  *aElapsedTime = ElapsedTime();
  return NS_OK;
}

float
AnimationEvent::ElapsedTime()
{
  return mEvent->AsAnimationEvent()->elapsedTime;
}

NS_IMETHODIMP
AnimationEvent::GetPseudoElement(nsAString& aPseudoElement)
{
  aPseudoElement = mEvent->AsAnimationEvent()->pseudoElement;
  return NS_OK;
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMAnimationEvent(nsIDOMEvent** aInstancePtrResult,
                        EventTarget* aOwner,
                        nsPresContext* aPresContext,
                        InternalAnimationEvent* aEvent)
{
  AnimationEvent* it = new AnimationEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
