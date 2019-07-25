





#ifndef mozilla_dom_domrequest_h__
#define mozilla_dom_domrequest_h__

#include "nsIDOMDOMRequest.h"
#include "nsIDOMDOMError.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {

class DOMRequest : public nsDOMEventTargetHelper,
                   public nsIDOMDOMRequest
{
protected:
  jsval mResult;
  nsCOMPtr<nsIDOMDOMError> mError;
  bool mDone;
  bool mRooted;

  NS_DECL_EVENT_HANDLER(success)
  NS_DECL_EVENT_HANDLER(error)

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMREQUEST
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(DOMRequest,
                                                         nsDOMEventTargetHelper)

  void FireSuccess(jsval aResult);
  void FireError(const nsAString& aError);
  void FireError(nsresult aError);

  DOMRequest(nsIDOMWindow* aWindow);
  DOMRequest();

  virtual ~DOMRequest()
  {
    if (mRooted) {
      UnrootResultVal();
    }
  }

protected:
  void FireEvent(const nsAString& aType, bool aBubble, bool aCancelable);

  void RootResultVal();
  void UnrootResultVal();

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
