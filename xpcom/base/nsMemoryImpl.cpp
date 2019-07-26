




#include "nsXPCOM.h"
#include "nsMemoryImpl.h"
#include "nsThreadUtils.h"

#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"

#include "prcvar.h"
#include "pratom.h"

#include "nsAlgorithm.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Services.h"

static nsMemoryImpl sGlobalMemory;

NS_IMPL_QUERY_INTERFACE1(nsMemoryImpl, nsIMemory)

NS_IMETHODIMP_(void*)
nsMemoryImpl::Alloc(size_t size)
{
    return NS_Alloc(size);
}

NS_IMETHODIMP_(void*)
nsMemoryImpl::Realloc(void* ptr, size_t size)
{
    return NS_Realloc(ptr, size);
}

NS_IMETHODIMP_(void)
nsMemoryImpl::Free(void* ptr)
{
    NS_Free(ptr);
}

NS_IMETHODIMP
nsMemoryImpl::HeapMinimize(bool aImmediate)
{
    return FlushMemory(NS_LITERAL_STRING("heap-minimize").get(), aImmediate);
}

NS_IMETHODIMP
nsMemoryImpl::IsLowMemory(bool *result)
{
    NS_ERROR("IsLowMemory is deprecated.  See bug 592308.");
    *result = false;
    return NS_OK;
}

 nsresult
nsMemoryImpl::Create(nsISupports* outer, const nsIID& aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(outer);
    return sGlobalMemory.QueryInterface(aIID, aResult);
}

nsresult
nsMemoryImpl::FlushMemory(const PRUnichar* aReason, bool aImmediate)
{
    nsresult rv = NS_OK;

    if (aImmediate) {
        
        
        
        if (!NS_IsMainThread()) {
            NS_ERROR("can't synchronously flush memory: not on UI thread");
            return NS_ERROR_FAILURE;
        }
    }

    int32_t lastVal = PR_ATOMIC_SET(&sIsFlushing, 1);
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
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {

        
        
        
        

        nsCOMPtr<nsISimpleEnumerator> e;
        os->EnumerateObservers("memory-pressure", getter_AddRefs(e));

        if ( e ) {
          nsCOMPtr<nsIObserver> observer;
          bool loop = true;

          while (NS_SUCCEEDED(e->HasMoreElements(&loop)) && loop) 
          {
              e->GetNext(getter_AddRefs(observer));

              if (!observer)
                  continue;

              observer->Observe(observer, "memory-pressure", aReason);
          }
        }
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

int32_t
nsMemoryImpl::sIsFlushing = 0;

PRIntervalTime
nsMemoryImpl::sLastFlushTime = 0;

nsMemoryImpl::FlushEvent
nsMemoryImpl::sFlushEvent;

XPCOM_API(void*)
NS_Alloc(size_t size)
{
    return moz_xmalloc(size);
}

XPCOM_API(void*)
NS_Realloc(void* ptr, size_t size)
{
    return moz_xrealloc(ptr, size);
}

XPCOM_API(void)
NS_Free(void* ptr)
{
    moz_free(ptr);
}

nsresult
NS_GetMemoryManager(nsIMemory* *result)
{
    return sGlobalMemory.QueryInterface(NS_GET_IID(nsIMemory), (void**) result);
}
