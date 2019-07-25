



































#include "nsCycleCollectorUtils.h"

#include "prthread.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsXPCOMCIDInternal.h"

#include "nsIThreadManager.h"

#if defined(XP_WIN)
#include "windows.h"
#endif

#ifndef MOZILLA_INTERNAL_API

bool
NS_IsCycleCollectorThread()
{
  PRBool result = PR_FALSE;
  nsCOMPtr<nsIThreadManager> mgr =
    do_GetService(NS_THREADMANAGER_CONTRACTID);
  if (mgr)
    mgr->GetIsCycleCollectorThread(&result);
  return bool(result);
}

#elif defined(XP_WIN)


extern DWORD gTLSThreadIDIndex;

bool
NS_IsCycleCollectorThread()
{
  return TlsGetValue(gTLSThreadIDIndex) ==
    (void*) mozilla::threads::CycleCollector;
}

#elif !defined(NS_TLS)


extern PRThread* gCycleCollectorThread;

bool
NS_IsCycleCollectorThread()
{
  return PR_GetCurrentThread() == gCycleCollectorThread;
}

#endif
