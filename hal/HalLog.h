





#ifndef mozilla_HalLog_h
#define mozilla_HalLog_h

#include "mozilla/Logging.h"








namespace mozilla {

namespace hal {

extern PRLogModuleInfo *GetHalLog();
#define HAL_LOG(...) \
  MOZ_LOG(mozilla::hal::GetHalLog(), LogLevel::Debug, (__VA_ARGS__))
#define HAL_ERR(...) \
  MOZ_LOG(mozilla::hal::GetHalLog(), LogLevel::Error, (__VA_ARGS__))

} 

} 

#endif 
