




#include "mozilla/dom/PowerManager.h"

#include "mozilla/Hal.h"
#include "WakeLock.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIDocument.h"
#include "nsIPermissionManager.h"
#include "nsIPowerManagerService.h"
#include "nsIPrincipal.h"
#include "nsPIDOMWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsError.h"
#include "mozilla/dom/MozPowerManagerBinding.h"
#include "mozilla/Services.h"

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PowerManager)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozWakeLockListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(PowerManager, mListeners, mWindow)

NS_IMPL_CYCLE_COLLECTING_ADDREF(PowerManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PowerManager)

 JSObject*
PowerManager::WrapObject(JSContext* aCx)
{
  return MozPowerManagerBinding::Wrap(aCx, this);
}

nsresult
PowerManager::Init(nsIDOMWindow *aWindow)
{
  mWindow = aWindow;

  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  NS_ENSURE_STATE(pmService);

  
  pmService->AddWakeLockListener(this);
  return NS_OK;
}

nsresult
PowerManager::Shutdown()
{
  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  NS_ENSURE_STATE(pmService);

  
  pmService->RemoveWakeLockListener(this);
  return NS_OK;
}

void
PowerManager::Reboot(ErrorResult& aRv)
{
  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  if (pmService) {
    pmService->Reboot();
  } else {
    aRv.Throw(NS_ERROR_UNEXPECTED);
  }
}

void
PowerManager::FactoryReset()
{
  hal::FactoryReset();
}

void
PowerManager::PowerOff(ErrorResult& aRv)
{
  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  if (pmService) {
    pmService->PowerOff();
  } else {
    aRv.Throw(NS_ERROR_UNEXPECTED);
  }
}

void
PowerManager::AddWakeLockListener(nsIDOMMozWakeLockListener *aListener)
{
  if (!mListeners.Contains(aListener)) {
    mListeners.AppendElement(aListener);
  }
}

void
PowerManager::RemoveWakeLockListener(nsIDOMMozWakeLockListener *aListener)
{
  mListeners.RemoveElement(aListener);
}

void
PowerManager::GetWakeLockState(const nsAString& aTopic,
                               nsAString& aState,
                               ErrorResult& aRv)
{
  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  if (pmService) {
    aRv = pmService->GetWakeLockState(aTopic, aState);
  } else {
    aRv.Throw(NS_ERROR_UNEXPECTED);
  }
}

NS_IMETHODIMP
PowerManager::Callback(const nsAString &aTopic, const nsAString &aState)
{
  







  nsAutoTArray<nsCOMPtr<nsIDOMMozWakeLockListener>, 2> listeners(mListeners);
  for (uint32_t i = 0; i < listeners.Length(); ++i) {
    listeners[i]->Callback(aTopic, aState);
  }

  return NS_OK;
}

bool
PowerManager::ScreenEnabled()
{
  return hal::GetScreenEnabled();
}

void
PowerManager::SetScreenEnabled(bool aEnabled)
{
  hal::SetScreenEnabled(aEnabled);
}

bool
PowerManager::KeyLightEnabled()
{
  return hal::GetKeyLightEnabled();
}

void
PowerManager::SetKeyLightEnabled(bool aEnabled)
{
  hal::SetKeyLightEnabled(aEnabled);
}

double
PowerManager::ScreenBrightness()
{
  return hal::GetScreenBrightness();
}

void
PowerManager::SetScreenBrightness(double aBrightness, ErrorResult& aRv)
{
  if (0 <= aBrightness && aBrightness <= 1) {
    hal::SetScreenBrightness(aBrightness);
  } else {
    aRv.Throw(NS_ERROR_INVALID_ARG);
  }
}

bool
PowerManager::CpuSleepAllowed()
{
  return hal::GetCpuSleepAllowed();
}

void
PowerManager::SetCpuSleepAllowed(bool aAllowed)
{
  hal::SetCpuSleepAllowed(aAllowed);
}

already_AddRefed<PowerManager>
PowerManager::CreateInstance(nsPIDOMWindow* aWindow)
{
  nsRefPtr<PowerManager> powerManager = new PowerManager();
  if (NS_FAILED(powerManager->Init(aWindow))) {
    powerManager = nullptr;
  }

  return powerManager.forget();
}

} 
} 
