



#ifndef mozilla_dom_power_PowerManagerService_h
#define mozilla_dom_power_PowerManagerService_h

#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsIPowerManagerService.h"
#include "mozilla/Observer.h"
#include "Types.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {
namespace dom {
namespace power {

class PowerManagerService
  : public nsIPowerManagerService
  , public WakeLockObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPOWERMANAGERSERVICE

  static already_AddRefed<PowerManagerService> GetInstance();

  void Init();

  
  void Notify(const hal::WakeLockInformation& aWakeLockInfo);

private:

  ~PowerManagerService();

  void ComputeWakeLockState(const hal::WakeLockInformation& aWakeLockInfo,
                            nsAString &aState);

  void SyncProfile();

  static StaticRefPtr<PowerManagerService> sSingleton;

  nsTArray<nsCOMPtr<nsIDOMMozWakeLockListener> > mWakeLockListeners;
  
  int32_t mWatchdogTimeoutSecs;
};

} 
} 
} 

#endif 
