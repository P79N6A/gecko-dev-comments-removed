





#ifndef mozilla_dom_domrequest_h__
#define mozilla_dom_domrequest_h__

#include "nsIDOMDOMRequest.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/DOMRequestBinding.h"

#include "nsCOMPtr.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class AnyCallback;
class Promise;

class DOMRequest : public DOMEventTargetHelper,
                   public nsIDOMDOMRequest
{
protected:
  JS::Heap<JS::Value> mResult;
  nsRefPtr<DOMError> mError;
  nsRefPtr<Promise> mPromise;
  bool mDone;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMREQUEST
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(DOMRequest,
                                                         DOMEventTargetHelper)

  
  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  DOMRequestReadyState ReadyState() const
  {
    return mDone ? DOMRequestReadyState::Done
                 : DOMRequestReadyState::Pending;
  }

  void GetResult(JSContext*, JS::MutableHandle<JS::Value> aRetval) const
  {
    NS_ASSERTION(mDone || mResult.isUndefined(),
                 "Result should be undefined when pending");
    JS::ExposeValueToActiveJS(mResult);
    aRetval.set(mResult);
  }

  DOMError* GetError() const
  {
    NS_ASSERTION(mDone || !mError,
                 "Error should be null when pending");
    return mError;
  }

  IMPL_EVENT_HANDLER(success)
  IMPL_EVENT_HANDLER(error)

  already_AddRefed<mozilla::dom::Promise>
  Then(JSContext* aCx, AnyCallback* aResolveCallback,
       AnyCallback* aRejectCallback, mozilla::ErrorResult& aRv);

  void FireSuccess(JS::Handle<JS::Value> aResult);
  void FireError(const nsAString& aError);
  void FireError(nsresult aError);
  void FireDetailedError(DOMError* aError);

  explicit DOMRequest(nsPIDOMWindow* aWindow);
  explicit DOMRequest(nsIGlobalObject* aGlobal);

protected:
  virtual ~DOMRequest();

  void FireEvent(const nsAString& aType, bool aBubble, bool aCancelable);

  void RootResultVal();
};

class DOMRequestService final : public nsIDOMRequestService
{
  ~DOMRequestService() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMREQUESTSERVICE

  
  static DOMRequestService* FactoryCreate()
  {
    DOMRequestService* res = new DOMRequestService;
    NS_ADDREF(res);
    return res;
  }
};

} 
} 

#define DOMREQUEST_SERVICE_CONTRACTID "@mozilla.org/dom/dom-request-service;1"

#endif 
