



#ifndef mozilla_dom_TelephonyCallback_h
#define mozilla_dom_TelephonyCallback_h

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsITelephonyService.h"

namespace mozilla {
namespace dom {

class Promise;

namespace telephony {

class TelephonyCallback : public nsITelephonyCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYCALLBACK

  explicit TelephonyCallback(Promise* aPromise);

protected:
  virtual ~TelephonyCallback() {}

protected:
  nsRefPtr<Promise> mPromise;
};

} 
} 
} 

#endif 
