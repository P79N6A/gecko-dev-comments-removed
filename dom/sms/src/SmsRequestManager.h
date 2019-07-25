





































#ifndef mozilla_dom_sms_SmsRequestManager_h
#define mozilla_dom_sms_SmsRequestManager_h

#include "nsCOMArray.h"

class nsIDOMMozSmsRequest;
class nsPIDOMWindow;
class nsIScriptContext;

namespace mozilla {
namespace dom {
namespace sms {

class SmsRequestManager
{
public:
  static void Init();
  static void Shutdown();
  static SmsRequestManager* GetInstance();

  PRInt32 CreateRequest(nsPIDOMWindow* aWindow,
                        nsIScriptContext* aScriptContext,
                        nsIDOMMozSmsRequest** aRequest);

private:
  static SmsRequestManager* sInstance;

  nsCOMArray<nsIDOMMozSmsRequest> mRequests;
};

} 
} 
} 

#endif 
