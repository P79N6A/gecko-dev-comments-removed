




































#ifndef nsIDOMEventReceiver_h__
#define nsIDOMEventReceiver_h__

#include "nsIDOMEventTarget.h"

class nsIDOMEventListener;
class nsIEventListenerManager;
class nsIDOMEvent;
class nsIDOMEventGroup;





 

#define NS_IDOMEVENTRECEIVER_IID \
{0x2fa04cfb, 0x2494, 0x41e5, \
  { 0xba, 0x76, 0x9a, 0x79, 0x29, 0x3e, 0xeb, 0x7e } }

class nsIDOMEventReceiver : public nsIDOMEventTarget
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMEVENTRECEIVER_IID)

  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID) = 0;
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID) = 0;
  NS_IMETHOD GetListenerManager(PRBool aCreateIfNotFound,
                                nsIEventListenerManager** aResult) = 0;
  NS_IMETHOD HandleEvent(nsIDOMEvent *aEvent) = 0;
  NS_IMETHOD GetSystemEventGroup(nsIDOMEventGroup** aGroup) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMEventReceiver, NS_IDOMEVENTRECEIVER_IID)

#endif 
