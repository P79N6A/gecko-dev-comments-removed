




































#ifndef nsPIDOMEventTarget_h_
#define nsPIDOMEventTarget_h_

#include "nsISupports.h"
#include "nsEvent.h"

class nsIDOMEvent;
class nsPresContext;
class nsEventChainPreVisitor;
class nsEventChainPostVisitor;
class nsIEventListenerManager;
class nsIDOMEventListener;
class nsIDOMEventGroup;


#define NS_PIDOMEVENTTARGET_IID \
  { 0x44a6597b, 0x9fc3, 0x4a8d, \
    { 0xb7, 0xa4, 0xd9, 0x00, 0x9a, 0xbf, 0x9d, 0x15 } }

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

  






  virtual nsresult GetListenerManager(PRBool aCreateIfNotFound,
                                      nsIEventListenerManager** aResult) = 0;

  


  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID) = 0;
  


  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID) = 0;
  
  


  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMEventTarget, NS_PIDOMEVENTTARGET_IID)

#endif 
