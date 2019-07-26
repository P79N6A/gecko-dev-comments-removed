





#include "prlog.h"

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

#endif 
} 
