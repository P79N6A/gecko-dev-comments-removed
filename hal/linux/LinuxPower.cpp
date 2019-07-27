




#include "Hal.h"

#include <unistd.h>
#include <sys/reboot.h>
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "MainThreadUtils.h"

#if defined(MOZ_WIDGET_GONK)
#include "cutils/android_reboot.h"
#include "cutils/properties.h"
#endif

namespace mozilla {
namespace hal_impl {

#if (defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 19)
static void
PowerCtl(const char* aValue, int aCmd)
{
  
  property_set("sys.powerctl", aValue);
  
  
  sleep(10);
  HAL_LOG("Powerctl call takes too long, forcing %s.", aValue);
  android_reboot(aCmd, 0, nullptr);
}
#endif

void
Reboot()
{
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIObserverService> obsServ = services::GetObserverService();
    if (obsServ) {
      obsServ->NotifyObservers(nullptr, "system-reboot", nullptr);
    }
  }

#if !defined(MOZ_WIDGET_GONK)
  sync();
  reboot(RB_AUTOBOOT);
#elif (ANDROID_VERSION < 19)
  android_reboot(ANDROID_RB_RESTART, 0, nullptr);
#else
  PowerCtl("reboot", ANDROID_RB_RESTART);
#endif
}

void
PowerOff()
{
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIObserverService> obsServ = services::GetObserverService();
    if (obsServ) {
      obsServ->NotifyObservers(nullptr, "system-power-off", nullptr);
    }
  }

#if !defined(MOZ_WIDGET_GONK)
  sync();
  reboot(RB_POWER_OFF);
#elif (ANDROID_VERSION < 19)
  android_reboot(ANDROID_RB_POWEROFF, 0, nullptr);
#else
  PowerCtl("shutdown", ANDROID_RB_POWEROFF);
#endif
}


typedef struct watchdogParam
{
  hal::ShutdownMode mode; 
  int32_t timeoutSecs;    

  watchdogParam(hal::ShutdownMode aMode, int32_t aTimeoutSecs)
    : mode(aMode), timeoutSecs(aTimeoutSecs) {}
} watchdogParam_t;


static void
QuitHard(hal::ShutdownMode aMode)
{
  switch (aMode)
  {
    case hal::eHalShutdownMode_PowerOff:
      PowerOff();
      break;
    case hal::eHalShutdownMode_Reboot:
      Reboot();
      break;
    case hal::eHalShutdownMode_Restart:
      
      kill(0, SIGKILL);
      
      
      
      
      _exit(1);
      break;
    default:
      MOZ_CRASH();
  }
}


static void*
ForceQuitWatchdog(void* aParamPtr)
{
  watchdogParam_t* paramPtr = reinterpret_cast<watchdogParam_t*>(aParamPtr);
  if (paramPtr->timeoutSecs > 0 && paramPtr->timeoutSecs <= 30) {
    
    
    TimeStamp deadline =
      (TimeStamp::Now() + TimeDuration::FromSeconds(paramPtr->timeoutSecs));
    while (true) {
      TimeDuration remaining = (deadline - TimeStamp::Now());
      int sleepSeconds = int(remaining.ToSeconds());
      if (sleepSeconds <= 0) {
        break;
      }
      sleep(sleepSeconds);
    }
  }
  hal::ShutdownMode mode = paramPtr->mode;
  delete paramPtr;
  QuitHard(mode);
  return nullptr;
}

void
StartForceQuitWatchdog(hal::ShutdownMode aMode, int32_t aTimeoutSecs)
{
  
  
  
  
  if (aTimeoutSecs <= 0) {
    return;
  }

  
  
  
  
  
  watchdogParam_t* paramPtr = new watchdogParam_t(aMode, aTimeoutSecs);
  pthread_t watchdog;
  if (pthread_create(&watchdog, nullptr,
                     ForceQuitWatchdog,
                     reinterpret_cast<void*>(paramPtr))) {
    
    delete paramPtr;
    QuitHard(aMode);
  }
  
}

} 
} 
