




#include "Hal.h"

#include <unistd.h>
#include <sys/reboot.h>
#include "nsIObserverService.h"

namespace mozilla {
namespace hal_impl {

void
Reboot()
{
  nsCOMPtr<nsIObserverService> obsServ = services::GetObserverService();
  if (obsServ) {
    obsServ->NotifyObservers(nullptr, "system-reboot", nullptr);
  }
  sync();
  reboot(RB_AUTOBOOT);
}

void
PowerOff()
{
  nsCOMPtr<nsIObserverService> obsServ = services::GetObserverService();
  if (obsServ) {
    obsServ->NotifyObservers(nullptr, "system-power-off", nullptr);
  }
  sync();
  reboot(RB_POWER_OFF);
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
      MOZ_NOT_REACHED();
      break;
  }
  MOZ_NOT_REACHED();
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
