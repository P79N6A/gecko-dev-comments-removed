




































#ifndef nsPIDOMEventTarget_h_
#define nsPIDOMEventTarget_h_

#include "nsISupports.h"
#include "nsEvent.h"

class nsIDOMEvent;
class nsPresContext;
class nsEventChainPreVisitor;
class nsEventChainPostVisitor;
class nsIEventListenerManager;


#define NS_PIDOMEVENTTARGET_IID \
{ 0x360fa72e, 0xc709, 0x42cc, \
  { 0x92, 0x85, 0x1f, 0x75, 0x5e, 0xc9, 0x03, 0x76 } }

class nsPIDOMEventTarget : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMEVENTTARGET_IID)

  





   virtual nsPIDOMEventTarget* GetTargetForDOMEvent() { return this; }

  





   virtual nsPIDOMEventTarget* GetTargetForEventTargetChain() { return this; }

  














  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor) = 0;

  







  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor) = 0;

  
















  virtual nsresult DispatchDOMEvent(nsEvent* aEvent,
                                    nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus) = 0;

  






  NS_IMETHOD GetListenerManager(PRBool aCreateIfNotFound,
                                nsIEventListenerManager** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMEventTarget, NS_PIDOMEVENTTARGET_IID)

#endif 
