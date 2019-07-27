





#include <stdint.h>

namespace mozilla {
namespace a11y {

class ProxyAccessible;

enum EPlatformDisabledState {
  ePlatformIsForceEnabled = -1,
  ePlatformIsEnabled = 0,
  ePlatformIsDisabled = 1
};




EPlatformDisabledState PlatformDisabledState();

#ifdef MOZ_ACCESSIBILITY_ATK





void PreInit();
#endif

#if defined(MOZ_ACCESSIBILITY_ATK) || defined(XP_MACOSX)




bool ShouldA11yBeEnabled();
#endif





void PlatformInit();





void PlatformShutdown();





void ProxyCreated(ProxyAccessible*);





void ProxyDestroyed(ProxyAccessible*);




void ProxyEvent(ProxyAccessible* aTarget, uint32_t aEventType);
} 
} 

