




































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
  { 0xe6579895, 0xa23c, 0x4afc, \
    { 0x87, 0x2a, 0xd5, 0x3d, 0xa7, 0x1d, 0xef, 0x5d } }

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

  





  virtual nsIEventListenerManager* GetListenerManager(PRBool aMayCreate) = 0;

  


  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID) = 0;
  


  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID) = 0;
  
  


  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup) = 0;

  




  virtual nsIScriptContext* GetContextForEventHandlers(nsresult* aRv) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMEventTarget, NS_PIDOMEVENTTARGET_IID)

#endif 
