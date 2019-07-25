




































#ifndef mozilla_dom_sms_SmsManager_h
#define mozilla_dom_sms_SmsManager_h

#include "nsIDOMSmsManager.h"
#include "nsIObserver.h"
#include "nsDOMEventTargetHelper.h"

class nsIDOMMozSmsMessage;

namespace mozilla {
namespace dom {
namespace sms {

class SmsManager : public nsIDOMMozSmsManager
                 , public nsIObserver
                 , public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMMOZSMSMANAGER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SmsManager,
                                           nsDOMEventTargetHelper)

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

private:
  


  nsresult Send(JSContext* aCx, JSObject* aGlobal, JSString* aNumber,
                const nsAString& aMessage, jsval* aRequest);

  


  nsresult Delete(PRInt32 aId, nsIDOMMozSmsRequest** aRequest);

  nsresult DispatchTrustedSmsEventToSelf(const nsAString& aEventName,
                                         nsIDOMMozSmsMessage* aMessage);
  NS_DECL_EVENT_HANDLER(received)
  NS_DECL_EVENT_HANDLER(sent)
  NS_DECL_EVENT_HANDLER(delivered)
};

} 
} 
} 

#endif
