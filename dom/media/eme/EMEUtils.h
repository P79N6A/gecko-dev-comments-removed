





#ifndef EME_LOG_H_
#define EME_LOG_H_

#include "mozilla/Logging.h"
#include "nsString.h"

namespace mozilla {

#ifndef EME_LOG
  PRLogModuleInfo* GetEMELog();
  #define EME_LOG(...) MOZ_LOG(GetEMELog(), mozilla::LogLevel::Debug, (__VA_ARGS__))
  #define EME_LOG_ENABLED() MOZ_LOG_TEST(GetEMELog(), mozilla::LogLevel::Debug)
#endif

#ifndef EME_VERBOSE_LOG
  PRLogModuleInfo* GetEMEVerboseLog();
  #define EME_VERBOSE_LOG(...) MOZ_LOG(GetEMEVerboseLog(), mozilla::LogLevel::Debug, (__VA_ARGS__))
#else
  #ifndef EME_LOG
    #define EME_LOG(...)
  #endif

  #ifndef EME_VERBOSE_LOG
    #define EME_VERBOSE_LOG(...)
  #endif
#endif

#define NO_CDM_VERSION -1















bool ParseKeySystem(const nsAString& aKeySystem,
                    nsAString& aOutKeySystem,
                    int32_t& aOutMinCDMVersion);

} 

#endif 
