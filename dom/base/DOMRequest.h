





#ifndef mozilla_dom_domrequest_h__
#define mozilla_dom_domrequest_h__

#include "nsIDOMDOMRequest.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/DOMRequestBinding.h"

#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {

class DOMRequest : public nsDOMEventTargetHelper,
                   public nsIDOMDOMRequest
{
protected:
  JS::Heap<JS::Value> mResult;
  nsCOMPtr<nsISupports> mError;
  bool mDone;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMREQUEST
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(DOMRequest,
                                                         nsDOMEventTargetHelper)

  
  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  DOMRequestReadyState ReadyState() const
  {
    return mDone ? DOMRequestReadyState::Done
                 : DOMRequestReadyState::Pending;
  }

  JS::Value Result(JSContext* = nullptr) const
  {
    NS_ASSERTION(mDone || mResult == JSVAL_VOID,
               "Result should be undefined when pending");
    return mResult;
  }

  nsISupports* GetError() const
  {
    NS_ASSERTION(mDone || !mError,
                 "Error should be null when pending");
    return mError;
  }

  IMPL_EVENT_HANDLER(success)
  IMPL_EVENT_HANDLER(error)


  void FireSuccess(JS::Handle<JS::Value> aResult);
  void FireError(const nsAString& aError);
  void FireError(nsresult aError);
  void FireDetailedError(nsISupports* aError);

  DOMRequest(nsIDOMWindow* aWindow);
  DOMRequest();

  virtual ~DOMRequest()
  {
    mResult = JSVAL_VOID;
    NS_DROP_JS_OBJECTS(this, DOMRequest);
  }

protected:
  void FireEvent(const nsAString& aType, bool aBubble, bool aCancelable);

  void RootResultVal();

  void Init(nsIDOMWindow* aWindow);
};

class DOMRequestService MOZ_FINAL : public nsIDOMRequestService
{
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
