





#ifndef MainThreadUtils_h_
#define MainThreadUtils_h_

#include "nscore.h"
#include "mozilla/threads/nsThreadIDs.h"

class nsIThread;







extern NS_COM_GLUE NS_METHOD
NS_GetMainThread(nsIThread **result);

#if defined(MOZILLA_INTERNAL_API) && defined(XP_WIN)
bool NS_IsMainThread();
#elif defined(MOZILLA_INTERNAL_API) && defined(NS_TLS)


extern NS_TLS mozilla::threads::ID gTLSThreadID;
#ifdef MOZ_ASAN

MOZ_ASAN_BLACKLIST bool NS_IsMainThread();
#else
inline
bool NS_IsMainThread()
{
  return gTLSThreadID == mozilla::threads::Main;
}
#endif
#else






extern NS_COM_GLUE bool NS_IsMainThread();
#endif

#endif 
