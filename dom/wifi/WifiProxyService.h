



#ifndef WifiProxyService_h
#define WifiProxyService_h

#include "nsIWifiService.h"
#include "nsCOMPtr.h"
#include "nsThread.h"
#include "mozilla/dom/WifiOptionsBinding.h"
#include "nsTArray.h"

namespace mozilla {

class WifiProxyService MOZ_FINAL : public nsIWifiProxyService
{
private:
  struct EventThreadListEntry
  {
    nsCOMPtr<nsIThread> mThread;
    nsCString mInterface;
  };

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWIFIPROXYSERVICE

  static already_AddRefed<WifiProxyService>
  FactoryCreate();

  void DispatchWifiEvent(const nsAString& aEvent, const nsACString& aInterface);
  void DispatchWifiResult(const mozilla::dom::WifiResultOptions& aOptions,
                          const nsACString& aInterface);

private:
  WifiProxyService();
  ~WifiProxyService();

  nsTArray<EventThreadListEntry> mEventThreadList;
  nsCOMPtr<nsIThread> mControlThread;
  nsCOMPtr<nsIWifiEventListener> mListener;
};

} 

#endif 
