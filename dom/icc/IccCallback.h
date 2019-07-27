





#ifndef mozilla_dom_icc_IccCallback_h
#define mozilla_dom_icc_IccCallback_h

#include "nsCOMPtr.h"
#include "nsIIccService.h"

namespace mozilla {
namespace dom {

class DOMRequest;
class Promise;

namespace icc {











class IccCallback final : public nsIIccCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICCCALLBACK

  
  
  IccCallback(nsPIDOMWindow* aWindow, DOMRequest* aRequest,
              bool aIsCardLockEnabled = false);
  IccCallback(nsPIDOMWindow* aWindow, Promise* aPromise);

private:
  ~IccCallback() {}

  nsresult
  NotifySuccess(JS::Handle<JS::Value> aResult);

  
  
  nsresult
  NotifyGetCardLockEnabled(bool aResult);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<DOMRequest> mRequest;
  nsRefPtr<Promise> mPromise;
  
  
  bool mIsCardLockEnabled;
};

} 
} 
} 

#endif 
