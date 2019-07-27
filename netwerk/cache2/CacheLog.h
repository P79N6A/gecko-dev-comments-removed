



#ifndef Cache2Log__h__
#define Cache2Log__h__

#include "mozilla/Logging.h"

namespace mozilla {
namespace net {

extern PRLogModuleInfo* GetCache2Log();
#define LOG(x)  PR_LOG(GetCache2Log(), PR_LOG_DEBUG, x)
#define LOG_ENABLED() PR_LOG_TEST(GetCache2Log(), PR_LOG_DEBUG)

} 
} 

#endif
