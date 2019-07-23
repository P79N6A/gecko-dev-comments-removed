




































#ifndef nsDOMDragEvent_h__
#define nsDOMDragEvent_h__

#include "nsIDOMDragEvent.h"
#include "nsDOMMouseEvent.h"
#include "nsIDOMDataTransfer.h"

class nsIContent;
class nsEvent;

class nsDOMDragEvent : public nsDOMMouseEvent,
                       public nsIDOMDragEvent
{
public:
  nsDOMDragEvent(nsPresContext* aPresContext, nsInputEvent* aEvent);
  virtual ~nsDOMDragEvent();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMDRAGEVENT
  
  NS_FORWARD_TO_NSDOMMOUSEEVENT
};

nsresult NS_NewDOMDragEvent(nsIDOMEvent** aInstancePtrResult,
                            nsPresContext* aPresContext,
                            nsDragEvent* aEvent);

#endif
