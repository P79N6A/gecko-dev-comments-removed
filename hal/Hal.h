





#ifndef mozilla_Hal_h
#define mozilla_Hal_h

#include "mozilla/hal_sandbox/PHal.h"
#include "base/basictypes.h"
#include "mozilla/Types.h"
#include "nsTArray.h"
#include "prlog.h"
#include "mozilla/dom/battery/Types.h"
#include "mozilla/dom/network/Types.h"
#include "mozilla/dom/power/Types.h"
#include "mozilla/hal_sandbox/PHal.h"
#include "mozilla/dom/ScreenOrientation.h"











class nsIDOMWindow;

#ifndef MOZ_HAL_NAMESPACE
# define MOZ_HAL_NAMESPACE hal
# define MOZ_DEFINED_HAL_NAMESPACE 1
#endif

namespace mozilla {

template <class T>
class Observer;

namespace hal {

typedef Observer<void_t> AlarmObserver;
typedef Observer<ScreenConfiguration> ScreenConfigurationObserver;

class WindowIdentifier;

extern PRLogModuleInfo *sHalLog;
#define HAL_LOG(msg) PR_LOG(mozilla::hal::sHalLog, PR_LOG_DEBUG, msg)

} 

namespace MOZ_HAL_NAMESPACE {
















void Vibrate(const nsTArray<uint32_t>& pattern,
             nsIDOMWindow* aWindow);
void Vibrate(const nsTArray<uint32_t>& pattern,
             const hal::WindowIdentifier &id);













void CancelVibrate(nsIDOMWindow* aWindow);
void CancelVibrate(const hal::WindowIdentifier &id);





void RegisterBatteryObserver(BatteryObserver* aBatteryObserver);





void UnregisterBatteryObserver(BatteryObserver* aBatteryObserver);




void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo);





void NotifyBatteryChange(const hal::BatteryInformation& aBatteryInfo);




bool GetScreenEnabled();






void SetScreenEnabled(bool enabled);








double GetScreenBrightness();











void SetScreenBrightness(double brightness);




bool GetCpuSleepAllowed();





void SetCpuSleepAllowed(bool allowed);











bool SetLight(hal::LightType light, const hal::LightConfiguration& aConfig);




bool GetLight(hal::LightType light, hal::LightConfiguration* aConfig);








void RegisterSensorObserver(hal::SensorType aSensor,
                            hal::ISensorObserver *aObserver);




void UnregisterSensorObserver(hal::SensorType aSensor,
                              hal::ISensorObserver *aObserver);






void NotifySensorChange(const hal::SensorData &aSensorData);







void EnableSensorNotifications(hal::SensorType aSensor);







void DisableSensorNotifications(hal::SensorType aSensor);






void RegisterNetworkObserver(NetworkObserver* aNetworkObserver);





void UnregisterNetworkObserver(NetworkObserver* aNetworkObserver);




void GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo);





void NotifyNetworkChange(const hal::NetworkInformation& aNetworkInfo);





void AdjustSystemClock(int32_t aDeltaMilliseconds);






void SetTimezone(const nsCString& aTimezoneSpec);




void Reboot();




void PowerOff();






void EnableWakeLockNotifications();






void DisableWakeLockNotifications();





void RegisterWakeLockObserver(WakeLockObserver* aObserver);





void UnregisterWakeLockObserver(WakeLockObserver* aObserver);







void ModifyWakeLock(const nsAString &aTopic,
                    hal::WakeLockControl aLockAdjust,
                    hal::WakeLockControl aHiddenAdjust);






void GetWakeLockInfo(const nsAString &aTopic, hal::WakeLockInformation *aWakeLockInfo);





void NotifyWakeLockChange(const hal::WakeLockInformation& aWakeLockInfo);





void RegisterScreenConfigurationObserver(hal::ScreenConfigurationObserver* aScreenConfigurationObserver);





void UnregisterScreenConfigurationObserver(hal::ScreenConfigurationObserver* aScreenConfigurationObserver);




void GetCurrentScreenConfiguration(hal::ScreenConfiguration* aScreenConfiguration);





void NotifyScreenConfigurationChange(const hal::ScreenConfiguration& aScreenConfiguration);





bool LockScreenOrientation(const dom::ScreenOrientation& aOrientation);




void UnlockScreenOrientation();







void RegisterSwitchObserver(hal::SwitchDevice aDevice, hal::SwitchObserver *aSwitchObserver);




void UnregisterSwitchObserver(hal::SwitchDevice aDevice, hal::SwitchObserver *aSwitchObserver);






void NotifySwitchChange(const hal::SwitchEvent& aEvent);




hal::SwitchState GetCurrentSwitchState(hal::SwitchDevice aDevice);







bool RegisterTheOneAlarmObserver(hal::AlarmObserver* aObserver);





void UnregisterTheOneAlarmObserver();






void NotifyAlarmFired();













bool SetAlarm(int32_t aSeconds, int32_t aNanoseconds);








void SetProcessPriority(int aPid, hal::ProcessPriority aPriority);

} 
} 

#ifdef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_HAL_NAMESPACE
#endif

#endif  
