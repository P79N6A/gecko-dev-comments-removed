




































#include "TestHarness.h"

#include "nsIEventTarget.h"
#include "nsIProxyObjectManager.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIThreadPool.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "prlog.h"

#include "mozilla/Mutex.h"
using namespace mozilla;

typedef nsresult(*TestFuncPtr)();

#define TEST_NAME "TestProxies"

#ifdef PR_LOGGING
static PRLogModuleInfo* sLog = PR_NewLogModule(TEST_NAME);
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

static nsIThread* gMainThread = nsnull;
static nsIThread* gTestThread = nsnull;

static nsresult
GetProxyForObject(nsIEventTarget* aTarget,
                  REFNSIID aIID,
                  nsISupports* aObj,
                  PRInt32 aProxyType,
                  void** aProxyObject)
{
  nsresult rv;
  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
    do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return proxyObjMgr->GetProxyForObject(aTarget, aIID, aObj, aProxyType,
                                        aProxyObject);
}

class nsAutoTestThread
{
public:
  nsAutoTestThread(nsIThread** aGlobal = nsnull)
  : mGlobal(aGlobal)
  {
    nsCOMPtr<nsIThread> newThread;
    nsresult rv = NS_NewThread(getter_AddRefs(newThread));
    NS_ENSURE_SUCCESS(rv,);

    rv = newThread->GetPRThread(&mNativeThread);
    NS_ENSURE_SUCCESS(rv,);

    LOG(("Created test thread [0x%p]", static_cast<void*>(mNativeThread)));

    newThread.swap(mThread);

    if (mGlobal)
      *mGlobal = mThread;
  }

  ~nsAutoTestThread()
  {
    if (mGlobal)
      *mGlobal = nsnull;

#ifdef PR_LOGGING
    void* nativeThread = static_cast<void*>(mNativeThread);
#endif

    LOG(("Shutting down test thread [0x%p]", nativeThread));
    mThread->Shutdown();
    LOG(("Test thread successfully shut down [0x%p]", nativeThread));
  }

  operator nsIThread*() const
  {
    return mThread;
  }

  nsIThread* operator->() const
  {
    return mThread;
  }

private:
  nsIThread** mGlobal;
  nsCOMPtr<nsIThread> mThread;
  PRThread* mNativeThread;
};

class SimpleRunnable : public nsRunnable
{
public:
  SimpleRunnable(const char* aType = "SimpleRunnable")
  : mType(aType)
  { }

  NS_IMETHOD Run()
  {
    LOG(("%s::Run() [0x%p]", mType,
         static_cast<void*>(static_cast<nsISupports*>(this))));
    return NS_OK;
  }
private:
  const char* mType;
};

class TestTargetThreadRunnable : public SimpleRunnable
{
public:
  TestTargetThreadRunnable(nsIThread* aTarget)
  : SimpleRunnable("TestTargetThreadRunnable"),
    mTarget(aTarget)
  { }

