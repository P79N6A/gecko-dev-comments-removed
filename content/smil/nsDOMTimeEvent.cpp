




#include "nsDOMTimeEvent.h"
#include "nsPresContext.h"
#include "nsIInterfaceRequestorUtils.h"
#include "mozilla/BasicEvents.h"

using namespace mozilla;

nsDOMTimeEvent::nsDOMTimeEvent(mozilla::dom::EventTarget* aOwner,
                               nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext,
               aEvent ? aEvent : new InternalUIEvent(false, 0, 0)),
    mDetail(0)
{
  SetIsDOMBinding();
  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->eventStructType = NS_SMIL_TIME_EVENT;
  }

  if (mEvent->eventStructType == NS_SMIL_TIME_EVENT) {
    InternalUIEvent* event = static_cast<InternalUIEvent*>(mEvent);
    mDetail = event->detail;
  }

  mEvent->mFlags.mBubbles = false;
  mEvent->mFlags.mCancelable = false;

  if (mPresContext) {
    nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
    if (container) {
      nsCOMPtr<nsIDOMWindow> window = do_GetInterface(container);
      if (window) {
        mView = do_QueryInterface(window);
      }
    }
  }
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(nsDOMTimeEvent, nsDOMEvent,
                                     mView)

NS_IMPL_ADDREF_INHERITED(nsDOMTimeEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMTimeEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMTimeEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTimeEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMTimeEvent::GetView(nsIDOMWindow** aView)
{
  *aView = mView;
  NS_IF_ADDREF(*aView);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMTimeEvent::GetDetail(int32_t* aDetail)
{
  *aDetail = mDetail;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMTimeEvent::InitTimeEvent(const nsAString& aTypeArg,
                              nsIDOMWindow* aViewArg,
                              int32_t aDetailArg)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, false ,
                                                false );
  NS_ENSURE_SUCCESS(rv, rv);

  mDetail = aDetailArg;
  mView = aViewArg;

  return NS_OK;
}

nsresult NS_NewDOMTimeEvent(nsIDOMEvent** aInstancePtrResult,
                            mozilla::dom::EventTarget* aOwner,
                            nsPresContext* aPresContext,
                            nsEvent* aEvent)
{
  nsDOMTimeEvent* it = new nsDOMTimeEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
