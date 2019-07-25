





































#ifndef nsDOMUIEvent_h
#define nsDOMUIEvent_h

#include "nsIDOMUIEvent.h"
#include "nsDOMEvent.h"

class nsDOMUIEvent : public nsDOMEvent,
                     public nsIDOMUIEvent
{
public:
  nsDOMUIEvent(nsPresContext* aPresContext, nsGUIEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMUIEvent, nsDOMEvent)

  
  NS_DECL_NSIDOMUIEVENT

  
  NS_IMETHOD DuplicatePrivateData();
  virtual void Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType);
  virtual bool Deserialize(const IPC::Message* aMsg, void** aIter);
  
  
  NS_FORWARD_TO_NSDOMEVENT

  NS_FORWARD_NSIDOMNSEVENT(nsDOMEvent::)

protected:
  
  nsIntPoint GetClientPoint();
  nsIntPoint GetScreenPoint();
  nsIntPoint GetLayerPoint();
  nsIntPoint GetPagePoint();

  
  virtual nsresult Which(PRUint32* aWhich)
  {
    NS_ENSURE_ARG_POINTER(aWhich);
    
    *aWhich = 0;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMWindow> mView;
  PRInt32 mDetail;
  nsIntPoint mClientPoint;
  
  nsIntPoint mLayerPoint;
  nsIntPoint mPagePoint;
};

#define NS_FORWARD_TO_NSDOMUIEVENT \
  NS_FORWARD_NSIDOMUIEVENT(nsDOMUIEvent::) \
  NS_FORWARD_TO_NSDOMEVENT

#endif 
