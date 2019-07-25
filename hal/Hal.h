






































#ifndef mozilla_Hal_h
#define mozilla_Hal_h 1

#include "mozilla/hal_sandbox/PHal.h"
#include "base/basictypes.h"
#include "mozilla/Types.h"
#include "nsTArray.h"
#include "prlog.h"
#include "mozilla/dom/battery/Types.h"
#include "mozilla/dom/network/Types.h"
#include "mozilla/hal_sandbox/PHal.h"











class nsIDOMWindow;

#ifndef MOZ_HAL_NAMESPACE
# define MOZ_HAL_NAMESPACE hal
# define MOZ_DEFINED_HAL_NAMESPACE 1
#endif

namespace mozilla {

namespace hal {

class WindowIdentifier;

extern PRLogModuleInfo *sHalLog;
#define HAL_LOG(msg) PR_LOG(mozilla::hal::sHalLog, PR_LOG_DEBUG, msg)

} 

namespace MOZ_HAL_NAMESPACE {
















void Vibrate(const nsTArray<uint32>& pattern,
             nsIDOMWindow* aWindow);
void Vibrate(const nsTArray<uint32>& pattern,
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




void Reboot();




void PowerOff();

} 
} 

#ifdef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_HAL_NAMESPACE
#endif

#endif  
