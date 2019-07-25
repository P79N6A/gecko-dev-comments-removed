




































#include "nsDOMTimeEvent.h"
#include "nsGUIEvent.h"
#include "nsPresContext.h"
#include "nsIInterfaceRequestorUtils.h"

nsDOMTimeEvent::nsDOMTimeEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent : new nsUIEvent(PR_FALSE, 0, 0)),
    mDetail(0)
{
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  } else {
    mEventIsInternal = PR_TRUE;
    mEvent->eventStructType = NS_SMIL_TIME_EVENT;
  }

  if (mEvent->eventStructType == NS_SMIL_TIME_EVENT) {
    nsUIEvent* event = static_cast<nsUIEvent*>(mEvent);
    mDetail = event->detail;
  }

  mEvent->flags |= NS_EVENT_FLAG_CANT_BUBBLE |
                   NS_EVENT_FLAG_CANT_CANCEL;

  if (mPresContext) {
    nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
    if (container) {
      nsCOMPtr<nsIDOMWindowInternal> window = do_GetInterface(container);
      if (window) {
        mView = do_QueryInterface(window);
      }
    }
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMTimeEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMTimeEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mView)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMTimeEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mView)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsDOMTimeEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMTimeEvent, nsDOMEvent)

DOMCI_DATA(TimeEvent, nsDOMTimeEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMTimeEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTimeEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TimeEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMTimeEvent::GetView(nsIDOMWindow** aView)
{
  *aView = mView;
  NS_IF_ADDREF(*aView);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMTimeEvent::GetDetail(PRInt32* aDetail)
{
  *aDetail = mDetail;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMTimeEvent::InitTimeEvent(const nsAString& aTypeArg,
                              nsIDOMWindow* aViewArg,
                              PRInt32 aDetailArg)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, PR_FALSE ,
                                                PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);

  mDetail = aDetailArg;
  mView = aViewArg;

  return NS_OK;
}

nsresult NS_NewDOMTimeEvent(nsIDOMEvent** aInstancePtrResult,
                            nsPresContext* aPresContext,
                            nsEvent* aEvent)
{
  nsDOMTimeEvent* it = new nsDOMTimeEvent(aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
