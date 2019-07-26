



#ifndef nsCycleCollectorUtils_h__
#define nsCycleCollectorUtils_h__

#include "nscore.h"
#include "mozilla/threads/nsThreadIDs.h"

#if defined(MOZILLA_INTERNAL_API)
#if defined(XP_WIN)
bool NS_IsCycleCollectorThread();
#elif defined(NS_TLS)

extern NS_TLS mozilla::threads::ID gTLSThreadID;
#ifdef MOZ_ASAN

MOZ_ASAN_BLACKLIST static
#else
inline
#endif
bool NS_IsCycleCollectorThread()
{
  return gTLSThreadID == mozilla::threads::CycleCollector;
}
#else
NS_COM_GLUE bool NS_IsCycleCollectorThread();
#endif
#else
NS_COM_GLUE bool NS_IsCycleCollectorThread();
#endif

#endif 
