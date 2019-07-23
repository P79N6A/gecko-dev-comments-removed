




































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




#define NS_IEVENTLISTENERMANAGER_IID \
{ 0x6ee5eeeb, 0x1bf3, 0x4865, \
  { 0xa9, 0x52, 0x3b, 0x3e, 0x97, 0x9b, 0x4a, 0xb3 } }


class nsIEventListenerManager : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEVENTLISTENERMANAGER_IID)

  



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
                         nsISupports* aCurrentTarget,
                         PRUint32 aFlags,
                         nsEventStatus* aEventStatus) = 0;

  






  NS_IMETHOD Disconnect() = 0;

  



  NS_IMETHOD SetListenerTarget(nsISupports* aTarget) = 0;

  


  NS_IMETHOD HasMutationListeners(PRBool* aListener) = 0;

  




  NS_IMETHOD GetSystemEventGroupLM(nsIDOMEventGroup** aGroup) = 0;

  



  virtual PRBool HasUnloadListeners() = 0;

  






  virtual PRUint32 MutationListenerBits() = 0;

  


  virtual PRBool HasListenersFor(const nsAString& aEventName) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEventListenerManager,
                              NS_IEVENTLISTENERMANAGER_IID)

nsresult
NS_NewEventListenerManager(nsIEventListenerManager** aInstancePtrResult);

#endif 
