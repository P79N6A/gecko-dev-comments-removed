






































#ifndef mozilla_Hal_h
#define mozilla_Hal_h 1

#include "base/basictypes.h"
#include "mozilla/Types.h"
#include "nsTArray.h"
#include "prlog.h"
#include "mozilla/dom/battery/Types.h"

#ifndef MOZ_HAL_NAMESPACE

class nsIDOMWindow;

namespace mozilla {
namespace dom {
class TabChild;
class PBrowserChild;
}
}



namespace mozilla {
namespace hal {

extern PRLogModuleInfo *sHalLog;
#define HAL_LOG(msg) PR_LOG(sHalLog, PR_LOG_DEBUG, msg)

class WindowIdentifier;

} 
} 












# include "HalImpl.h"
# include "HalSandbox.h"
# define MOZ_HAL_NAMESPACE hal
# define MOZ_DEFINED_HAL_NAMESPACE 1
#endif

namespace mozilla {

namespace hal {
class BatteryInformation;
} 

namespace MOZ_HAL_NAMESPACE  {
















void Vibrate(const nsTArray<uint32>& pattern,
             nsIDOMWindow* aWindow);
void Vibrate(const nsTArray<uint32>& pattern,
             const hal::WindowIdentifier &id);













void CancelVibrate(nsIDOMWindow* aWindow);
void CancelVibrate(const hal::WindowIdentifier &id);





void RegisterBatteryObserver(BatteryObserver* aBatteryObserver);





void UnregisterBatteryObserver(BatteryObserver* aBatteryObserver);








void EnableBatteryNotifications();








void DisableBatteryNotifications();




void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo);





void NotifyBatteryChange(const hal::BatteryInformation& aBatteryInfo);




bool GetScreenEnabled();






void SetScreenEnabled(bool enabled);








double GetScreenBrightness();











void SetScreenBrightness(double brightness);

} 
} 

#ifdef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_HAL_NAMESPACE
#endif

#endif  
