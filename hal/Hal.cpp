






































#include "Hal.h"
#include "mozilla/Util.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "mozilla/Observer.h"

#define PROXY_IF_SANDBOXED(_call)                 \
  do {                                            \
    if (InSandbox()) {                            \
      hal_sandbox::_call;                         \
    } else {                                      \
      hal_impl::_call;                            \
    }                                             \
  } while (0)

namespace mozilla {
namespace hal {

static void
AssertMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}

static bool
InSandbox()
{
  return GeckoProcessType_Content == XRE_GetProcessType();
}

void
Vibrate(const nsTArray<uint32>& pattern)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(Vibrate(pattern));
}

class BatteryObserversManager
{
public:
  void AddObserver(BatteryObserver* aObserver) {
    if (!mObservers) {
      mObservers = new ObserverList<BatteryInformation>();
    }

    mObservers->AddObserver(aObserver);

    if (mObservers->Length() == 1) {
      PROXY_IF_SANDBOXED(EnableBatteryNotifications());
    }
  }

  void RemoveObserver(BatteryObserver* aObserver) {
    mObservers->RemoveObserver(aObserver);

    if (mObservers->Length() == 0) {
      PROXY_IF_SANDBOXED(DisableBatteryNotifications());

      delete mObservers;
      mObservers = 0;

      delete mBatteryInfo;
      mBatteryInfo = 0;
    }
  }

  void CacheBatteryInformation(const BatteryInformation& aBatteryInfo) {
    if (mBatteryInfo) {
      delete mBatteryInfo;
    }
    mBatteryInfo = new BatteryInformation(aBatteryInfo);
  }

  bool HasCachedBatteryInformation() const {
    return mBatteryInfo;
  }

  void GetCachedBatteryInformation(BatteryInformation* aBatteryInfo) const {
    *aBatteryInfo = *mBatteryInfo;
  }

  void Broadcast(const BatteryInformation& aBatteryInfo) {
    MOZ_ASSERT(mObservers);
    mObservers->Broadcast(aBatteryInfo);
  }

private:
  ObserverList<BatteryInformation>* mObservers;
  BatteryInformation*               mBatteryInfo;
};

static BatteryObserversManager sBatteryObservers;

void
RegisterBatteryObserver(BatteryObserver* aBatteryObserver)
{
  AssertMainThread();
  sBatteryObservers.AddObserver(aBatteryObserver);
}

void
UnregisterBatteryObserver(BatteryObserver* aBatteryObserver)
{
  AssertMainThread();
  sBatteryObservers.RemoveObserver(aBatteryObserver);
}




void
GetCurrentBatteryInformation(BatteryInformation* aBatteryInfo)
{
  AssertMainThread();

  if (sBatteryObservers.HasCachedBatteryInformation()) {
    sBatteryObservers.GetCachedBatteryInformation(aBatteryInfo);
  } else {
    PROXY_IF_SANDBOXED(GetCurrentBatteryInformation(aBatteryInfo));
    sBatteryObservers.CacheBatteryInformation(*aBatteryInfo);
  }
}

void NotifyBatteryChange(const BatteryInformation& aBatteryInfo)
{
  AssertMainThread();

  sBatteryObservers.CacheBatteryInformation(aBatteryInfo);
  sBatteryObservers.Broadcast(aBatteryInfo);
}

} 
} 
