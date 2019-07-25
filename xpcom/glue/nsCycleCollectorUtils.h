



































#ifndef nsCycleCollectorUtils_h__
#define nsCycleCollectorUtils_h__

#include "nscore.h"
#include "mozilla/threads/nsThreadIDs.h"

#if defined(MOZILLA_INTERNAL_API) && !defined(XPCOM_MAKING_STUB)
#if defined(XP_WIN)
XPCOM_API(bool) NS_IsCycleCollectorThread();
#elif defined(NS_TLS)

extern NS_TLS mozilla::threads::ID gTLSThreadID;
inline bool NS_IsCycleCollectorThread()
{
  return gTLSThreadID == mozilla::threads::CycleCollector;
}
#else
XPCOM_API(bool) NS_IsCycleCollectorThread();
#endif
#else
XPCOM_API(bool) NS_IsCycleCollectorThread();
#endif

#endif 
