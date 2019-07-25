






































#ifndef mozilla_Hal_h
#define mozilla_Hal_h 1

#include "base/basictypes.h"
#include "mozilla/Types.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "prlog.h"
#include "mozilla/dom/battery/Types.h"

#ifndef MOZ_HAL_NAMESPACE

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





















class WindowIdentifier
{
public:
  





  WindowIdentifier();

  


  WindowIdentifier(const WindowIdentifier& other);

  







  WindowIdentifier(nsIDOMWindow* window);
  WindowIdentifier(nsCOMPtr<nsIDOMWindow> &window);

  




  WindowIdentifier(const nsTArray<uint64>& id, nsIDOMWindow* window);

  


  typedef InfallibleTArray<uint64> IDArrayType;
  const IDArrayType& AsArray() const;

  



  void AppendProcessID();

  




  bool HasTraveledThroughIPC() const;

  


  nsIDOMWindow* GetWindow() const;

private:
  


  uint64 GetWindowID() const;

  AutoInfallibleTArray<uint64, 3> mID;
  nsCOMPtr<nsIDOMWindow> mWindow;
  bool mIsEmpty;
};

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
             const hal::WindowIdentifier &id);














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
