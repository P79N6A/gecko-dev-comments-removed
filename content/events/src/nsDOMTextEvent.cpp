






































#include "nsDOMTextEvent.h"
#include "nsContentUtils.h"
#include "nsPrivateTextRange.h"

nsDOMTextEvent::nsDOMTextEvent(nsPresContext* aPresContext,
                               nsTextEvent* aEvent)
  : nsDOMUIEvent(aPresContext, aEvent ? aEvent :
                 new nsTextEvent(false, 0, nsnull))
{
  NS_ASSERTION(mEvent->eventStructType == NS_TEXT_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }

  
  
  
  nsTextEvent *te = static_cast<nsTextEvent*>(mEvent);
  mText = te->theText;

  
  
  
  
  
  mTextRange = new nsPrivateTextRangeList(te->rangeCount);
  if (mTextRange) {
    PRUint16 i;

    for(i = 0; i < te->rangeCount; i++) {
      nsRefPtr<nsPrivateTextRange> tempPrivateTextRange = new
        nsPrivateTextRange(te->rangeArray[i]);

      if (tempPrivateTextRange) {
        mTextRange->AppendTextRange(tempPrivateTextRange);
      }
    }
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMTextEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMTextEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMTextEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateTextEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)

NS_METHOD nsDOMTextEvent::GetText(nsString& aText)
{
  aText = mText;
  return NS_OK;
}

NS_METHOD_(already_AddRefed<nsIPrivateTextRangeList>) nsDOMTextEvent::GetInputRange()
{
  if (mEvent->message == NS_TEXT_TEXT) {
    nsRefPtr<nsPrivateTextRangeList> textRange = mTextRange;
    nsPrivateTextRangeList *textRangePtr = nsnull;
    textRange.swap(textRangePtr);
    return textRangePtr;
  }
  return nsnull;
}

nsresult NS_NewDOMTextEvent(nsIDOMEvent** aInstancePtrResult,
                            nsPresContext* aPresContext,
                            nsTextEvent *aEvent)
{
  nsDOMTextEvent* it = new nsDOMTextEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
