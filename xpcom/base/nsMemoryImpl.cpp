




































#include "nsXPCOM.h"
#include "nsMemoryImpl.h"
#include "nsThreadUtils.h"

#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"

#include "prmem.h"
#include "prcvar.h"
#include "pratom.h"

#include "nsAlgorithm.h"
#include "nsAutoLock.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#if defined(XP_WIN)
#include <windows.h>
#endif

#if defined (NS_OSSO)
#include <osso-mem.h>
#include <fcntl.h>
#include <unistd.h>
const char* kHighMark = "/sys/kernel/high_watermark";
#endif





#ifdef WINCE
#define NOTIFY_LOW_MEMORY
#endif

#ifdef WINCE_WINDOWS_MOBILE
#include "aygshell.h"
#endif

#include "nsITimer.h"

static nsMemoryImpl sGlobalMemory;

NS_IMPL_QUERY_INTERFACE1(nsMemoryImpl, nsIMemory)

NS_IMETHODIMP_(void*)
nsMemoryImpl::Alloc(PRSize size)
{
    return NS_Alloc(size);
}

NS_IMETHODIMP_(void*)
nsMemoryImpl::Realloc(void* ptr, PRSize size)
{
    return NS_Realloc(ptr, size);
}

NS_IMETHODIMP_(void)
nsMemoryImpl::Free(void* ptr)
{
    NS_Free(ptr);
}

NS_IMETHODIMP
nsMemoryImpl::HeapMinimize(PRBool aImmediate)
{
    return FlushMemory(NS_LITERAL_STRING("heap-minimize").get(), aImmediate);
}





static const int kRequiredMemory = 0x3000000;

NS_IMETHODIMP
nsMemoryImpl::IsLowMemory(PRBool *result)
{
#if defined(WINCE_WINDOWS_MOBILE)
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    *result = (stat.dwMemoryLoad >= 98);
#elif defined(WINCE)
    
    *result = PR_FALSE;
#elif defined(XP_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof stat;
    GlobalMemoryStatusEx(&stat);
    *result = (stat.ullAvailPageFile < kRequiredMemory) &&
        ((float)stat.ullAvailPageFile / stat.ullTotalPageFile) < 0.1;
#elif defined(NS_OSSO)
    int fd = open (kHighMark, O_RDONLY);
    if (fd == -1) {
        *result = PR_FALSE;
        return NS_OK;
    }
    int c = 0;
    read (fd, &c, 1);
    close(fd);
    *result = (c == '1');
#else
    *result = PR_FALSE;
#endif

#ifdef NOTIFY_LOW_MEMORY
    if (*result) {
        sGlobalMemory.FlushMemory(NS_LITERAL_STRING("low-memory").get(), PR_FALSE);
    }
#endif
    return NS_OK;
}

 nsresult
nsMemoryImpl::Create(nsISupports* outer, const nsIID& aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(outer);
    return sGlobalMemory.QueryInterface(aIID, aResult);
}

nsresult
nsMemoryImpl::FlushMemory(const PRUnichar* aReason, PRBool aImmediate)
{
    nsresult rv;

    if (aImmediate) {
        
        
        
        if (!NS_IsMainThread()) {
            NS_ERROR("can't synchronously flush memory: not on UI thread");
            return NS_ERROR_FAILURE;
        }
    }

    PRInt32 lastVal = PR_AtomicSet(&sIsFlushing, 1);
    if (lastVal)
        return NS_OK;

    PRIntervalTime now = PR_IntervalNow();

    
    
    if (aImmediate) {
        rv = RunFlushers(aReason);
    }
    else {
        
        if (PR_IntervalToMicroseconds(now - sLastFlushTime) > 1000) {
            sFlushEvent.mReason = aReason;
            rv = NS_DispatchToMainThread(&sFlushEvent, NS_DISPATCH_NORMAL);
        }
    }

    sLastFlushTime = now;
    return rv;
}

nsresult
nsMemoryImpl::RunFlushers(const PRUnichar* aReason)
{
    nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
    if (os) {

        
        
        
        

        nsCOMPtr<nsISimpleEnumerator> e;
        os->EnumerateObservers("memory-pressure", getter_AddRefs(e));

        if ( e ) {
          nsCOMPtr<nsIObserver> observer;
          PRBool loop = PR_TRUE;

          while (NS_SUCCEEDED(e->HasMoreElements(&loop)) && loop) 
          {
              e->GetNext(getter_AddRefs(observer));

              if (!observer)
                  continue;

              observer->Observe(observer, "memory-pressure", aReason);
          }
        }
    }

    
#ifdef WINCE_WINDOWS_MOBILE

    
    
    

    SHCloseApps(1024 * 1024 * 4);

#endif

    sIsFlushing = 0;
    return NS_OK;
}


NS_IMETHODIMP_(nsrefcnt) nsMemoryImpl::FlushEvent::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) nsMemoryImpl::FlushEvent::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE1(nsMemoryImpl::FlushEvent, nsIRunnable)

NS_IMETHODIMP
nsMemoryImpl::FlushEvent::Run()
{
    sGlobalMemory.RunFlushers(mReason);
    return NS_OK;
}

PRInt32
nsMemoryImpl::sIsFlushing = 0;

PRIntervalTime
nsMemoryImpl::sLastFlushTime = 0;

nsMemoryImpl::FlushEvent
nsMemoryImpl::sFlushEvent;

XPCOM_API(void*)
NS_Alloc(PRSize size)
{
    if (size > PR_INT32_MAX)
        return nsnull;

    void* result = PR_Malloc(size);
    if (! result) {
        
        sGlobalMemory.FlushMemory(NS_LITERAL_STRING("alloc-failure").get(), PR_FALSE);
    }
    return result;
}

XPCOM_API(void*)
NS_Realloc(void* ptr, PRSize size)
{
    if (size > PR_INT32_MAX)
        return nsnull;

    void* result = PR_Realloc(ptr, size);
    if (! result && size != 0) {
        
        sGlobalMemory.FlushMemory(NS_LITERAL_STRING("alloc-failure").get(), PR_FALSE);
    }
    return result;
}

XPCOM_API(void)
NS_Free(void* ptr)
{
    PR_Free(ptr);
}

nsresult
NS_GetMemoryManager(nsIMemory* *result)
{
    return sGlobalMemory.QueryInterface(NS_GET_IID(nsIMemory), (void**) result);
}
