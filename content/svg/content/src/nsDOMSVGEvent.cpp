




#include "nsDOMClassInfoID.h"
#include "nsDOMSVGEvent.h"




nsDOMSVGEvent::nsDOMSVGEvent(mozilla::dom::EventTarget* aOwner,
                             nsPresContext* aPresContext,
                             nsEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext,
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
                  mozilla::dom::EventTarget* aOwner,
                  nsPresContext* aPresContext,
                  nsEvent *aEvent)
{
  nsDOMSVGEvent* it = new nsDOMSVGEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
