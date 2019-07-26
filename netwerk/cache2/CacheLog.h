



#ifndef CacheLog__h__
#define CacheLog__h__

#define FORCE_PR_LOG

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
