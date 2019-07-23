




































#include "nsXPCOM.h"
#include "nsMemoryImpl.h"
#include "nsThreadUtils.h"

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
#define NS_MEMORY_FLUSHER
#elif defined (NS_OSSO)
#include <osso-mem.h>
#include <fcntl.h>
#include <unistd.h>
const char* kHighMark = "/sys/kernel/high_watermark";
#else

#undef NS_MEMORY_FLUSHER
#endif

#ifdef NS_MEMORY_FLUSHER
#include "nsITimer.h"
#endif




#ifdef DEBUG_xwarren
#define NS_OUT_OF_MEMORY_TESTER
#endif

#ifdef NS_OUT_OF_MEMORY_TESTER


#define NS_FLUSH_FREQUENCY        100000


#define NS_FAIL_FREQUENCY         10

PRUint32 gFlushFreq = 0;
PRUint32 gFailFreq = 0;

static void*
mallocator(PRSize size, PRUint32& counter, PRUint32 max)
{
    if (counter++ >= max) {
        counter = 0;
        NS_ERROR("about to fail allocation... watch out");
        return nsnull;
    }
    return PR_Malloc(size);
}

static void*
reallocator(void* ptr, PRSize size, PRUint32& counter, PRUint32 max)
{
    if (counter++ >= max) {
        counter = 0;
        NS_ERROR("about to fail reallocation... watch out");
        return nsnull;
    }
    return PR_Realloc(ptr, size);
}

#define MALLOC1(s)       mallocator(s, gFlushFreq, NS_FLUSH_FREQUENCY)
#define REALLOC1(p, s)   reallocator(p, s, gFlushFreq, NS_FLUSH_FREQUENCY)

#else

#define MALLOC1(s)       PR_Malloc(s)
#define REALLOC1(p, s)   PR_Realloc(p, s)

#endif 

#if defined(XDEBUG_waterson)
#define NS_TEST_MEMORY_FLUSHER
#endif

#ifdef NS_MEMORY_FLUSHER




class MemoryFlusher : public nsITimerCallback
{
public:
    
    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aResult);
    NS_IMETHOD_(nsrefcnt) AddRef(void) { return 2; }
    NS_IMETHOD_(nsrefcnt) Release(void) { return 1; }

    NS_DECL_NSITIMERCALLBACK

    nsresult Init();
    void StopAndJoin();

private:
    static PRIntervalTime sTimeout;
    static PRLock*        sLock;
    static PRCondVar*     sCVar;
    
    enum {
        kTimeout = 60000 
    };
};

static MemoryFlusher sGlobalMemoryFlusher;

#endif 

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
#if defined(WINCE)
    *result = PR_FALSE;
    
    
    
    
    
#if 0
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    *result = ((float)stat.dwAvailPhys / stat.dwTotalPhys) < 0.1;
#endif
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
    return NS_OK;
}


 nsresult 
nsMemoryImpl::InitFlusher()
{
#ifdef NS_MEMORY_FLUSHER
    return sGlobalMemoryFlusher.Init();
#else
    return NS_OK;
#endif
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

    
    
    if (aImmediate) {
        rv = RunFlushers(aReason);
    }
    else {
        sFlushEvent.mReason = aReason;
        rv = NS_DispatchToMainThread(&sFlushEvent, NS_DISPATCH_NORMAL);
    }

    return rv;
}

nsresult
nsMemoryImpl::RunFlushers(const PRUnichar* aReason)
{
    nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
    if (os) {
        os->NotifyObservers(this, "memory-pressure", aReason);
    }

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

nsMemoryImpl::FlushEvent
nsMemoryImpl::sFlushEvent;

XPCOM_API(void*)
NS_Alloc(PRSize size)
{
    if (size > PR_INT32_MAX)
        return nsnull;

    void* result = MALLOC1(size);
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

    void* result = REALLOC1(ptr, size);
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

#ifdef NS_MEMORY_FLUSHER

NS_IMPL_QUERY_INTERFACE1(MemoryFlusher, nsITimerCallback)

NS_IMETHODIMP
MemoryFlusher::Notify(nsITimer *timer)
{
    PRBool isLowMemory;
    sGlobalMemory.IsLowMemory(&isLowMemory);

#ifdef NS_TEST_MEMORY_FLUSHER
    
    isLowMemory = PR_TRUE;
#endif

    if (isLowMemory)
        sGlobalMemory.FlushMemory(NS_LITERAL_STRING("low-memory").get(),
                                  PR_FALSE);
    return NS_OK;
}

nsresult
MemoryFlusher::Init()
{
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    NS_ENSURE_STATE(timer);

    
    
    

    return timer->InitWithCallback(this, kTimeout,
                                   nsITimer::TYPE_REPEATING_SLACK);
}

#endif 

nsresult
NS_GetMemoryManager(nsIMemory* *result)
{
    return sGlobalMemory.QueryInterface(NS_GET_IID(nsIMemory), (void**) result);
}
