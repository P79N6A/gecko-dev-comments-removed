



#ifndef NetworkWorker_h
#define NetworkWorker_h

#include "mozilla/dom/NetworkOptionsBinding.h"
#include "mozilla/ipc/Netd.h"
#include "nsINetworkWorker.h"
#include "nsCOMPtr.h"
#include "nsThread.h"

namespace mozilla {

class NetworkWorker MOZ_FINAL : public nsINetworkWorker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETWORKWORKER

  static already_AddRefed<NetworkWorker> FactoryCreate();

  void DispatchNetworkResult(const mozilla::dom::NetworkResultOptions& aOptions);

private:
  NetworkWorker();
  ~NetworkWorker();

  static void NotifyResult(mozilla::dom::NetworkResultOptions& aResult);

  nsCOMPtr<nsINetworkEventListener> mListener;
};

} 

#endif  
