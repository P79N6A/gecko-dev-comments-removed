




#ifndef mozilla_dom_telephony_TelephonyIPCProvider_h
#define mozilla_dom_telephony_TelephonyIPCProvider_h

#include "mozilla/dom/telephony/TelephonyCommon.h"
#include "mozilla/Attributes.h"
#include "nsIObserver.h"
#include "nsITelephonyProvider.h"

BEGIN_TELEPHONY_NAMESPACE

class PTelephonyChild;

class TelephonyIPCProvider MOZ_FINAL : public nsITelephonyProvider
                                     , public nsITelephonyListener
                                     , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYPROVIDER
  NS_DECL_NSITELEPHONYLISTENER
  NS_DECL_NSIOBSERVER

  TelephonyIPCProvider();

protected:
  virtual ~TelephonyIPCProvider();

private:
  nsTArray<nsCOMPtr<nsITelephonyListener> > mListeners;
  PTelephonyChild* mPTelephonyChild;
  uint32_t mDefaultServiceId;
};

END_TELEPHONY_NAMESPACE

#endif 
