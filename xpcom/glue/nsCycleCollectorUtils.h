



































#ifndef nsCycleCollectorUtils_h__
#define nsCycleCollectorUtils_h__

#include "nscore.h"
#include "mozilla/threads/nsThreadIDs.h"

#if defined(MOZILLA_INTERNAL_API)
#define NS_IsCycleCollectorThread NS_IsCycleCollectorThread_P
#if defined(XP_WIN)
NS_COM bool NS_IsCycleCollectorThread();
#elif defined(NS_TLS)

extern NS_TLS mozilla::threads::ID gTLSThreadID;
inline bool NS_IsCycleCollectorThread()
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
