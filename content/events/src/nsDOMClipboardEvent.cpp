




#include "nsDOMClipboardEvent.h"
#include "nsDOMDataTransfer.h"
#include "nsIClipboard.h"
#include "mozilla/ContentEvents.h"

using namespace mozilla;

nsDOMClipboardEvent::nsDOMClipboardEvent(mozilla::dom::EventTarget* aOwner,
                                         nsPresContext* aPresContext,
                                         InternalClipboardEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext, aEvent ? aEvent :
               new InternalClipboardEvent(false, 0))
{
  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}

nsDOMClipboardEvent::~nsDOMClipboardEvent()
{
  if (mEventIsInternal && mEvent->eventStructType == NS_CLIPBOARD_EVENT) {
    delete static_cast<InternalClipboardEvent*>(mEvent);
    mEvent = nullptr;
  }
}

NS_INTERFACE_MAP_BEGIN(nsDOMClipboardEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMClipboardEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMClipboardEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMClipboardEvent, nsDOMEvent)

nsresult
nsDOMClipboardEvent::InitClipboardEvent(const nsAString & aType, bool aCanBubble, bool aCancelable,
                                        nsIDOMDataTransfer* clipboardData)
{
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  InternalClipboardEvent* event = static_cast<InternalClipboardEvent*>(mEvent);
  event->clipboardData = clipboardData;

  return NS_OK;
}

already_AddRefed<nsDOMClipboardEvent>
nsDOMClipboardEvent::Constructor(const mozilla::dom::GlobalObject& aGlobal,
                                 const nsAString& aType,
                                 const mozilla::dom::ClipboardEventInit& aParam,
                                 mozilla::ErrorResult& aRv)
{
  nsCOMPtr<mozilla::dom::EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<nsDOMClipboardEvent> e =
    new nsDOMClipboardEvent(t, nullptr, nullptr);
  bool trusted = e->Init(t);

  nsRefPtr<nsDOMDataTransfer> clipboardData;
  if (e->mEventIsInternal) {
    InternalClipboardEvent* event =
      static_cast<InternalClipboardEvent*>(e->mEvent);
    if (event) {
      
      
      
      clipboardData = new nsDOMDataTransfer(NS_COPY, false, -1);
      clipboardData->SetData(aParam.mDataType, aParam.mData);
    }
  }

  aRv = e->InitClipboardEvent(aType, aParam.mBubbles, aParam.mCancelable,
                              clipboardData);
  e->SetTrusted(trusted);
  return e.forget();
}

NS_IMETHODIMP
nsDOMClipboardEvent::GetClipboardData(nsIDOMDataTransfer** aClipboardData)
{
  NS_IF_ADDREF(*aClipboardData = GetClipboardData());
  return NS_OK;
}

nsIDOMDataTransfer*
nsDOMClipboardEvent::GetClipboardData()
{
  InternalClipboardEvent* event = static_cast<InternalClipboardEvent*>(mEvent);

  if (!event->clipboardData) {
    if (mEventIsInternal) {
      event->clipboardData = new nsDOMDataTransfer(NS_COPY, false, -1);
    } else {
      event->clipboardData =
        new nsDOMDataTransfer(event->message, event->message == NS_PASTE, nsIClipboard::kGlobalClipboard);
    }
  }

  return event->clipboardData;
}

nsresult NS_NewDOMClipboardEvent(nsIDOMEvent** aInstancePtrResult,
                                 mozilla::dom::EventTarget* aOwner,
                                 nsPresContext* aPresContext,
                                 InternalClipboardEvent* aEvent)
{
  nsDOMClipboardEvent* it =
    new nsDOMClipboardEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
