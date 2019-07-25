



#ifndef mozilla_system_automounter_h__
#define mozilla_system_automounter_h__

#include "mozilla/StandardInteger.h"

namespace mozilla {
namespace system {


#define AUTOMOUNTER_DISABLE                 0
#define AUTOMOUNTER_ENABLE                  1
#define AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED  2










void InitAutoMounter();







void SetAutoMounterMode(int32_t aMode);






void ShutdownAutoMounter();

} 
} 

#endif  
