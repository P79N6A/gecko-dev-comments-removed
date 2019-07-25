




































#ifndef mozilla_dom_sms_SmsRequest_h
#define mozilla_dom_sms_SmsRequest_h

#include "nsIDOMSmsRequest.h"
#include "nsDOMEventTargetWrapperCache.h"

class nsIDOMMozSmsMessage;

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

private:
  SmsRequest() MOZ_DELETE;

  SmsRequest(nsPIDOMWindow* aWindow, nsIScriptContext* aScriptContext);
  ~SmsRequest();

  


  void RootResult();

  


  void UnrootResult();

  


  void SetSuccess(nsIDOMMozSmsMessage* aMessage);

  


  void SetSuccess(bool aResult);

  


  void SetError(ErrorType aError);

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
