




#include "mozilla/Hal.h"
#include "mozilla/HalWakeLock.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Services.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIDOMWindow.h"
#include "nsIObserverService.h"
#include "PowerManagerService.h"
#include "WakeLock.h"

namespace mozilla {
namespace dom {
namespace power {

NS_IMPL_ISUPPORTS1(PowerManagerService, nsIPowerManagerService)

 StaticRefPtr<PowerManagerService> PowerManagerService::sSingleton;

 already_AddRefed<nsIPowerManagerService>
PowerManagerService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new PowerManagerService();
    sSingleton->Init();
    ClearOnShutdown(&sSingleton);
  }

  nsCOMPtr<nsIPowerManagerService> service(do_QueryInterface(sSingleton));
  return service.forget();
}

void
PowerManagerService::Init()
{
  hal::RegisterWakeLockObserver(this);
}

PowerManagerService::~PowerManagerService()
{
  hal::UnregisterWakeLockObserver(this);
}

void
PowerManagerService::ComputeWakeLockState(const hal::WakeLockInformation& aWakeLockInfo,
                                          nsAString &aState)
{
  hal::WakeLockState state = hal::ComputeWakeLockState(aWakeLockInfo.numLocks(),
                                                       aWakeLockInfo.numHidden());
  switch (state) {
  case hal::WAKE_LOCK_STATE_UNLOCKED:
    aState.AssignLiteral("unlocked");
    break;
  case hal::WAKE_LOCK_STATE_HIDDEN:
    aState.AssignLiteral("locked-background");
    break;
  case hal::WAKE_LOCK_STATE_VISIBLE:
    aState.AssignLiteral("locked-foreground");
    break;
  }
}

void
PowerManagerService::Notify(const hal::WakeLockInformation& aWakeLockInfo)
{
  nsAutoString state;
  ComputeWakeLockState(aWakeLockInfo, state);

  




  nsAutoTArray<nsCOMPtr<nsIDOMMozWakeLockListener>, 2> listeners(mWakeLockListeners);

  for (uint32_t i = 0; i < listeners.Length(); ++i) {
    listeners[i]->Callback(aWakeLockInfo.topic(), state);
  }
}

void
PowerManagerService::SyncProfile()
{
  
  
  nsCOMPtr<nsIObserverService> obsServ = services::GetObserverService();
  if (obsServ) {
    NS_NAMED_LITERAL_STRING(context, "shutdown-persist");
    obsServ->NotifyObservers(nullptr, "profile-change-net-teardown", context.get());
    obsServ->NotifyObservers(nullptr, "profile-change-teardown", context.get());
    obsServ->NotifyObservers(nullptr, "profile-before-change", context.get());
  }
}

NS_IMETHODIMP
PowerManagerService::Reboot()
{
  
  SyncProfile();
  hal::Reboot();
  MOZ_NOT_REACHED();
  return NS_OK;
}

NS_IMETHODIMP
PowerManagerService::PowerOff()
{
  
  SyncProfile();
  hal::PowerOff();
  MOZ_NOT_REACHED();
  return NS_OK;
}

NS_IMETHODIMP
PowerManagerService::AddWakeLockListener(nsIDOMMozWakeLockListener *aListener)
{
  if (mWakeLockListeners.Contains(aListener))
    return NS_OK;

  mWakeLockListeners.AppendElement(aListener);
  return NS_OK;
}

NS_IMETHODIMP
PowerManagerService::RemoveWakeLockListener(nsIDOMMozWakeLockListener *aListener)
{
  mWakeLockListeners.RemoveElement(aListener);
  return NS_OK;
}

NS_IMETHODIMP
PowerManagerService::GetWakeLockState(const nsAString &aTopic, nsAString &aState)
{
  hal::WakeLockInformation info;
  hal::GetWakeLockInfo(aTopic, &info);

  ComputeWakeLockState(info, aState);

  return NS_OK;
}

NS_IMETHODIMP
PowerManagerService::NewWakeLock(const nsAString &aTopic,
                                 nsIDOMWindow *aWindow,
                                 nsIDOMMozWakeLock **aWakeLock)
{
  nsRefPtr<WakeLock> wakelock = new WakeLock();
  nsresult rv = wakelock->Init(aTopic, aWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMMozWakeLock> wl(wakelock);
  wl.forget(aWakeLock);

  return NS_OK;
}

} 
} 
} 
