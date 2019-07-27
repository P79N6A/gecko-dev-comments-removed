





#ifndef mozilla_HalLog_h
#define mozilla_HalLog_h

#include "mozilla/Logging.h"








namespace mozilla {

namespace hal {

extern PRLogModuleInfo *GetHalLog();
#define HAL_LOG(...) \
  PR_LOG(mozilla::hal::GetHalLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#define HAL_ERR(...) \
  PR_LOG(mozilla::hal::GetHalLog(), PR_LOG_ERROR, (__VA_ARGS__))

} 

} 

#endif 
