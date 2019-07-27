





#ifndef MainThreadUtils_h_
#define MainThreadUtils_h_

#include "nscore.h"
#include "mozilla/threads/nsThreadIDs.h"

class nsIThread;







extern NS_COM_GLUE NS_METHOD NS_GetMainThread(nsIThread** aResult);

#ifdef MOZILLA_INTERNAL_API




extern NS_COM_GLUE nsIThread* NS_GetCurrentThread();
#endif

#ifdef MOZILLA_INTERNAL_API
bool NS_IsMainThread();
#else






extern NS_COM_GLUE bool NS_IsMainThread();
#endif

#endif 
