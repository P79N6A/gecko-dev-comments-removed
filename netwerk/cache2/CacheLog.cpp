



#include "CacheLog.h"

namespace mozilla {
namespace net {










PRLogModuleInfo* GetCache2Log()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("cache2");
  return sLog;
}

} 
} 
