




#ifndef mozilla_dom_telephony_TelephonyIPCService_h
#define mozilla_dom_telephony_TelephonyIPCService_h

#include "mozilla/dom/telephony/TelephonyCommon.h"
#include "mozilla/Attributes.h"
#include "nsIObserver.h"
#include "nsITelephonyService.h"

BEGIN_TELEPHONY_NAMESPACE

struct IPCTelephonyRequest;
class PTelephonyChild;

class TelephonyIPCService MOZ_FINAL : public nsITelephonyService
                                     , public nsITelephonyListener
                                     , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYSERVICE
  NS_DECL_NSITELEPHONYLISTENER
  NS_DECL_NSIOBSERVER

  TelephonyIPCService();

  void NoteActorDestroyed();

private:
  ~TelephonyIPCService();

  nsTArray<nsCOMPtr<nsITelephonyListener> > mListeners;
  PTelephonyChild* mPTelephonyChild;
  uint32_t mDefaultServiceId;

  nsresult SendRequest(nsITelephonyListener *aListener,
                       nsITelephonyCallback *aCallback,
                       const IPCTelephonyRequest& aRequest);
};

END_TELEPHONY_NAMESPACE

#endif 
