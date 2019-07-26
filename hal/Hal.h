





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
#include "mozilla/dom/ContentParent.h"
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

extern PRLogModuleInfo *GetHalLog();
#define HAL_LOG(msg) PR_LOG(mozilla::hal::GetHalLog(), PR_LOG_DEBUG, msg)

typedef Observer<int64_t> SystemClockChangeObserver;
typedef Observer<SystemTimezoneChangeInformation> SystemTimezoneChangeObserver;

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





void AdjustSystemClock(int64_t aDeltaMilliseconds);






void SetTimezone(const nsCString& aTimezoneSpec);





nsCString GetTimezone();





void RegisterSystemClockChangeObserver(
  hal::SystemClockChangeObserver* aObserver);





void UnregisterSystemClockChangeObserver(
  hal::SystemClockChangeObserver* aObserver);





void NotifySystemClockChange(const int64_t& aClockDeltaMS);





void RegisterSystemTimezoneChangeObserver(
  hal::SystemTimezoneChangeObserver* aObserver);





void UnregisterSystemTimezoneChangeObserver(
  hal::SystemTimezoneChangeObserver* aObserver);





void NotifySystemTimezoneChange(
  const hal::SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo);






void Reboot();






void PowerOff();






void EnableWakeLockNotifications();






void DisableWakeLockNotifications();





void RegisterWakeLockObserver(WakeLockObserver* aObserver);





void UnregisterWakeLockObserver(WakeLockObserver* aObserver);

















void ModifyWakeLock(const nsAString &aTopic,
                    hal::WakeLockControl aLockAdjust,
                    hal::WakeLockControl aHiddenAdjust,
                    uint64_t aProcessID = CONTENT_PROCESS_ID_UNKNOWN);






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




void RegisterFMRadioObserver(hal::FMRadioObserver* aRadioObserver);




void UnregisterFMRadioObserver(hal::FMRadioObserver* aRadioObserver);





void NotifyFMRadioStatus(const hal::FMRadioOperationInformation& aRadioState);




void EnableFMRadio(const hal::FMRadioSettings& aInfo);




void DisableFMRadio();





void FMRadioSeek(const hal::FMRadioSeekDirection& aDirection);




void GetFMRadioSettings(hal::FMRadioSettings* aInfo);




void SetFMRadioFrequency(const uint32_t frequency);




uint32_t GetFMRadioFrequency();




bool IsFMRadioOn();




uint32_t GetFMRadioSignalStrength();




void CancelFMRadioSeek();




hal::FMRadioSettings GetFMBandSettings(hal::FMRadioCountry aCountry);








void StartForceQuitWatchdog(hal::ShutdownMode aMode, int32_t aTimeoutSecs);




void FactoryReset();

} 
} 

#ifdef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_HAL_NAMESPACE
#endif

#endif  
