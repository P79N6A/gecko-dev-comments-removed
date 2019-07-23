





































#ifndef nsDOMUIEvent_h__
#define nsDOMUIEvent_h__

#include "nsIDOMUIEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMAbstractView.h"
#include "nsIPrivateCompositionEvent.h"
#include "nsDOMEvent.h"

class nsDOMUIEvent : public nsIDOMUIEvent,
                     public nsIDOMNSUIEvent,
                     public nsIPrivateCompositionEvent,
                     public nsDOMEvent
{
public:
  nsDOMUIEvent(nsPresContext* aPresContext, nsGUIEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMUIEvent, nsDOMEvent)

  
  NS_DECL_NSIDOMUIEVENT

  
  NS_DECL_NSIDOMNSUIEVENT

  
  NS_IMETHOD DuplicatePrivateData();
  
  
  NS_IMETHOD GetCompositionReply(nsTextEventReply** aReply);
  
  
  NS_FORWARD_TO_NSDOMEVENT

protected:

  
  nsIntPoint GetClientPoint();
  nsIntPoint GetScreenPoint();
  nsIntPoint GetLayerPoint();
  nsIntPoint GetPagePoint();
  
protected:
  nsCOMPtr<nsIDOMAbstractView> mView;
  PRInt32 mDetail;
  nsIntPoint mClientPoint;
  
  nsIntPoint mLayerPoint;
  nsIntPoint mPagePoint;
};

#define NS_FORWARD_TO_NSDOMUIEVENT \
  NS_FORWARD_NSIDOMUIEVENT(nsDOMUIEvent::) \
  NS_FORWARD_TO_NSDOMEVENT

#endif 
