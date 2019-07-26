





#ifndef mozilla_HalInternal_h
#define mozilla_HalInternal_h 1











#ifndef MOZ_HAL_NAMESPACE
# error "You shouldn't directly include HalInternal.h!"
#endif

namespace mozilla {
namespace MOZ_HAL_NAMESPACE {




void EnableBatteryNotifications();




void DisableBatteryNotifications();




void EnableNetworkNotifications();




void DisableNetworkNotifications();




void EnableScreenConfigurationNotifications();




void DisableScreenConfigurationNotifications();




void EnableSwitchNotifications(hal::SwitchDevice aDevice);




void DisableSwitchNotifications(hal::SwitchDevice aDevice);




bool EnableAlarm();




void DisableAlarm();




void EnableSystemClockChangeNotifications();




void DisableSystemClockChangeNotifications();




void EnableSystemTimezoneChangeNotifications();




void DisableSystemTimezoneChangeNotifications();





bool HalChildDestroyed();
} 
} 

#endif  
