




































#include "nsDOMAnimationEvent.h"
#include "nsGUIEvent.h"
#include "nsDOMClassInfoID.h"
#include "nsIClassInfo.h"
#include "nsIXPCScriptable.h"

nsDOMAnimationEvent::nsDOMAnimationEvent(nsPresContext *aPresContext,
                                         nsAnimationEvent *aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent
                                    : new nsAnimationEvent(PR_FALSE, 0,
                                                           EmptyString(),
                                                           0.0))
{
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }
}

nsDOMAnimationEvent::~nsDOMAnimationEvent()
{
  if (mEventIsInternal) {
    delete AnimationEvent();
    mEvent = nsnull;
  }
}

DOMCI_DATA(AnimationEvent, nsDOMAnimationEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMAnimationEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMAnimationEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(AnimationEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMAnimationEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMAnimationEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMAnimationEvent::GetAnimationName(nsAString & aAnimationName)
{
  aAnimationName = AnimationEvent()->animationName;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAnimationEvent::GetElapsedTime(float *aElapsedTime)
{
  *aElapsedTime = AnimationEvent()->elapsedTime;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAnimationEvent::InitAnimationEvent(const nsAString & typeArg,
                                        PRBool canBubbleArg,
                                        PRBool cancelableArg,
                                        const nsAString & animationNameArg,
                                        float elapsedTimeArg)
{
  nsresult rv = nsDOMEvent::InitEvent(typeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  AnimationEvent()->animationName = animationNameArg;
  AnimationEvent()->elapsedTime = elapsedTimeArg;

  return NS_OK;
}

nsresult
NS_NewDOMAnimationEvent(nsIDOMEvent **aInstancePtrResult,
                        nsPresContext *aPresContext,
                        nsAnimationEvent *aEvent)
{
  nsDOMAnimationEvent *it = new nsDOMAnimationEvent(aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
