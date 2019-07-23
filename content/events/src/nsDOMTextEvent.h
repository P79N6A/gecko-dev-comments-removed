





































#ifndef nsDOMTextEvent_h__
#define nsDOMTextEvent_h__

#include "nsDOMUIEvent.h"
#include "nsIPrivateTextEvent.h"
#include "nsPrivateTextRange.h"

class nsDOMTextEvent : public nsDOMUIEvent,
                       public nsIPrivateTextEvent
{
public:
  nsDOMTextEvent(nsPresContext* aPresContext, nsTextEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMUIEVENT

  
  NS_IMETHOD GetText(nsString& aText);
  NS_IMETHOD GetInputRange(nsIPrivateTextRangeList** aInputRange);
  NS_IMETHOD GetEventReply(nsTextEventReply** aReply);
  
protected:
  nsString mText;
  nsRefPtr<nsPrivateTextRangeList> mTextRange;
};

#endif 