  NS_IMETHOD Run()
  {
    nsresult rv = SimpleRunnable::Run();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIThread> currentThread(do_GetCurrentThread());
    if (currentThread != mTarget) {
      NS_ERROR("Proxy sent call to wrong thread!");
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

private:
  nsCOMPtr<nsIThread> mTarget;
};

class ChainedProxyRunnable : public SimpleRunnable
{
public:
  ChainedProxyRunnable(nsIThread* aSecondTarget,
                       nsIThread* aThirdTarget = nsnull)
  : SimpleRunnable("ChainedProxyRunnable"), mSecondTarget(aSecondTarget),
    mThirdTarget(aThirdTarget)
  { }

  NS_IMETHOD Run()
  {
    nsresult rv = SimpleRunnable::Run();
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<SimpleRunnable> runnable = mThirdTarget ?
                                        new ChainedProxyRunnable(mThirdTarget) :
                                        new SimpleRunnable();
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);
  
    nsCOMPtr<nsIRunnable> proxy;
    rv = GetProxyForObject(mSecondTarget, NS_GET_IID(nsIRunnable), runnable,
                           NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                           getter_AddRefs(proxy));
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = proxy->Run();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  nsCOMPtr<nsIThread> mSecondTarget;
  nsCOMPtr<nsIThread> mThirdTarget;
};

class IncrementingRunnable : public SimpleRunnable
{
public:
  IncrementingRunnable(PRUint32* aCounter, Mutex* aLock = nsnull)
  : SimpleRunnable("IncrementingRunnable"), mCounter(aCounter), mLock(aLock)
  { }

  NS_IMETHOD Run()
  {
    nsresult rv = SimpleRunnable::Run();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mLock)
      mLock->Lock();

    (*mCounter)++;

    if (mLock)
      mLock->Unlock();

    return NS_OK;
  }

private:
  PRUint32* mCounter;
  Mutex* mLock;
};

class NonThreadsafeRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  NonThreadsafeRunnable(PRUint32* aCounter,
                        const char* aType = "NonThreadsafeRunnable")
  : mCounter(aCounter),
    mType(aType)
  { }

  virtual ~NonThreadsafeRunnable()
  { };

  NS_IMETHOD Run()
  {
    LOG(("%s::Run() [0x%p]", mType,
         static_cast<void*>(static_cast<nsISupports*>(this))));

    (*mCounter)++;
    return NS_OK;
  }

private:
  PRUint32* mCounter;
  const char* mType;
};

NS_IMPL_ISUPPORTS1(NonThreadsafeRunnable, nsIRunnable)

class MainThreadRunnable : public NonThreadsafeRunnable
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  MainThreadRunnable(PRUint32* aCounter)
  : NonThreadsafeRunnable(aCounter, "MainThreadRunnable")
  {
    if (!NS_IsMainThread()) {
      NS_ERROR("Not running on the main thread!");
    }
  }

  virtual ~MainThreadRunnable()
  {
    if (!NS_IsMainThread()) {
      NS_ERROR("Not running on the main thread!");
    }
  }

  NS_IMETHOD Run()
  {
    if (!NS_IsMainThread()) {
      NS_ERROR("Not running on the main thread!");
      return NS_ERROR_FAILURE;
    }

    nsresult rv = NonThreadsafeRunnable::Run();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS_INHERITED0(MainThreadRunnable, NonThreadsafeRunnable)

class ProxyGetter : public nsRunnable
{
public:
  ProxyGetter(nsIRunnable* aRunnable, nsIRunnable** retval)
  : mRunnable(aRunnable), _retval(retval)
  { }

  NS_IMETHOD Run()
  {
    *_retval = nsnull;

    if (NS_IsMainThread()) {
      NS_ERROR("Shouldn't be running on the main thread!");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIRunnable> proxy;
    nsresult rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable),
                                    mRunnable, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                    getter_AddRefs(proxy));
    NS_ENSURE_SUCCESS(rv, rv);

    proxy.forget(_retval);
    return NS_OK;
  }

private:
  nsIRunnable* mRunnable;
  nsIRunnable** _retval;
};

class RunnableGetter : public nsRunnable
{
public:
  RunnableGetter(PRUint32* aCounter, nsIRunnable** retval)
  : mCounter(aCounter), _retval(retval)
  { }

