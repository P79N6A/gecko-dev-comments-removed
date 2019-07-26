




#ifndef nsDOMTextEvent_h__
#define nsDOMTextEvent_h__

#include "mozilla/dom/UIEvent.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsIPrivateTextEvent.h"
#include "nsPrivateTextRange.h"

class nsDOMTextEvent : public mozilla::dom::UIEvent
{
  typedef mozilla::dom::UIEvent UIEvent;

public:
  nsDOMTextEvent(mozilla::dom::EventTarget* aOwner,
                 nsPresContext* aPresContext,
                 mozilla::WidgetTextEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_UIEVENT
};

#endif
