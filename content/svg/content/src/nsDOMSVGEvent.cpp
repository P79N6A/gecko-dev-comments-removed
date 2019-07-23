




































#include "nsDOMSVGEvent.h"
#include "nsContentUtils.h"




nsDOMSVGEvent::nsDOMSVGEvent(nsPresContext* aPresContext,
                             nsEvent* aEvent)
  : nsDOMEvent(aPresContext,
               aEvent ? aEvent : new nsEvent(PR_FALSE, 0))
{
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->eventStructType = NS_SVG_EVENT;
    mEvent->time = PR_Now();
  }

  mEvent->flags |= NS_EVENT_FLAG_CANT_CANCEL;
  if (mEvent->message == NS_SVG_LOAD || mEvent->message == NS_SVG_UNLOAD) {
    mEvent->flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  }
}




NS_IMPL_ADDREF_INHERITED(nsDOMSVGEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMSVGEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMSVGEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGEvent)
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
