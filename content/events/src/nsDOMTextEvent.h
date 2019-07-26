




#ifndef nsDOMTextEvent_h__
#define nsDOMTextEvent_h__

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsDOMUIEvent.h"
#include "nsIPrivateTextEvent.h"
#include "nsPrivateTextRange.h"

class nsDOMTextEvent : public nsDOMUIEvent,
                       public nsIPrivateTextEvent
{
public:
  nsDOMTextEvent(mozilla::dom::EventTarget* aOwner,
                 nsPresContext* aPresContext,
                 mozilla::WidgetTextEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMUIEVENT

  
  NS_IMETHOD GetText(nsString& aText) MOZ_OVERRIDE;
  NS_IMETHOD_(already_AddRefed<nsIPrivateTextRangeList>) GetInputRange() MOZ_OVERRIDE;
  
protected:
  nsString mText;
  nsRefPtr<nsPrivateTextRangeList> mTextRange;
};

#endif 
