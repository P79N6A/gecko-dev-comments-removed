




#ifndef mozilla_dom_mobilemessage_SmsManager_h
#define mozilla_dom_mobilemessage_SmsManager_h

#include "nsIDOMSmsManager.h"
#include "nsIObserver.h"
#include "nsDOMEventTargetHelper.h"

class nsIDOMMozSmsMessage;

namespace mozilla {
namespace dom {

class SmsManager : public nsDOMEventTargetHelper
                 , public nsIDOMMozSmsManager
                 , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMMOZSMSMANAGER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  static already_AddRefed<SmsManager>
  CreateInstanceIfAllowed(nsPIDOMWindow *aWindow);

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

private:
  


  nsresult Send(JSContext* aCx, JSObject* aGlobal, JSString* aNumber,
                const nsAString& aMessage, jsval* aRequest);

  


  nsresult Delete(int32_t aId, nsIDOMMozSmsRequest** aRequest);

  nsresult DispatchTrustedSmsEventToSelf(const nsAString& aEventName,
                                         nsIDOMMozSmsMessage* aMessage);
};

} 
} 

#endif 
