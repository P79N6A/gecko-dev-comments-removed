




#include "nsDOMAnimationEvent.h"
#include "nsGUIEvent.h"
#include "prtime.h"

nsDOMAnimationEvent::nsDOMAnimationEvent(mozilla::dom::EventTarget* aOwner,
                                         nsPresContext *aPresContext,
                                         nsAnimationEvent *aEvent)
  : nsDOMEvent(aOwner, aPresContext,
               aEvent ? aEvent : new nsAnimationEvent(false, 0,
                                                      EmptyString(),
                                                      0.0,
                                                      EmptyString()))
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}

nsDOMAnimationEvent::~nsDOMAnimationEvent()
{
  if (mEventIsInternal) {
    delete AnimationEvent();
    mEvent = nullptr;
  }
}

NS_INTERFACE_MAP_BEGIN(nsDOMAnimationEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMAnimationEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMAnimationEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMAnimationEvent, nsDOMEvent)


already_AddRefed<nsDOMAnimationEvent>
nsDOMAnimationEvent::Constructor(const mozilla::dom::GlobalObject& aGlobal,
                                 const nsAString& aType,
                                 const mozilla::dom::AnimationEventInit& aParam,
                                 mozilla::ErrorResult& aRv)
{
  nsCOMPtr<mozilla::dom::EventTarget> t = do_QueryInterface(aGlobal.Get());
  nsRefPtr<nsDOMAnimationEvent> e = new nsDOMAnimationEvent(t, nullptr, nullptr);
  bool trusted = e->Init(t);

  aRv = e->InitEvent(aType, aParam.mBubbles, aParam.mCancelable);

  e->AnimationEvent()->animationName = aParam.mAnimationName;
  e->AnimationEvent()->elapsedTime = aParam.mElapsedTime;
  e->AnimationEvent()->pseudoElement = aParam.mPseudoElement;

  e->SetTrusted(trusted);
  return e.forget();
}

NS_IMETHODIMP
nsDOMAnimationEvent::GetAnimationName(nsAString & aAnimationName)
{
  aAnimationName = AnimationEvent()->animationName;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAnimationEvent::GetElapsedTime(float *aElapsedTime)
{
  *aElapsedTime = ElapsedTime();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAnimationEvent::GetPseudoElement(nsAString& aPseudoElement)
{
  aPseudoElement = AnimationEvent()->pseudoElement;
  return NS_OK;
}

nsresult
NS_NewDOMAnimationEvent(nsIDOMEvent **aInstancePtrResult,
                        mozilla::dom::EventTarget* aOwner,
                        nsPresContext *aPresContext,
                        nsAnimationEvent *aEvent)
{
  nsDOMAnimationEvent* it =
    new nsDOMAnimationEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
