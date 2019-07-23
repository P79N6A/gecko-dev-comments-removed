





































#include "nsThreadUtils.h"

#ifdef MOZILLA_INTERNAL_API
# include "nsThreadManager.h"
#else
# include "nsXPCOMCIDInternal.h"
# include "nsIThreadManager.h"
# include "nsServiceManagerUtils.h"
#endif

#ifdef XP_WIN
#include <windows.h>
#endif

#ifndef XPCOM_GLUE_AVOID_NSPR

NS_IMPL_THREADSAFE_ISUPPORTS1(nsRunnable, nsIRunnable)
  
NS_IMETHODIMP
nsRunnable::Run()
{
  
  return NS_OK;
}

#endif  



NS_METHOD
NS_NewThread(nsIThread **result, nsIRunnable *event)
{
  nsCOMPtr<nsIThread> thread;
#ifdef MOZILLA_INTERNAL_API
  nsresult rv = nsThreadManager::get()->
      nsThreadManager::NewThread(0, getter_AddRefs(thread));
#else
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mgr->NewThread(0, getter_AddRefs(thread));
#endif
  NS_ENSURE_SUCCESS(rv, rv);

  if (event) {
    rv = thread->Dispatch(event, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *result = nsnull;
  thread.swap(*result);
  return NS_OK;
}

NS_METHOD
NS_GetCurrentThread(nsIThread **result)
{
#ifdef MOZILLA_INTERNAL_API
  return nsThreadManager::get()->nsThreadManager::GetCurrentThread(result);
#else
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return mgr->GetCurrentThread(result);
#endif
}

NS_METHOD
NS_GetMainThread(nsIThread **result)
{
#ifdef MOZILLA_INTERNAL_API
  return nsThreadManager::get()->nsThreadManager::GetMainThread(result);
#else
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return mgr->GetMainThread(result);
#endif
}

#ifndef MOZILLA_INTERNAL_API
bool NS_IsMainThread()
{
  PRBool result = PR_FALSE;
  nsCOMPtr<nsIThreadManager> mgr =
    do_GetService(NS_THREADMANAGER_CONTRACTID);
  if (mgr)
    mgr->GetIsMainThread(&result);
  return bool(result);
}
#elif defined(XP_WIN)
extern DWORD gTLSIsMainThreadIndex;
bool
NS_IsMainThread()
{
  return !!TlsGetValue(gTLSIsMainThreadIndex);
}
#elif !defined(NS_TLS)
bool NS_IsMainThread()
{
  PRBool result = PR_FALSE;
  nsThreadManager::get()->nsThreadManager::GetIsMainThread(&result);
  return bool(result);
}
#elif !defined(MOZ_ENABLE_LIBXUL)
bool NS_IsMainThread()
{
  return gTLSIsMainThread;
}
#endif

NS_METHOD
NS_DispatchToCurrentThread(nsIRunnable *event)
{
#ifdef MOZILLA_INTERNAL_API
  nsIThread *thread = NS_GetCurrentThread();
  if (!thread) { return NS_ERROR_UNEXPECTED; }
#else
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_GetCurrentThread(getter_AddRefs(thread));
  NS_ENSURE_SUCCESS(rv, rv);
#endif
  return thread->Dispatch(event, NS_DISPATCH_NORMAL);
}

NS_METHOD
NS_DispatchToMainThread(nsIRunnable *event, PRUint32 dispatchFlags)
{
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_GetMainThread(getter_AddRefs(thread));
  NS_ENSURE_SUCCESS(rv, rv);
  return thread->Dispatch(event, dispatchFlags);
}

#ifndef XPCOM_GLUE_AVOID_NSPR
NS_METHOD
NS_ProcessPendingEvents(nsIThread *thread, PRIntervalTime timeout)
{
  nsresult rv = NS_OK;

#ifdef MOZILLA_INTERNAL_API
  if (!thread) {
    thread = NS_GetCurrentThread();
    NS_ENSURE_STATE(thread);
  }
#else
  nsCOMPtr<nsIThread> current;
  if (!thread) {
    rv = NS_GetCurrentThread(getter_AddRefs(current));
    NS_ENSURE_SUCCESS(rv, rv);
    thread = current.get();
  }
#endif

  PRIntervalTime start = PR_IntervalNow();
  for (;;) {
    PRBool processedEvent;
    rv = thread->ProcessNextEvent(PR_FALSE, &processedEvent);
    if (NS_FAILED(rv) || !processedEvent)
      break;
    if (PR_IntervalNow() - start > timeout)
      break;
  }
  return rv;
}
#endif 

inline PRBool
hasPendingEvents(nsIThread *thread)
{
  PRBool val;
  return NS_SUCCEEDED(thread->HasPendingEvents(&val)) && val;
}

PRBool
NS_HasPendingEvents(nsIThread *thread)
{
  if (!thread) {
#ifndef MOZILLA_INTERNAL_API
    nsCOMPtr<nsIThread> current;
    NS_GetCurrentThread(getter_AddRefs(current));
    return hasPendingEvents(current);
#else
    thread = NS_GetCurrentThread();
    NS_ENSURE_TRUE(thread, PR_FALSE);
#endif
  }
  return hasPendingEvents(thread);
}

PRBool
NS_ProcessNextEvent(nsIThread *thread, PRBool mayWait)
{
#ifdef MOZILLA_INTERNAL_API
  if (!thread) {
    thread = NS_GetCurrentThread();
    NS_ENSURE_TRUE(thread, PR_FALSE);
  }
#else
  nsCOMPtr<nsIThread> current;
  if (!thread) {
    NS_GetCurrentThread(getter_AddRefs(current));
    NS_ENSURE_TRUE(current, PR_FALSE);
    thread = current.get();
  }
#endif
  PRBool val;
  return NS_SUCCEEDED(thread->ProcessNextEvent(mayWait, &val)) && val;
}

#ifdef MOZILLA_INTERNAL_API
nsIThread *
NS_GetCurrentThread() {
  return nsThreadManager::get()->GetCurrentThread();
}
#endif
