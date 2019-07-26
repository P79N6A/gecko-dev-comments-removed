




#ifndef mozilla_dom_mobilemessage_MobileMessageManager_h
#define mozilla_dom_mobilemessage_MobileMessageManager_h

#include "nsIDOMMobileMessageManager.h"
#include "nsIObserver.h"
#include "nsDOMEventTargetHelper.h"

class nsIDOMMozSmsMessage;
class nsIDOMMozMmsMessage;

namespace mozilla {
namespace dom {

class MobileMessageManager : public nsDOMEventTargetHelper
                           , public nsIDOMMozMobileMessageManager
                           , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMMOZMOBILEMESSAGEMANAGER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

private:
  


  nsresult Send(JSContext* aCx, JS::Handle<JSObject*> aGlobal,
                uint32_t aServiceId,
                JS::Handle<JSString*> aNumber,
                const nsAString& aMessage,
                JS::Value* aRequest);

  nsresult DispatchTrustedSmsEventToSelf(const char* aTopic,
                                         const nsAString& aEventName,
                                         nsISupports* aMsg);

  


  nsresult GetMessageId(AutoPushJSContext &aCx, const JS::Value &aMessage,
                        int32_t &aId);
};

} 
} 

#endif 