  NS_IMETHOD Run()
  {
    *_retval = nsnull;

    if (NS_IsMainThread()) {
      NS_ERROR("Shouldn't be running on the main thread!");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIRunnable> runnable = new NonThreadsafeRunnable(mCounter);
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

    runnable.forget(_retval);
    return NS_OK;
  }

private:
  PRUint32* mCounter;
  nsIRunnable** _retval;
};

nsresult
TestTargetThread()
{
  LOG(("--- Running TestTargetThread ---"));

  nsRefPtr<TestTargetThreadRunnable> runnable =
    new TestTargetThreadRunnable(gMainThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIRunnable> proxy;
  nsresult rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable),
                                  runnable, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  runnable = new TestTargetThreadRunnable(gTestThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS, getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
TestNonThreadsafeProxy()
{
  LOG(("--- Running TestNonThreadsafeProxy 1 ---"));

  
  

  PRUint32 counter = 0;
  nsCOMPtr<nsIRunnable> runnable(new MainThreadRunnable(&counter));
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIRunnable> proxy;
  nsresult rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable),
                                  runnable, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 otherCounter = 0; otherCounter < 5;) {
    rv = gTestThread->Dispatch(proxy, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);
  }

  
  

  LOG(("--- Running TestNonThreadsafeProxy 2 ---"));

  counter = 0;
  runnable = new NonThreadsafeRunnable(&counter);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable),
                         runnable, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  runnable = nsnull;

  for (PRUint32 otherCounter = 0; otherCounter < 5;) {
    rv = proxy->Run();
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);
  }

  NS_ENSURE_TRUE(counter == 5, NS_ERROR_FAILURE);

  
  

  LOG(("--- Running TestNonThreadsafeProxy 3 ---"));

  counter = 0;
  proxy = nsnull;

  runnable = new MainThreadRunnable(&counter);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIRunnable> proxyGetter =
    new ProxyGetter(runnable, getter_AddRefs(proxy));
  NS_ENSURE_TRUE(proxyGetter, NS_ERROR_OUT_OF_MEMORY);

  rv = gTestThread->Dispatch(proxyGetter, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(proxy, NS_ERROR_FAILURE);

  for (PRUint32 otherCounter = 0; otherCounter < 5;) {
    rv = proxy->Run();
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);
  }

  
  

  LOG(("--- Running TestNonThreadsafeProxy 4 ---"));

  counter = 0;
  proxy = nsnull;
  runnable = nsnull;

  nsCOMPtr<nsIRunnable> runnableGetter =
    new RunnableGetter(&counter, getter_AddRefs(runnable));
  NS_ENSURE_TRUE(runnableGetter, NS_ERROR_OUT_OF_MEMORY);

  rv = gTestThread->Dispatch(runnableGetter, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

  proxyGetter = new ProxyGetter(runnable, getter_AddRefs(proxy));
  NS_ENSURE_TRUE(proxyGetter, NS_ERROR_OUT_OF_MEMORY);

  nsAutoTestThread otherTestThread;
  NS_ENSURE_TRUE(otherTestThread, NS_ERROR_FAILURE);

  rv = otherTestThread->Dispatch(proxyGetter, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(proxy, NS_ERROR_FAILURE);

  for (PRUint32 otherCounter = 0; otherCounter < 5;) {
    rv = proxy->Run();
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);
  }

  return NS_OK;
}

nsresult
TestChainedProxy()
{
  LOG(("--- Running TestChainedProxy ---"));

  nsRefPtr<ChainedProxyRunnable> runnable =
    new ChainedProxyRunnable(gMainThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIRunnable> proxy;
  nsresult rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable),
                                  runnable, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  runnable = new ChainedProxyRunnable(gTestThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  runnable = new ChainedProxyRunnable(gMainThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  runnable = new ChainedProxyRunnable(gTestThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  runnable = new ChainedProxyRunnable(gMainThread, gTestThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
TestReleaseOfRealObjects()
{
  LOG(("--- Running TestReleaseOfRealObjects ---"));

  PRUint32 counter = 0, otherCounter = 0;

  nsRefPtr<IncrementingRunnable> runnable(new IncrementingRunnable(&counter));
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIRunnable> proxy1;
  nsresult rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable),
                                  runnable, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(proxy1));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> proxy2;
  rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy2));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> proxy3;
  rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy3));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_FALSE(proxy1 == proxy2, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(proxy2 == proxy3, NS_ERROR_FAILURE);
  proxy3 = nsnull;

  rv = proxy1->Run();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);

  rv = proxy2->Run();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);

  runnable = nsnull;

  rv = proxy1->Run();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);

  rv = proxy2->Run();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);

  proxy1 = nsnull;

  rv = proxy2->Run();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
