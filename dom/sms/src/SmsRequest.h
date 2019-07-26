




#ifndef mozilla_dom_sms_SmsRequest_h
#define mozilla_dom_sms_SmsRequest_h

#include "nsIDOMSmsRequest.h"
#include "nsISmsRequest.h"
#include "nsDOMEventTargetHelper.h"

class nsIDOMMozSmsMessage;
class nsIDOMMozSmsCursor;

namespace mozilla {
namespace dom {
namespace sms {

class SmsRequestChild;
class SmsRequestParent;
class MessageReply;
class ThreadListItem;



class SmsRequestForwarder : public nsISmsRequest
{
  friend class SmsRequestChild;

public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_NSISMSREQUEST(mRealRequest->)

  SmsRequestForwarder(nsISmsRequest* aRealRequest) {
    mRealRequest = aRealRequest;
  }

private:
  virtual
  ~SmsRequestForwarder() {}

  nsISmsRequest* GetRealRequest() {
    return mRealRequest;
  }

  nsCOMPtr<nsISmsRequest> mRealRequest;
};

class SmsManager;

class SmsRequest : public nsDOMEventTargetHelper
                 , public nsIDOMMozSmsRequest
                 , public nsISmsRequest
{
public:
  friend class SmsCursor;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMREQUEST
  NS_DECL_NSISMSREQUEST
  NS_DECL_NSIDOMMOZSMSREQUEST

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(SmsRequest,
                                                         nsDOMEventTargetHelper)

  static already_AddRefed<nsIDOMMozSmsRequest> Create(SmsManager* aManager);

  static already_AddRefed<SmsRequest> Create(SmsRequestParent* requestParent);
  void Reset();

  void SetActorDied() {
    mParentAlive = false;
  }

  void
  NotifyThreadList(const InfallibleTArray<ThreadListItem>& aItems);

private:
  SmsRequest() MOZ_DELETE;

  SmsRequest(SmsManager* aManager);
  SmsRequest(SmsRequestParent* aParent);
  ~SmsRequest();

  nsresult SendMessageReply(const MessageReply& aReply);

  


  void RootResult();

  


  void UnrootResult();

  


  void SetSuccess(nsIDOMMozSmsMessage* aMessage);

  


  void SetSuccess(bool aResult);

  


  void SetSuccess(nsIDOMMozSmsCursor* aCursor);

  


  void SetSuccess(const jsval& aVal);

  


  void SetError(int32_t aError);

  




  bool SetSuccessInternal(nsISupports* aObject);

  nsresult DispatchTrustedEvent(const nsAString& aEventName);

  template <class T>
  nsresult NotifySuccess(T aParam);
  nsresult NotifyError(int32_t aError);

  jsval     mResult;
  bool      mResultRooted;
  bool      mDone;
  bool      mParentAlive;
  SmsRequestParent* mParent;
  nsCOMPtr<nsIDOMDOMError> mError;
  nsCOMPtr<nsIDOMMozSmsCursor> mCursor;
};

} 
} 
} 

#endif 
