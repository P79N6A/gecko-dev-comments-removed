



#ifndef BASE_SYSTEM_MONITOR_H_
#define BASE_SYSTEM_MONITOR_H_

#include "base/observer_list_threadsafe.h"
#include "base/singleton.h"



#if defined(OS_WIN)
#define ENABLE_BATTERY_MONITORING 1
#else
#undef ENABLE_BATTERY_MONITORING
#endif  

namespace base {




class SystemMonitor {
 public:
  
  static SystemMonitor* Get() {
    
    return
        Singleton<SystemMonitor, LeakySingletonTraits<SystemMonitor> >::get();
  }

  
  
  
  
  
  
  static void Start();

  
  
  

  
  
  bool BatteryPower() {
    
    return battery_in_use_;
  }

  
  enum PowerEvent {
    POWER_STATE_EVENT,  
    SUSPEND_EVENT,      
    RESUME_EVENT        
  };

  
  
  
  
  
  class PowerObserver {
  public:
    
    
    virtual void OnPowerStateChange(SystemMonitor*) = 0;

    
    virtual void OnSuspend(SystemMonitor*) = 0;

    
    virtual void OnResume(SystemMonitor*) = 0;
  };

  
  
  
  void AddObserver(PowerObserver* obs);

  
  
  
  void RemoveObserver(PowerObserver* obs);

#if defined(OS_WIN)
  
  
  
  void ProcessWmPowerBroadcastMessage(int event_id);
#endif

  
  void ProcessPowerMessage(PowerEvent event_id);

  
  
  SystemMonitor();

 private:
  
  
  
  bool IsBatteryPower();

  
  
  void BatteryCheck();

  
  void NotifyPowerStateChange();
  void NotifySuspend();
  void NotifyResume();

  scoped_refptr<ObserverListThreadSafe<PowerObserver> > observer_list_;
  bool battery_in_use_;
  bool suspended_;

#if defined(ENABLE_BATTERY_MONITORING)
  base::OneShotTimer<SystemMonitor> delayed_battery_check_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SystemMonitor);
};

}

#endif  
