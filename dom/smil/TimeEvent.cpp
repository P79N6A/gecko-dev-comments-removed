




#include "mozilla/dom/TimeEvent.h"
#include "mozilla/BasicEvents.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsPresContext.h"

namespace mozilla {
namespace dom {

TimeEvent::TimeEvent(EventTarget* aOwner,
                     nsPresContext* aPresContext,
                     WidgetEvent* aEvent)
  : Event(aOwner, aPresContext,
          aEvent ? aEvent : new InternalUIEvent(false, 0))
  , mDetail(0)
{
  SetIsDOMBinding();
  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->eventStructType = NS_SMIL_TIME_EVENT;
  }

  if (mEvent->eventStructType == NS_SMIL_TIME_EVENT) {
    mDetail = mEvent->AsUIEvent()->detail;
  }

  mEvent->mFlags.mBubbles = false;
  mEvent->mFlags.mCancelable = false;

  if (mPresContext) {
    nsCOMPtr<nsIDocShell> docShell = mPresContext->GetDocShell();
    if (docShell) {
      mView = docShell->GetWindow();
    }
  }
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(TimeEvent, Event,
                                   mView)

NS_IMPL_ADDREF_INHERITED(TimeEvent, Event)
NS_IMPL_RELEASE_INHERITED(TimeEvent, Event)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TimeEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTimeEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)

NS_IMETHODIMP
TimeEvent::GetView(nsIDOMWindow** aView)
{
  *aView = mView;
  NS_IF_ADDREF(*aView);
  return NS_OK;
}

NS_IMETHODIMP
TimeEvent::GetDetail(int32_t* aDetail)
{
  *aDetail = mDetail;
  return NS_OK;
}

NS_IMETHODIMP
TimeEvent::InitTimeEvent(const nsAString& aTypeArg,
                         nsIDOMWindow* aViewArg,
                         int32_t aDetailArg)
{
  nsresult rv = Event::InitEvent(aTypeArg, false ,
                                           false );
  NS_ENSURE_SUCCESS(rv, rv);

  mDetail = aDetailArg;
  mView = aViewArg;

  return NS_OK;
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMTimeEvent(nsIDOMEvent** aInstancePtrResult,
                   EventTarget* aOwner,
                   nsPresContext* aPresContext,
                   WidgetEvent* aEvent)
{
  TimeEvent* it = new TimeEvent(aOwner, aPresContext, aEvent);
  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
