




































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
{ 0x764756cd, 0x8af2, 0x4a25,   \
  { 0x91, 0x9d, 0xca, 0x95, 0x75, 0x9a, 0x1b, 0xe1 } }

class nsPIDOMEventTarget : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMEVENTTARGET_IID)

  














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
