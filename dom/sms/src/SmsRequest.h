




#ifndef mozilla_dom_sms_SmsRequest_h
#define mozilla_dom_sms_SmsRequest_h

#include "nsIDOMSmsRequest.h"
#include "nsDOMEventTargetHelper.h"

class nsIDOMMozSmsMessage;
class nsIDOMMozSmsCursor;

namespace mozilla {
namespace dom {
namespace sms {
class SmsManager;

class SmsRequest : public nsDOMEventTargetHelper
                 , public nsIDOMMozSmsRequest
{
public:
  friend class SmsRequestManager;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSREQUEST

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(SmsRequest,
                                                         nsDOMEventTargetHelper)

  void Reset();

private:
  SmsRequest() MOZ_DELETE;

  SmsRequest(SmsManager* aManager);
  ~SmsRequest();

  


  void RootResult();

  


  void UnrootResult();

  


  void SetSuccess(nsIDOMMozSmsMessage* aMessage);

  


  void SetSuccess(bool aResult);

  


  void SetSuccess(nsIDOMMozSmsCursor* aCursor);

  


  void SetError(PRInt32 aError);

  




  bool SetSuccessInternal(nsISupports* aObject);

  




  nsIDOMMozSmsCursor* GetCursor();

  jsval     mResult;
  bool      mResultRooted;
  PRInt32   mError;
  bool      mDone;
  nsCOMPtr<nsIDOMMozSmsCursor> mCursor;

  NS_DECL_EVENT_HANDLER(success)
  NS_DECL_EVENT_HANDLER(error)
};

inline nsIDOMMozSmsCursor*
SmsRequest::GetCursor()
{
  return mCursor;
}

} 
} 
} 

#endif
