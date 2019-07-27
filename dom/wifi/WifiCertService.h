




#ifndef WifiCertService_h
#define WifiCertService_h

#include "nsIWifiCertService.h"
#include "nsCOMPtr.h"
#include "nsNSSShutDown.h"
#include "nsThread.h"
#include "mozilla/dom/WifiOptionsBinding.h"

namespace mozilla {
namespace dom {

class WifiCertService final : public nsIWifiCertService,
                                  public nsNSSShutDownObject
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
  virtual void virtualDestroyNSSReference() {};
  nsCOMPtr<nsIWifiEventListener> mListener;
};

} 
} 

#endif 
