



#ifndef CacheLog__h__
#define CacheLog__h__

#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

#if defined(PR_LOG)
#error "If nsCache.h #included it must come before any files that #include prlog.h"
#endif

#include "prlog.h"

namespace mozilla {
namespace net {

#if defined(PR_LOGGING)
extern PRLogModuleInfo* GetCache2Log();
#define LOG(x)  PR_LOG(GetCache2Log(), PR_LOG_DEBUG, x)
#else
#define LOG(x)
#endif 

} 
} 

#endif
