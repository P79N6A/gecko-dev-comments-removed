





#include "SharedThreadPool.h"
#include "mozilla/Monitor.h"
#include "mozilla/StaticPtr.h"
#include "nsDataHashtable.h"
#include "VideoUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "nsComponentManagerUtils.h"
#ifdef XP_WIN
#include "ThreadPoolCOMListener.h"
#endif

namespace mozilla {


static StaticAutoPtr<ReentrantMonitor> sMonitor;



static StaticAutoPtr<nsDataHashtable<nsCStringHashKey, SharedThreadPool*>> sPools;

static already_AddRefed<nsIThreadPool>
CreateThreadPool(const nsCString& aName);

static void
DestroySharedThreadPoolHashTable();

void
SharedThreadPool::EnsureInitialized()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (sMonitor || sPools) {
    
    return;
  }
  sMonitor = new ReentrantMonitor("SharedThreadPool");
  sPools = new nsDataHashtable<nsCStringHashKey, SharedThreadPool*>();
}

class ShutdownPoolsEvent : public nsRunnable {
public:
  NS_IMETHODIMP Run() {
    MOZ_ASSERT(NS_IsMainThread());
    DestroySharedThreadPoolHashTable();
    return NS_OK;
  }
};

static void
DestroySharedThreadPoolHashTable()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sMonitor && sPools);
  if (!sPools->Count()) {
    
    
    
    
    sPools = nullptr;
    sMonitor = nullptr;
  }
}


void
SharedThreadPool::SpinUntilShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  
  while (sPools) {
    if (!NS_ProcessNextEvent(NS_GetCurrentThread(), true)) {
      break;
    }
  }
  MOZ_ASSERT(!sPools);
  MOZ_ASSERT(!sMonitor);
}

TemporaryRef<SharedThreadPool>
SharedThreadPool::Get(const nsCString& aName, uint32_t aThreadLimit)
{
  MOZ_ASSERT(NS_IsMainThread());
  EnsureInitialized();
  MOZ_ASSERT(sMonitor);
  ReentrantMonitorAutoEnter mon(*sMonitor);
  SharedThreadPool* pool = nullptr;
  nsresult rv;
  if (!sPools->Get(aName, &pool)) {
    nsCOMPtr<nsIThreadPool> threadPool(CreateThreadPool(aName));
    NS_ENSURE_TRUE(threadPool, nullptr);
    pool = new SharedThreadPool(aName, threadPool);

    
    
    
    
    
    rv = pool->SetThreadLimit(aThreadLimit);
    NS_ENSURE_SUCCESS(rv, nullptr);

    rv = pool->SetIdleThreadLimit(aThreadLimit);
    NS_ENSURE_SUCCESS(rv, nullptr);

    sPools->Put(aName, pool);
  } else if (NS_FAILED(pool->EnsureThreadLimitIsAtLeast(aThreadLimit))) {
    NS_WARNING("Failed to set limits on thread pool");
  }

  MOZ_ASSERT(pool);
  RefPtr<SharedThreadPool> instance(pool);
  return instance.forget();
}

NS_IMETHODIMP_(MozExternalRefCountType) SharedThreadPool::AddRef(void)
{
  MOZ_ASSERT(sMonitor);
  ReentrantMonitorAutoEnter mon(*sMonitor);
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");
  nsrefcnt count = ++mRefCnt;
  NS_LOG_ADDREF(this, count, "SharedThreadPool", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(MozExternalRefCountType) SharedThreadPool::Release(void)
{
  MOZ_ASSERT(sMonitor);
  bool dispatchShutdownEvent;
  {
    ReentrantMonitorAutoEnter mon(*sMonitor);
    nsrefcnt count = --mRefCnt;
    NS_LOG_RELEASE(this, count, "SharedThreadPool");
    if (count) {
      return count;
    }

    

    
    
    
    RefPtr<nsIRunnable> r = NS_NewRunnableMethod(mPool, &nsIThreadPool::Shutdown);
    NS_DispatchToMainThread(r);

    
    sPools->Remove(mName);
    MOZ_ASSERT(!sPools->Get(mName));

    
    
    mRefCnt = 1;

    delete this;

    dispatchShutdownEvent = sPools->Count() == 0;
  }
  if (dispatchShutdownEvent) {
    
    
    
    
    NS_DispatchToMainThread(new ShutdownPoolsEvent());
  }
  return 0;
}

NS_IMPL_QUERY_INTERFACE(SharedThreadPool, nsIThreadPool, nsIEventTarget)

SharedThreadPool::SharedThreadPool(const nsCString& aName,
                                   nsIThreadPool* aPool)
  : mName(aName)
  , mPool(aPool)
  , mRefCnt(0)
{
  MOZ_COUNT_CTOR(SharedThreadPool);
  mEventTarget = do_QueryInterface(aPool);
}

SharedThreadPool::~SharedThreadPool()
{
  MOZ_COUNT_DTOR(SharedThreadPool);
}

nsresult
SharedThreadPool::EnsureThreadLimitIsAtLeast(uint32_t aLimit)
{
  
  
  
  
  
  
  
  uint32_t existingLimit = 0;
  nsresult rv;

  rv = mPool->GetThreadLimit(&existingLimit);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aLimit > existingLimit) {
    rv = mPool->SetThreadLimit(aLimit);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mPool->GetIdleThreadLimit(&existingLimit);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aLimit > existingLimit) {
    rv = mPool->SetIdleThreadLimit(aLimit);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

static already_AddRefed<nsIThreadPool>
CreateThreadPool(const nsCString& aName)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;
  nsCOMPtr<nsIThreadPool> pool = do_CreateInstance(NS_THREADPOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nullptr);

  rv = pool->SetName(aName);
  NS_ENSURE_SUCCESS(rv, nullptr);

  rv = pool->SetThreadStackSize(MEDIA_THREAD_STACK_SIZE);
  NS_ENSURE_SUCCESS(rv, nullptr);

#ifdef XP_WIN
  
  nsCOMPtr<nsIThreadPoolListener> listener = new MSCOMInitThreadPoolListener();
  rv = pool->SetListener(listener);
  NS_ENSURE_SUCCESS(rv, nullptr);
#endif

  return pool.forget();
}

} 
