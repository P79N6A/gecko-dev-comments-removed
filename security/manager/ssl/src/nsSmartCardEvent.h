



#ifndef nsSmartCardEvent_h_
#define nsSmartCardEvent_h_

#include "nsIDOMSmartCardEvent.h"
#include "nsIDOMEvent.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPCOM.h"


class nsSmartCardEvent : public nsIDOMSmartCardEvent
{
public:
  nsSmartCardEvent(const nsAString &aTokenName);
  virtual ~nsSmartCardEvent();


  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENT
  NS_DECL_NSIDOMSMARTCARDEVENT

protected:
  nsCOMPtr<nsIDOMEvent> mInner;
  nsString mTokenName;
};

#define SMARTCARDEVENT_INSERT "smartcard-insert"
#define SMARTCARDEVENT_REMOVE "smartcard-remove"

#endif
