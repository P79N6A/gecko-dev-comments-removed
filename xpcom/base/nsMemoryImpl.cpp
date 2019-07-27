





#include "nsMemoryImpl.h"
#include "nsThreadUtils.h"

#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsISimpleEnumerator.h"

#include "nsCOMPtr.h"
#include "mozilla/Services.h"

#ifdef ANDROID
#include <stdio.h>





#define LOW_MEMORY_THRESHOLD_KB (384 * 1024)
#endif

static nsMemoryImpl sGlobalMemory;

NS_IMPL_QUERY_INTERFACE(nsMemoryImpl, nsIMemory)

NS_IMETHODIMP
nsMemoryImpl::HeapMinimize(bool aImmediate)
{
  return FlushMemory(MOZ_UTF16("heap-minimize"), aImmediate);
}

NS_IMETHODIMP
nsMemoryImpl::IsLowMemory(bool* aResult)
{
  NS_ERROR("IsLowMemory is deprecated.  See bug 592308.");
  *aResult = false;
  return NS_OK;
}

NS_IMETHODIMP
nsMemoryImpl::IsLowMemoryPlatform(bool* aResult)
{
#ifdef ANDROID
  static int sLowMemory = -1; 
  if (sLowMemory == -1) {
    sLowMemory = 0; 
    *aResult = false;

    
    FILE* fd = fopen("/proc/meminfo", "r");
    if (!fd) {
      return NS_OK;
    }
    uint64_t mem = 0;
    int rv = fscanf(fd, "MemTotal: %llu kB", &mem);
    if (fclose(fd)) {
      return NS_OK;
    }
    if (rv != 1) {
      return NS_OK;
    }
    sLowMemory = (mem < LOW_MEMORY_THRESHOLD_KB) ? 1 : 0;
  }
  *aResult = (sLowMemory == 1);
#else
  *aResult = false;
#endif
  return NS_OK;
}

 nsresult
nsMemoryImpl::Create(nsISupports* aOuter, const nsIID& aIID, void** aResult)
{
  if (NS_WARN_IF(aOuter)) {
    return NS_ERROR_NO_AGGREGATION;
  }
  return sGlobalMemory.QueryInterface(aIID, aResult);
}

nsresult
nsMemoryImpl::FlushMemory(const char16_t* aReason, bool aImmediate)
{
  nsresult rv = NS_OK;

  if (aImmediate) {
    
    
    
    if (!NS_IsMainThread()) {
      NS_ERROR("can't synchronously flush memory: not on UI thread");
      return NS_ERROR_FAILURE;
    }
  }

  bool lastVal = sIsFlushing.exchange(true);
  if (lastVal) {
    return NS_OK;
  }

  PRIntervalTime now = PR_IntervalNow();

  
  
  if (aImmediate) {
    rv = RunFlushers(aReason);
  } else {
    
    if (PR_IntervalToMicroseconds(now - sLastFlushTime) > 1000) {
      sFlushEvent.mReason = aReason;
      rv = NS_DispatchToMainThread(&sFlushEvent);
    }
  }

  sLastFlushTime = now;
  return rv;
}

nsresult
nsMemoryImpl::RunFlushers(const char16_t* aReason)
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {

    
    
    
    

    nsCOMPtr<nsISimpleEnumerator> e;
    os->EnumerateObservers("memory-pressure", getter_AddRefs(e));

    if (e) {
      nsCOMPtr<nsIObserver> observer;
      bool loop = true;

      while (NS_SUCCEEDED(e->HasMoreElements(&loop)) && loop) {
        nsCOMPtr<nsISupports> supports;
        e->GetNext(getter_AddRefs(supports));

        if (!supports) {
          continue;
        }

        observer = do_QueryInterface(supports);
        observer->Observe(observer, "memory-pressure", aReason);
      }
    }
  }

  sIsFlushing = false;
  return NS_OK;
}


NS_IMETHODIMP_(MozExternalRefCountType)
nsMemoryImpl::FlushEvent::AddRef()
{
  return 2;
}
NS_IMETHODIMP_(MozExternalRefCountType)
nsMemoryImpl::FlushEvent::Release()
{
  return 1;
}
NS_IMPL_QUERY_INTERFACE(nsMemoryImpl::FlushEvent, nsIRunnable)

NS_IMETHODIMP
nsMemoryImpl::FlushEvent::Run()
{
  sGlobalMemory.RunFlushers(mReason);
  return NS_OK;
}

mozilla::Atomic<bool>
nsMemoryImpl::sIsFlushing;

PRIntervalTime
nsMemoryImpl::sLastFlushTime = 0;

nsMemoryImpl::FlushEvent
nsMemoryImpl::sFlushEvent;

nsresult
NS_GetMemoryManager(nsIMemory** aResult)
{
  return sGlobalMemory.QueryInterface(NS_GET_IID(nsIMemory), (void**)aResult);
}
