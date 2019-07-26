




#ifndef mozilla_dom_mobilemessage_MobileMessageManager_h
#define mozilla_dom_mobilemessage_MobileMessageManager_h

#include "mozilla/DOMEventTargetHelper.h"
#include "nsIDOMMobileMessageManager.h"
#include "nsIObserver.h"

class nsISmsService;
class nsIDOMMozSmsMessage;
class nsIDOMMozMmsMessage;

namespace mozilla {
namespace dom {

class MobileMessageManager : public DOMEventTargetHelper
                           , public nsIDOMMozMobileMessageManager
                           , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMMOZMOBILEMESSAGEMANAGER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

private:
  


  nsresult
  Send(JSContext* aCx,
       JS::Handle<JSObject*> aGlobal,
       nsISmsService* aSmsService,
       uint32_t aServiceId,
       JS::Handle<JSString*> aNumber,
       const nsAString& aText,
       JS::MutableHandle<JS::Value> aRequest);

  nsresult
  DispatchTrustedSmsEventToSelf(const char* aTopic,
                                const nsAString& aEventName,
                                nsISupports* aMsg);

  


  nsresult
  GetMessageId(JSContext* aCx,
               const JS::Value& aMessage,
               int32_t* aId);
};

} 
} 

#endif 
