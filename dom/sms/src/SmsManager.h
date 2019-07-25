




































#ifndef mozilla_dom_sms_SmsManager_h
#define mozilla_dom_sms_SmsManager_h

#include "nsIDOMSmsManager.h"
#include "nsIObserver.h"
#include "nsDOMEventTargetWrapperCache.h"

class nsIDOMMozSmsMessage;

namespace mozilla {
namespace dom {
namespace sms {

class SmsManager : public nsIDOMMozSmsManager
                 , public nsIObserver
                 , public nsDOMEventTargetWrapperCache
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMMOZSMSMANAGER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetWrapperCache::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SmsManager,
                                           nsDOMEventTargetWrapperCache)

  void Init(nsPIDOMWindow *aWindow, nsIScriptContext* aScriptContext);
  void Shutdown();

private:
  nsresult DispatchTrustedSmsEventToSelf(const nsAString& aEventName,
                                         nsIDOMMozSmsMessage* aMessage);
  NS_DECL_EVENT_HANDLER(received)
};

} 
} 
} 

#endif