TestCurrentThreadProxy()
{
  LOG(("--- Running TestCurrentThreadProxy ---"));

  PRUint32 counter = 0, otherCounter = 0;
  nsRefPtr<IncrementingRunnable> runnable(new IncrementingRunnable(&counter));
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIRunnable> proxy1;
  nsresult rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable),
                                  runnable, NS_PROXY_SYNC,
                                  getter_AddRefs(proxy1));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> proxy2;
  rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(proxy2));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_FALSE(proxy1 == proxy2, NS_ERROR_FAILURE);

  nsCOMPtr<nsIRunnable> realRunnable(do_QueryInterface(runnable));
  NS_ENSURE_TRUE(realRunnable, NS_ERROR_FAILURE);

  NS_ENSURE_TRUE(static_cast<void*>(realRunnable) == static_cast<void*>(runnable),
                 NS_ERROR_FAILURE);

  rv = proxy1->Run();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);

  rv = proxy2->Run();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(counter == ++otherCounter, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
TestAsyncProxy()
{
  LOG(("--- Running TestAsyncProxy ---"));

  

  PRUint32 counter = 0;
  nsRefPtr<SimpleRunnable> runnable(new IncrementingRunnable(&counter));
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIRunnable> proxy;
  nsresult rv = GetProxyForObject(gMainThread, NS_GET_IID(nsIRunnable),
                                  runnable, NS_PROXY_ASYNC,
                                  getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  runnable = nsnull;

  for (PRUint32 i = 0; i < 5; i++) {
    rv = proxy->Run();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  while (counter < 5) {
    rv = NS_ProcessPendingEvents(gMainThread, PR_SecondsToInterval(1));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  

  Mutex* counterLock = new Mutex("counterLock");
  NS_ENSURE_TRUE(counterLock, NS_ERROR_OUT_OF_MEMORY);

  counter = 0;
  runnable = new IncrementingRunnable(&counter, counterLock);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable), runnable,
                         NS_PROXY_ASYNC, getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < 5; i++) {
    rv = proxy->Run();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 safeCounter = 0;
  while (safeCounter < 5) {
    rv = NS_ProcessPendingEvents(gMainThread, PR_SecondsToInterval(1));
    NS_ENSURE_SUCCESS(rv, rv);

    MutexAutoLock lock(*counterLock);
    safeCounter = counter;
  }

  delete counterLock;

  
  

  runnable = new ChainedProxyRunnable(gMainThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProxyForObject(gTestThread, NS_GET_IID(nsIRunnable),
                         runnable, NS_PROXY_ASYNC,
                         getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxy->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<nsIRunnable> flusher = new nsRunnable();
  NS_ENSURE_TRUE(flusher, NS_ERROR_OUT_OF_MEMORY);

  LOG(("Flushing events on test thread"));

  rv = gTestThread->Dispatch(flusher, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("Flushing events completed"));

  return NS_OK;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom(TEST_NAME);
  NS_ENSURE_FALSE(xpcom.failed(), 1);

  nsCOMPtr<nsIThread> mainThread(do_GetMainThread());
  NS_ENSURE_TRUE(mainThread, 1);

  LOG(("Got main thread"));
  gMainThread = mainThread;

  nsAutoTestThread testThread(&gTestThread);
  NS_ENSURE_TRUE(testThread, 1);

  static TestFuncPtr testsToRun[] = {
    TestTargetThread,
    
    TestChainedProxy,
    TestReleaseOfRealObjects,
    TestCurrentThreadProxy,
    TestAsyncProxy
  };
  static PRUint32 testCount = sizeof(testsToRun) / sizeof(testsToRun[0]);

  for (PRUint32 i = 0; i < testCount; i++) {
    nsresult rv = testsToRun[i]();
    NS_ENSURE_SUCCESS(rv, 1);
  }

  LOG(("--- Finished all tests ---"));
  return 0;
}
