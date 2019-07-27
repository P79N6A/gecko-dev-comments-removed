





#ifndef mozilla_Hal_h
#define mozilla_Hal_h

#include "mozilla/hal_sandbox/PHal.h"
#include "mozilla/HalTypes.h"
#include "base/basictypes.h"
#include "mozilla/Observer.h"
#include "mozilla/Types.h"
#include "nsTArray.h"
#include "mozilla/dom/MozPowerManagerBinding.h"
#include "mozilla/dom/battery/Types.h"
#include "mozilla/dom/network/Types.h"
#include "mozilla/dom/power/Types.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "mozilla/HalScreenConfiguration.h"











class nsIDOMWindow;

#ifndef MOZ_HAL_NAMESPACE
# define MOZ_HAL_NAMESPACE hal
# define MOZ_DEFINED_HAL_NAMESPACE 1
#endif

namespace mozilla {

namespace hal {

typedef Observer<void_t> AlarmObserver;

class WindowIdentifier;

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






void SetScreenEnabled(bool aEnabled);




bool GetKeyLightEnabled();




void SetKeyLightEnabled(bool aEnabled);








double GetScreenBrightness();











void SetScreenBrightness(double aBrightness);




bool GetCpuSleepAllowed();





void SetCpuSleepAllowed(bool aAllowed);







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





int32_t GetTimezoneOffset();





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
                    uint64_t aProcessID = hal::CONTENT_PROCESS_ID_UNKNOWN);






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




void NotifySwitchStateFromInputDevice(hal::SwitchDevice aDevice,
                                      hal::SwitchState aState);







bool RegisterTheOneAlarmObserver(hal::AlarmObserver* aObserver);





void UnregisterTheOneAlarmObserver();






void NotifyAlarmFired();













bool SetAlarm(int32_t aSeconds, int32_t aNanoseconds);








void SetProcessPriority(int aPid,
                        hal::ProcessPriority aPriority,
                        uint32_t aLRU = 0);







void SetCurrentThreadPriority(hal::ThreadPriority aThreadPriority);




void RegisterFMRadioObserver(hal::FMRadioObserver* aRadioObserver);




void UnregisterFMRadioObserver(hal::FMRadioObserver* aRadioObserver);




void RegisterFMRadioRDSObserver(hal::FMRadioRDSObserver* aRDSObserver);




void UnregisterFMRadioRDSObserver(hal::FMRadioRDSObserver* aRDSObserver);





void NotifyFMRadioStatus(const hal::FMRadioOperationInformation& aRadioState);





void NotifyFMRadioRDSGroup(const hal::FMRadioRDSGroup& aRDSGroup);




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




bool EnableRDS(uint32_t aMask);




void DisableRDS();








void StartForceQuitWatchdog(hal::ShutdownMode aMode, int32_t aTimeoutSecs);




void FactoryReset(mozilla::dom::FactoryResetReason& aReason);






void StartDiskSpaceWatcher();






void StopDiskSpaceWatcher();






uint32_t GetTotalSystemMemory();







uint32_t GetTotalSystemMemoryLevel();




bool IsHeadphoneEventFromInputDev();

} 
} 

#ifdef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_HAL_NAMESPACE
#endif

#endif  
