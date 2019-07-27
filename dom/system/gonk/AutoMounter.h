



#ifndef mozilla_system_automounter_h__
#define mozilla_system_automounter_h__

#include <stdint.h>

class nsCString;

namespace mozilla {
namespace system {


#define AUTOMOUNTER_DISABLE                 0
#define AUTOMOUNTER_ENABLE_UMS              1
#define AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED  2
#define AUTOMOUNTER_ENABLE_MTP              3


#define AUTOMOUNTER_STATUS_DISABLED         0
#define AUTOMOUNTER_STATUS_ENABLED          1
#define AUTOMOUNTER_STATUS_FILES_OPEN       2










void
InitAutoMounter();







void
SetAutoMounterMode(int32_t aMode);




int32_t
GetAutoMounterStatus();








void
SetAutoMounterSharingMode(const nsCString& aVolumeName, bool aAllowSharing);







void
AutoMounterFormatVolume(const nsCString& aVolumeName);







void
AutoMounterMountVolume(const nsCString& aVolumeName);







void
AutoMounterUnmountVolume(const nsCString& aVolumeName);






void
ShutdownAutoMounter();

} 
} 

#endif  
