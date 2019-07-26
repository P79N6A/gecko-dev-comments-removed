





#include "SharedThreadPool.h"
#include "mozilla/Monitor.h"
#include "mozilla/StaticPtr.h"
#include "nsDataHashtable.h"
#include "VideoUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/Preferences.h"

#ifdef XP_WIN

#include <Objbase.h>
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

TemporaryRef<SharedThreadPool>
SharedThreadPool::Get(const nsCString& aName)
{
  MOZ_ASSERT(NS_IsMainThread());
  EnsureInitialized();
  MOZ_ASSERT(sMonitor);
  ReentrantMonitorAutoEnter mon(*sMonitor);
  SharedThreadPool* pool = nullptr;
  if (!sPools->Get(aName, &pool)) {
    nsCOMPtr<nsIThreadPool> threadPool(CreateThreadPool(aName));
    NS_ENSURE_TRUE(threadPool, nullptr);
    pool = new SharedThreadPool(aName, threadPool);
    sPools->Put(aName, pool);
  }
  MOZ_ASSERT(pool);
  RefPtr<SharedThreadPool> instance(pool);
  return instance.forget();
}

NS_IMETHODIMP_(nsrefcnt) SharedThreadPool::AddRef(void)
{
  MOZ_ASSERT(sMonitor);
  ReentrantMonitorAutoEnter mon(*sMonitor);
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");
  nsrefcnt count = ++mRefCnt;
  NS_LOG_ADDREF(this, count, "SharedThreadPool", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(nsrefcnt) SharedThreadPool::Release(void)
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
    
    
    
    
    NS_DispatchToMainThread(new ShutdownPoolsEvent(), NS_DISPATCH_NORMAL);
  }
  return 0;
}

NS_IMPL_QUERY_INTERFACE1(SharedThreadPool, nsIThreadPool)

SharedThreadPool::SharedThreadPool(const nsCString& aName,
                                   nsIThreadPool* aPool)
  : mName(aName)
  , mPool(aPool)
  , mRefCnt(0)
{
  mEventTarget = do_QueryInterface(aPool);
}

SharedThreadPool::~SharedThreadPool()
{
}

#ifdef XP_WIN




class MSCOMInitThreadPoolListener MOZ_FINAL : public nsIThreadPoolListener {
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITHREADPOOLLISTENER
};

NS_IMPL_ISUPPORTS1(MSCOMInitThreadPoolListener, nsIThreadPoolListener)

NS_IMETHODIMP
MSCOMInitThreadPoolListener::OnThreadCreated()
{
  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    NS_WARNING("Failed to initialize MSCOM on WMFByteStream thread.");
  }
  return NS_OK;
}

NS_IMETHODIMP
MSCOMInitThreadPoolListener::OnThreadShuttingDown()
{
  CoUninitialize();
  return NS_OK;
}

#endif

static already_AddRefed<nsIThreadPool>
CreateThreadPool(const nsCString& aName)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;
  nsCOMPtr<nsIThreadPool> pool = do_CreateInstance(NS_THREADPOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nullptr);

  rv = pool->SetName(aName);
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  
  
  
  
  
  
  rv = pool->SetThreadLimit(
    Preferences::GetUint("media.thread-pool.thread-limit", 4));
  NS_ENSURE_SUCCESS(rv, nullptr);

  rv = pool->SetIdleThreadLimit(
    Preferences::GetUint("media.thread-pool.idle-thread-limit", 4));
  NS_ENSURE_SUCCESS(rv, nullptr);

#ifdef XP_WIN
  
  nsCOMPtr<nsIThreadPoolListener> listener = new MSCOMInitThreadPoolListener();
  rv = pool->SetListener(listener);
  NS_ENSURE_SUCCESS(rv, nullptr);
#endif

  return pool.forget();
}

} 
