





#ifndef mozilla_dom_RequestSyncWifiService_h
#define mozilla_dom_RequestSyncWifiService_h

#include "mozilla/dom/network/Types.h"
#include "mozilla/Hal.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {

class RequestSyncWifiService MOZ_FINAL : public nsISupports
                                       , public NetworkObserver
{
public:
  NS_DECL_ISUPPORTS

  static void Init();

  static already_AddRefed<RequestSyncWifiService> GetInstance();

  void Notify(const hal::NetworkInformation& aNetworkInfo);

private:
  RequestSyncWifiService()
    : mIsWifi(false)
  {}

  ~RequestSyncWifiService()
  {}

  bool mIsWifi;
};

} 
} 

#endif 
