



#include "base/system_monitor.h"

namespace base {

void SystemMonitor::ProcessWmPowerBroadcastMessage(int event_id) {
  PowerEvent power_event;
  switch (event_id) {
    case PBT_APMPOWERSTATUSCHANGE:  
      power_event = POWER_STATE_EVENT;
      break;
    case PBT_APMRESUMEAUTOMATIC:  
    case PBT_APMRESUMESUSPEND:    
      power_event = RESUME_EVENT;
      break;
    case PBT_APMSUSPEND:  
      power_event = SUSPEND_EVENT;
      break;
    default:
      return;

    
    
    
    
    
    
    
  }
  ProcessPowerMessage(power_event);
}



bool SystemMonitor::IsBatteryPower() {
  SYSTEM_POWER_STATUS status;
  if (!GetSystemPowerStatus(&status)) {
    LOG(ERROR) << "GetSystemPowerStatus failed: " << GetLastError();
    return false;
  }
  return (status.ACLineStatus == 0);
}



} 
