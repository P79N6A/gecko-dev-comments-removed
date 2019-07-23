




































#ifndef nsIDOMEventReceiver_h__
#define nsIDOMEventReceiver_h__

#include "nsIDOMEventTarget.h"

class nsIDOMEventListener;
class nsIEventListenerManager;
class nsIDOMEvent;
class nsIDOMEventGroup;





 

#define NS_IDOMEVENTRECEIVER_IID \
{ 0x025957f3, 0x7b19, 0x452b, \
  { 0x89, 0xa1, 0x9b, 0xe6, 0x52, 0xd8, 0xd6, 0xdb } }

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
  NS_IMETHOD GetSystemEventGroup(nsIDOMEventGroup** aGroup) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMEventReceiver, NS_IDOMEVENTRECEIVER_IID)

#endif 
