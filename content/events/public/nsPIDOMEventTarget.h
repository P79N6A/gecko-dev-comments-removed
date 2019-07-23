




































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
class nsIScriptContext;


#define NS_PIDOMEVENTTARGET_IID \
  { 0x25982813, 0xaf2e, 0x4ab6, \
    { 0xb5, 0x12, 0xe6, 0xc6, 0xad, 0xa6, 0xd0, 0xec } }

class nsPIDOMEventTarget : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMEVENTTARGET_IID)

  





   virtual nsPIDOMEventTarget* GetTargetForDOMEvent() { return this; }

  





   virtual nsPIDOMEventTarget* GetTargetForEventTargetChain() { return this; }

  














  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor) = 0;

  



  virtual nsresult WillHandleEvent(nsEventChainPostVisitor& aVisitor)
  {
    return NS_OK;
  }

  







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

  



  virtual nsresult GetContextForEventHandlers(nsIScriptContext** aContext) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMEventTarget, NS_PIDOMEVENTTARGET_IID)

#endif 
