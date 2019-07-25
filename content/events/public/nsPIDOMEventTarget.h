




































#ifndef nsPIDOMEventTarget_h_
#define nsPIDOMEventTarget_h_

#include "nsIDOMEventTarget.h"

#if 0
#include "nsEvent.h"

class nsIDOMEvent;
class nsPresContext;
class nsEventChainPreVisitor;
class nsEventChainPostVisitor;
class nsIEventListenerManager;
class nsIDOMEventListener;
class nsIDOMEventGroup;
class nsIScriptContext;
struct JSContext;


#define NS_PIDOMEVENTTARGET_IID \
  { 0x89292f3a, 0x535d, 0x4ba0, \
    { 0x88, 0x2a, 0x10, 0xcf, 0xf9, 0xe2, 0x1b, 0xcc } }

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

  



   virtual JSContext* GetJSContextForEventHandlers() { return nsnull; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMEventTarget, NS_PIDOMEVENTTARGET_IID)
#endif

#endif 
