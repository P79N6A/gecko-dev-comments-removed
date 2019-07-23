






































#include "nsDOMTextEvent.h"
#include "nsContentUtils.h"
#include "nsPrivateTextRange.h"

nsDOMTextEvent::nsDOMTextEvent(nsPresContext* aPresContext,
                               nsTextEvent* aEvent)
  : nsDOMUIEvent(aPresContext, aEvent ? aEvent :
                 new nsTextEvent(PR_FALSE, 0, nsnull))
{
  NS_ASSERTION(mEvent->eventStructType == NS_TEXT_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }

  
  
  
  nsTextEvent *te = NS_STATIC_CAST(nsTextEvent*, mEvent);
  mText = te->theText;

  
  
  
  
  
  mTextRange = new nsPrivateTextRangeList(te->rangeCount);
  if (mTextRange) {
    PRUint16 i;

    for(i = 0; i < te->rangeCount; i++) {
      nsRefPtr<nsPrivateTextRange> tempPrivateTextRange = new
        nsPrivateTextRange(te->rangeArray[i].mStartOffset,
                           te->rangeArray[i].mEndOffset,
                           te->rangeArray[i].mRangeType);

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

NS_METHOD nsDOMTextEvent::GetInputRange(nsIPrivateTextRangeList** aInputRange)
{
  if (mEvent->message == NS_TEXT_TEXT)
  {
    *aInputRange = mTextRange;
    NS_IF_ADDREF(*aInputRange);
    return NS_OK;
  }
  *aInputRange = nsnull;
  return NS_ERROR_FAILURE;
}

NS_METHOD nsDOMTextEvent::GetEventReply(nsTextEventReply** aReply)
{
  if (mEvent->message == NS_TEXT_TEXT)
  {
     *aReply = &(NS_STATIC_CAST(nsTextEvent*, mEvent)->theReply);
     return NS_OK;
  }
  aReply = 0;
  return NS_ERROR_FAILURE;
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
