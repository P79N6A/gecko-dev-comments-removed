



#include "base/system_monitor.h"
#include "base/logging.h"
#include "base/message_loop.h"

namespace base {

#if defined(ENABLE_BATTERY_MONITORING)


static int kDelayedBatteryCheckMs = 10 * 1000;
#endif  

SystemMonitor::SystemMonitor()
    : battery_in_use_(false),
      suspended_(false) {
  observer_list_ = new ObserverListThreadSafe<PowerObserver>();
}

void SystemMonitor::ProcessPowerMessage(PowerEvent event_id) {
  
  
  switch (event_id) {
    case POWER_STATE_EVENT:
      {
        bool on_battery = IsBatteryPower();
        if (on_battery != battery_in_use_) {
          battery_in_use_ = on_battery;
          NotifyPowerStateChange();
        }
      }
      break;
    case RESUME_EVENT:
      if (suspended_) {
        suspended_ = false;
        NotifyResume();
      }
      break;
    case SUSPEND_EVENT:
      if (!suspended_) {
        suspended_ = true;
        NotifySuspend();
      }
      break;
  }
}

void SystemMonitor::AddObserver(PowerObserver* obs) {
  observer_list_->AddObserver(obs);
}

void SystemMonitor::RemoveObserver(PowerObserver* obs) {
  observer_list_->RemoveObserver(obs);
}

void SystemMonitor::NotifyPowerStateChange() {
  LOG(INFO) << "PowerStateChange: "
           << (BatteryPower() ? "On" : "Off") << " battery";
  observer_list_->Notify(&PowerObserver::OnPowerStateChange, this);
}

void SystemMonitor::NotifySuspend() {
  LOG(INFO) << "Power Suspending";
  observer_list_->Notify(&PowerObserver::OnSuspend, this);
}

void SystemMonitor::NotifyResume() {
  LOG(INFO) << "Power Resuming";
  observer_list_->Notify(&PowerObserver::OnResume, this);
}

void SystemMonitor::Start() {
#if defined(ENABLE_BATTERY_MONITORING)
  DCHECK(MessageLoop::current());  
  SystemMonitor* monitor = Get();
  monitor->delayed_battery_check_.Start(
      TimeDelta::FromMilliseconds(kDelayedBatteryCheckMs), monitor,
      &SystemMonitor::BatteryCheck);
#endif  
}

void SystemMonitor::BatteryCheck() {
  ProcessPowerMessage(SystemMonitor::POWER_STATE_EVENT);
}

} 
