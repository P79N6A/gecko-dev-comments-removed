





#include "nsDOMTextEvent.h"
#include "nsPrivateTextRange.h"
#include "prtime.h"
#include "mozilla/TextEvents.h"

using namespace mozilla;
using namespace mozilla::dom;

nsDOMTextEvent::nsDOMTextEvent(mozilla::dom::EventTarget* aOwner,
                               nsPresContext* aPresContext,
                               WidgetTextEvent* aEvent)
  : UIEvent(aOwner, aPresContext,
            aEvent ? aEvent : new WidgetTextEvent(false, 0, nullptr))
{
  NS_ASSERTION(mEvent->eventStructType == NS_TEXT_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }

  
  
  
  WidgetTextEvent* te = mEvent->AsTextEvent();
  mText = te->theText;

  
  
  
  
  
  mTextRange = new nsPrivateTextRangeList(te->RangeCount());
  if (mTextRange) {
    uint16_t i;

    for(i = 0; i < te->rangeCount; i++) {
      nsRefPtr<nsPrivateTextRange> tempPrivateTextRange = new
        nsPrivateTextRange(te->mRanges->ElementAt(i));

      if (tempPrivateTextRange) {
        mTextRange->AppendTextRange(tempPrivateTextRange);
      }
    }
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMTextEvent, UIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMTextEvent, UIEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMTextEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateTextEvent)
NS_INTERFACE_MAP_END_INHERITING(UIEvent)

NS_METHOD nsDOMTextEvent::GetText(nsString& aText)
{
  aText = mText;
  return NS_OK;
}

NS_METHOD_(already_AddRefed<nsIPrivateTextRangeList>) nsDOMTextEvent::GetInputRange()
{
  if (mEvent->message == NS_TEXT_TEXT) {
    nsRefPtr<nsPrivateTextRangeList> textRange = mTextRange;
    return textRange.forget();
  }
  return nullptr;
}

nsresult NS_NewDOMTextEvent(nsIDOMEvent** aInstancePtrResult,
                            mozilla::dom::EventTarget* aOwner,
                            nsPresContext* aPresContext,
                            WidgetTextEvent* aEvent)
{
  nsDOMTextEvent* it = new nsDOMTextEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
