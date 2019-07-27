




#include "mozilla/dom/ContentParent.h"
#include "mozilla/Hal.h"
#include "mozilla/HalWakeLock.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "jsprf.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIDOMWindow.h"
#include "nsIObserverService.h"
#include "PowerManagerService.h"
#include "WakeLock.h"


#ifdef XP_WIN
#include <process.h>
#else
#include <unistd.h>
#endif

#ifdef ANDROID
#include <android/log.h>
extern "C" char* PrintJSStack();
static void LogFunctionAndJSStack(const char* funcname) {
  char *jsstack = PrintJSStack();
  __android_log_print(ANDROID_LOG_INFO, "PowerManagerService", \
                      "Call to %s. The JS stack is:\n%s\n",
                      funcname,
                      jsstack ? jsstack : "<no JS stack>");
  JS_smprintf_free(jsstack);
}

#define LOG_FUNCTION_AND_JS_STACK() \
  LogFunctionAndJSStack(__PRETTY_FUNCTION__);
#else
#define LOG_FUNCTION_AND_JS_STACK()
#endif

namespace mozilla {
namespace dom {
namespace power {

using namespace hal;

NS_IMPL_ISUPPORTS(PowerManagerService, nsIPowerManagerService)

 StaticRefPtr<PowerManagerService> PowerManagerService::sSingleton;

 already_AddRefed<PowerManagerService>
PowerManagerService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new PowerManagerService();
    sSingleton->Init();
    ClearOnShutdown(&sSingleton);
  }

  nsRefPtr<PowerManagerService> service = sSingleton.get();
  return service.forget();
}

void
PowerManagerService::Init()
{
  RegisterWakeLockObserver(this);

  
  
  
  mWatchdogTimeoutSecs =
    Preferences::GetInt("shutdown.watchdog.timeoutSecs", 10);
}

PowerManagerService::~PowerManagerService()
{
  UnregisterWakeLockObserver(this);
}

void
PowerManagerService::ComputeWakeLockState(const WakeLockInformation& aWakeLockInfo,
                                          nsAString &aState)
{
  WakeLockState state = hal::ComputeWakeLockState(aWakeLockInfo.numLocks(),
                                                  aWakeLockInfo.numHidden());
  switch (state) {
  case WAKE_LOCK_STATE_UNLOCKED:
    aState.AssignLiteral("unlocked");
    break;
  case WAKE_LOCK_STATE_HIDDEN:
    aState.AssignLiteral("locked-background");
    break;
  case WAKE_LOCK_STATE_VISIBLE:
    aState.AssignLiteral("locked-foreground");
    break;
  }
}

void
PowerManagerService::Notify(const WakeLockInformation& aWakeLockInfo)
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
    obsServ->NotifyObservers(nullptr, "profile-before-change2", context.get());
  }
}

NS_IMETHODIMP
PowerManagerService::Reboot()
{
  LOG_FUNCTION_AND_JS_STACK() 

  StartForceQuitWatchdog(eHalShutdownMode_Reboot, mWatchdogTimeoutSecs);
  
  SyncProfile();
  hal::Reboot();
  MOZ_CRASH("hal::Reboot() shouldn't return");
}

NS_IMETHODIMP
PowerManagerService::PowerOff()
{
  LOG_FUNCTION_AND_JS_STACK() 

  StartForceQuitWatchdog(eHalShutdownMode_PowerOff, mWatchdogTimeoutSecs);
  
  SyncProfile();
  hal::PowerOff();
  MOZ_CRASH("hal::PowerOff() shouldn't return");
}

NS_IMETHODIMP
PowerManagerService::Restart()
{
  LOG_FUNCTION_AND_JS_STACK() 

  
  
  
  StartForceQuitWatchdog(eHalShutdownMode_Restart, mWatchdogTimeoutSecs);
  
  
  
  
  ContentParent::JoinAllSubprocesses();

  
  SyncProfile();
#ifdef XP_UNIX
  sync();
#endif
  _exit(0);
  MOZ_CRASH("_exit() shouldn't return");
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
  WakeLockInformation info;
  GetWakeLockInfo(aTopic, &info);

  ComputeWakeLockState(info, aState);

  return NS_OK;
}

already_AddRefed<WakeLock>
PowerManagerService::NewWakeLock(const nsAString& aTopic,
                                 nsIDOMWindow* aWindow,
                                 mozilla::ErrorResult& aRv)
{
  nsRefPtr<WakeLock> wakelock = new WakeLock();
  aRv = wakelock->Init(aTopic, aWindow);
  if (aRv.Failed()) {
    return nullptr;
  }

  return wakelock.forget();
}

NS_IMETHODIMP
PowerManagerService::NewWakeLock(const nsAString &aTopic,
                                 nsIDOMWindow *aWindow,
                                 nsISupports **aWakeLock)
{
  mozilla::ErrorResult rv;
  nsRefPtr<WakeLock> wakelock = NewWakeLock(aTopic, aWindow, rv);
  if (rv.Failed()) {
    return rv.StealNSResult();
  }

  nsCOMPtr<nsIDOMEventListener> eventListener = wakelock.get();
  eventListener.forget(aWakeLock);
  return NS_OK;
}

already_AddRefed<WakeLock>
PowerManagerService::NewWakeLockOnBehalfOfProcess(const nsAString& aTopic,
                                                  ContentParent* aContentParent)
{
  nsRefPtr<WakeLock> wakelock = new WakeLock();
  nsresult rv = wakelock->Init(aTopic, aContentParent);
  NS_ENSURE_SUCCESS(rv, nullptr);
  return wakelock.forget();
}

} 
} 
} 
