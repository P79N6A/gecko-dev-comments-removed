





































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

  
  NS_DECL_NSIDOMUIEVENT

  
  NS_DECL_NSIDOMNSUIEVENT
  
  
  NS_IMETHOD GetCompositionReply(nsTextEventReply** aReply);
  NS_IMETHOD GetReconversionReply(nsReconversionEventReply** aReply);
  NS_IMETHOD GetQueryCaretRectReply(nsQueryCaretRectEventReply** aReply);
  
  
  NS_FORWARD_TO_NSDOMEVENT

protected:

  
  nsPoint GetClientPoint();
  nsPoint GetScreenPoint();
  nsPoint GetLayerPoint();
  nsPoint GetPagePoint();
  
protected:
  nsCOMPtr<nsIDOMAbstractView> mView;
  PRInt32 mDetail;
  nsPoint mClientPoint;
};

#define NS_FORWARD_TO_NSDOMUIEVENT \
  NS_FORWARD_NSIDOMUIEVENT(nsDOMUIEvent::) \
  NS_FORWARD_TO_NSDOMEVENT

#endif 
