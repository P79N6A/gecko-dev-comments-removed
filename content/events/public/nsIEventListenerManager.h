




































#ifndef nsIEventListenerManager_h__
#define nsIEventListenerManager_h__

#include "nsEvent.h"
#include "nsISupports.h"

class nsPresContext;
class nsIDOMEventListener;
class nsIScriptContext;
class nsIDOMEventTarget;
class nsIDOMEventGroup;
class nsIAtom;
class nsPIDOMEventTarget;




#define NS_IEVENTLISTENERMANAGER_IID \
{ 0xadfdc265, 0xea1c, 0x4c0b, \
  { 0x91, 0xca, 0x37, 0x67, 0x2c, 0x83, 0x92, 0x1f } }

class nsIEventListenerManager : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEVENTLISTENERMANAGER_IID)

  nsIEventListenerManager() : mMayHavePaintEventListener(PR_FALSE),
    mMayHaveMutationListeners(PR_FALSE),
    mNoListenerForEvent(0)
  {}

  



  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID, PRInt32 flags) = 0;

  



  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID, PRInt32 flags) = 0;

  



  NS_IMETHOD AddEventListenerByType(nsIDOMEventListener *aListener,
                                    const nsAString& type,
                                    PRInt32 flags,
                                    nsIDOMEventGroup* aEvtGrp) = 0;

  



  NS_IMETHOD RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                       const nsAString& type,
                                       PRInt32 flags,
                                       nsIDOMEventGroup* aEvtGrp) = 0;

  




  NS_IMETHOD AddScriptEventListener(nsISupports *aObject,
                                    nsIAtom *aName,
                                    const nsAString& aFunc,
                                    PRUint32 aLanguage,
                                    PRBool aDeferCompilation,
                                    PRBool aPermitUntrustedEvents) = 0;


  NS_IMETHOD RemoveScriptEventListener(nsIAtom *aName) = 0;

  






  NS_IMETHOD RegisterScriptEventListener(nsIScriptContext *aContext,
                                         void *aScopeObject,
                                         nsISupports *aObject,
                                         nsIAtom* aName) = 0;

  



  NS_IMETHOD CompileScriptEventListener(nsIScriptContext *aContext,
                                        void *aScopeObject,
                                        nsISupports *aObject,
                                        nsIAtom* aName,
                                        PRBool *aDidCompile) = 0;

  




  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsEvent* aEvent,
                         nsIDOMEvent** aDOMEvent,
                         nsPIDOMEventTarget* aCurrentTarget,
                         PRUint32 aFlags,
                         nsEventStatus* aEventStatus) = 0;

  






  NS_IMETHOD Disconnect() = 0;

  



  NS_IMETHOD SetListenerTarget(nsISupports* aTarget) = 0;

  


  NS_IMETHOD HasMutationListeners(PRBool* aListener) = 0;

  




  NS_IMETHOD GetSystemEventGroupLM(nsIDOMEventGroup** aGroup) = 0;

  



  virtual PRBool HasUnloadListeners() = 0;

  






  virtual PRUint32 MutationListenerBits() = 0;

  


  virtual PRBool HasListenersFor(const nsAString& aEventName) = 0;

  


  virtual PRBool HasListeners() = 0;


  



  PRBool MayHavePaintEventListener() { return mMayHavePaintEventListener; }

protected:
  PRUint32 mMayHavePaintEventListener : 1;
  PRUint32 mMayHaveMutationListeners : 1;
  
  
  
  PRUint32 mNoListenerForEvent : 30;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEventListenerManager,
                              NS_IEVENTLISTENERMANAGER_IID)

nsresult
NS_NewEventListenerManager(nsIEventListenerManager** aInstancePtrResult);

#endif 
