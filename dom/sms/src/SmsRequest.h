




































#ifndef mozilla_dom_sms_SmsRequest_h
#define mozilla_dom_sms_SmsRequest_h

#include "nsIDOMSmsRequest.h"
#include "nsDOMEventTargetWrapperCache.h"

class nsIDOMMozSmsMessage;
class nsIDOMMozSmsCursor;

namespace mozilla {
namespace dom {
namespace sms {

class SmsRequest : public nsIDOMMozSmsRequest
                 , public nsDOMEventTargetWrapperCache
{
public:
  friend class SmsRequestManager;

  




  enum ErrorType {
    eNoError = 0,
    eNoSignalError,
    eNotFoundError,
    eUnknownError,
    eInternalError,
  };

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSREQUEST

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetWrapperCache::)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(SmsRequest,
                                                         nsDOMEventTargetWrapperCache)

  void Reset();

private:
  SmsRequest() MOZ_DELETE;

  SmsRequest(nsPIDOMWindow* aWindow, nsIScriptContext* aScriptContext);
  ~SmsRequest();

  


  void RootResult();

  


  void UnrootResult();

  


  void SetSuccess(nsIDOMMozSmsMessage* aMessage);

  


  void SetSuccess(bool aResult);

  


  void SetSuccess(nsIDOMMozSmsCursor* aCursor);

  


  void SetError(ErrorType aError);

  




  bool SetSuccessInternal(nsISupports* aObject);

  jsval     mResult;
  bool      mResultRooted;
  ErrorType mError;
  bool      mDone;

  NS_DECL_EVENT_HANDLER(success)
  NS_DECL_EVENT_HANDLER(error)
};

} 
} 
} 

#endif
