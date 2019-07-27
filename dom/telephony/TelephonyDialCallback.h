



#ifndef mozilla_dom_TelephonyDialCallback_h
#define mozilla_dom_TelephonyDialCallback_h

#include "Telephony.h"
#include "mozilla/dom/DOMRequest.h"
#include "mozilla/dom/MozMobileConnectionBinding.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/dom/telephony/TelephonyCallback.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsITelephonyService.h"
#include "nsJSUtils.h"
#include "nsString.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {
namespace telephony {

class TelephonyDialCallback MOZ_FINAL : public TelephonyCallback,
                                        public nsITelephonyDialCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSITELEPHONYDIALCALLBACK

  TelephonyDialCallback(nsPIDOMWindow* aWindow, Telephony* aTelephony,
                        Promise* aPromise, uint32_t aServiceId);

  NS_FORWARD_NSITELEPHONYCALLBACK(TelephonyCallback::)

private:
  ~TelephonyDialCallback() {}

  nsresult
  NotifyDialMMISuccess(JS::Handle<JS::Value> aResult);

  nsresult
  NotifyDialMMISuccess(JSContext* aCx, const MozMMIResult& aResult);


  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<Telephony> mTelephony;
  uint32_t mServiceId;

  nsRefPtr<DOMRequest> mMMIRequest;
  nsString mServiceCode;
};

} 
} 
} 

#endif 
