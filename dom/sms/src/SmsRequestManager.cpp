




































#include "SmsRequestManager.h"
#include "SmsRequest.h"

namespace mozilla {
namespace dom {
namespace sms {

SmsRequestManager* SmsRequestManager::sInstance = nsnull;

void
SmsRequestManager::Init()
{
  NS_PRECONDITION(!sInstance,
                  "sInstance shouldn't be set. Did you call Init() twice?");
  sInstance = new SmsRequestManager();
}

void
SmsRequestManager::Shutdown()
{
  NS_PRECONDITION(sInstance, "sInstance should be set. Did you call Init()?");

  delete sInstance;
  sInstance = nsnull;
}

 SmsRequestManager*
SmsRequestManager::GetInstance()
{
  return sInstance;
}

PRInt32
SmsRequestManager::CreateRequest(nsPIDOMWindow* aWindow,
                                 nsIScriptContext* aScriptContext,
                                 nsIDOMMozSmsRequest** aRequest)
{
  nsCOMPtr<nsIDOMMozSmsRequest> request =
    new SmsRequest(aWindow, aScriptContext);

  PRInt32 size = mRequests.Count();

  
  for (PRInt32 i=0; i<size; ++i) {
    if (mRequests[i]) {
      continue;
    }

    mRequests.ReplaceObjectAt(request, i);
    NS_ADDREF(*aRequest = request);
    return i;
  }


  mRequests.AppendObject(request);
  NS_ADDREF(*aRequest = request);
  return size;
}

} 
} 
} 
