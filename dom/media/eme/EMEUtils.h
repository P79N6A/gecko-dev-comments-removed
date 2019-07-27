





#ifndef EME_LOG_H_
#define EME_LOG_H_

#include "prlog.h"
#include "nsString.h"

namespace mozilla {

#ifdef PR_LOGGING

  #ifndef EME_LOG
    PRLogModuleInfo* GetEMELog();
    #define EME_LOG(...) PR_LOG(GetEMELog(), PR_LOG_DEBUG, (__VA_ARGS__))
  #endif

  #ifndef EME_VERBOSE_LOG
    PRLogModuleInfo* GetEMEVerboseLog();
    #define EME_VERBOSE_LOG(...) PR_LOG(GetEMEVerboseLog(), PR_LOG_DEBUG, (__VA_ARGS__))
  #else
    #ifndef EME_LOG
      #define EME_LOG(...)
    #endif

    #ifndef EME_VERBOSE_LOG
      #define EME_VERBOSE_LOG(...)
    #endif
  #endif

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

bool GetEMEVoucherPath(nsIFile** aPath);

bool EMEVoucherFileExists();

} 

#endif 
