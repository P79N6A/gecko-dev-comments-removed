





#include <stdint.h>

class nsString;

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





void ProxyCreated(ProxyAccessible* aProxy, uint32_t aInterfaces);





void ProxyDestroyed(ProxyAccessible*);




void ProxyEvent(ProxyAccessible* aTarget, uint32_t aEventType);
void ProxyStateChangeEvent(ProxyAccessible* aTarget, uint64_t aState,
                           bool aEnabled);
void ProxyCaretMoveEvent(ProxyAccessible* aTarget, int32_t aOffset);
void ProxyTextChangeEvent(ProxyAccessible* aTarget, const nsString& aStr,
                          int32_t aStart, uint32_t aLen, bool aIsInsert,
                          bool aFromUser);
} 
} 

