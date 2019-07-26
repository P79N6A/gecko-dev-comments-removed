




#include "nsDOMClipboardEvent.h"
#include "nsContentUtils.h"
#include "nsClientRect.h"
#include "nsDOMDataTransfer.h"
#include "DictionaryHelpers.h"
#include "nsDOMClassInfoID.h"

nsDOMClipboardEvent::nsDOMClipboardEvent(mozilla::dom::EventTarget* aOwner,
                                         nsPresContext* aPresContext,
                                         nsClipboardEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext, aEvent ? aEvent :
               new nsClipboardEvent(false, 0))
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
    delete static_cast<nsClipboardEvent*>(mEvent);
    mEvent = nullptr;
  }
}

DOMCI_DATA(ClipboardEvent, nsDOMClipboardEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMClipboardEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMClipboardEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ClipboardEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMClipboardEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMClipboardEvent, nsDOMEvent)

nsresult
nsDOMClipboardEvent::InitClipboardEvent(const nsAString & aType, bool aCanBubble, bool aCancelable,
                                        nsIDOMDataTransfer* clipboardData)
{
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsClipboardEvent* event = static_cast<nsClipboardEvent*>(mEvent);
  event->clipboardData = clipboardData;

  return NS_OK;
}

nsresult
nsDOMClipboardEvent::InitFromCtor(const nsAString& aType,
                                  JSContext* aCx, jsval* aVal)
{
  mozilla::idl::ClipboardEventInit d;
  nsresult rv = d.Init(aCx, aVal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsDOMDataTransfer> clipboardData;
  if (mEventIsInternal) {
    nsClipboardEvent* event = static_cast<nsClipboardEvent*>(mEvent);
    if (event) {
      
      
      
      clipboardData = new nsDOMDataTransfer(NS_COPY, false);
      clipboardData->SetData(d.dataType, d.data);
    }
  }

  rv = InitClipboardEvent(aType, d.bubbles, d.cancelable, clipboardData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClipboardEvent::GetClipboardData(nsIDOMDataTransfer** aClipboardData)
{
  nsClipboardEvent* event = static_cast<nsClipboardEvent*>(mEvent);

  if (!event->clipboardData) {
    if (mEventIsInternal) {
      event->clipboardData = new nsDOMDataTransfer(NS_COPY, false);
    } else {
      event->clipboardData =
        new nsDOMDataTransfer(event->message, event->message == NS_PASTE);
    }
  }

  NS_IF_ADDREF(*aClipboardData = event->clipboardData);
  return NS_OK;
}

nsresult NS_NewDOMClipboardEvent(nsIDOMEvent** aInstancePtrResult,
                                 mozilla::dom::EventTarget* aOwner,
                                 nsPresContext* aPresContext,
                                 nsClipboardEvent *aEvent)
{
  nsDOMClipboardEvent* it =
    new nsDOMClipboardEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
