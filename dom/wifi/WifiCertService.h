




#ifndef WifiCertService_h
#define WifiCertService_h

#include "nsIWifiCertService.h"
#include "nsCOMPtr.h"
#include "nsThread.h"
#include "mozilla/dom/WifiOptionsBinding.h"

namespace mozilla {
namespace dom {

class WifiCertService MOZ_FINAL : public nsIWifiCertService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWIFICERTSERVICE

  static already_AddRefed<WifiCertService>
  FactoryCreate();
  void DispatchResult(const mozilla::dom::WifiCertServiceResultOptions& aOptions);

private:
  WifiCertService();
  ~WifiCertService();
  nsCOMPtr<nsIThread> mRequestThread;
  nsCOMPtr<nsIWifiEventListener> mListener;
};

} 
} 

#endif 
