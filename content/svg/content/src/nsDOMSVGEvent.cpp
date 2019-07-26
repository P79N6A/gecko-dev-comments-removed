




#include "nsDOMClassInfoID.h"
#include "nsDOMSVGEvent.h"




nsDOMSVGEvent::nsDOMSVGEvent(nsPresContext* aPresContext,
                             nsEvent* aEvent)
  : nsDOMEvent(aPresContext,
               aEvent ? aEvent : new nsEvent(false, 0))
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->eventStructType = NS_SVG_EVENT;
    mEvent->time = PR_Now();
  }

  mEvent->mFlags.mCancelable = false;
  mEvent->mFlags.mBubbles =
    (mEvent->message != NS_SVG_LOAD && mEvent->message != NS_SVG_UNLOAD);
}




NS_IMPL_ADDREF_INHERITED(nsDOMSVGEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMSVGEvent, nsDOMEvent)

DOMCI_DATA(SVGEvent, nsDOMSVGEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMSVGEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)





nsresult
NS_NewDOMSVGEvent(nsIDOMEvent** aInstancePtrResult,
                  nsPresContext* aPresContext,
                  nsEvent *aEvent)
{
  nsDOMSVGEvent* it = new nsDOMSVGEvent(aPresContext, aEvent);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  return CallQueryInterface(it, aInstancePtrResult);
}
