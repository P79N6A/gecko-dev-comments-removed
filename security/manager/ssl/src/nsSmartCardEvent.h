




































#ifndef nsSmartCardEvent_h_
#define nsSmartCardEvent_h_

#include "nsIDOMSmartCardEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPCOM.h"


class nsSmartCardEvent : public nsIDOMSmartCardEvent,
                         public nsIDOMNSEvent,
                         public nsIPrivateDOMEvent
{
public:
  nsSmartCardEvent(const nsAString &aTokenName);
  virtual ~nsSmartCardEvent();


  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSMARTCARDEVENT
  NS_DECL_NSIDOMNSEVENT

  
  NS_IMETHOD DuplicatePrivateData();
  NS_IMETHOD SetTarget(nsIDOMEventTarget *aTarget);
  NS_IMETHOD_(nsEvent*) GetInternalNSEvent();
  NS_IMETHOD_(PRBool ) IsDispatchStopped();
  NS_IMETHOD SetTrusted(PRBool aResult);

  NS_DECL_NSIDOMEVENT

protected:
  nsCOMPtr<nsIDOMEvent> mInner;
  nsCOMPtr<nsIPrivateDOMEvent> mPrivate;
  nsCOMPtr<nsIDOMNSEvent> mNSEvent;
  nsString mTokenName;
};

#define SMARTCARDEVENT_INSERT "smartcard-insert"
#define SMARTCARDEVENT_REMOVE "smartcard-remove"

#endif
