




































#include "TestHarness.h"

#include "nsIFactory.h"
#include "nsIThread.h"
#include "nsIComponentRegistrar.h"

#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "prmon.h"

#include "mozilla/Monitor.h"
using namespace mozilla;

#ifdef DEBUG
#define TEST_ASSERTION(_test, _msg) \
    NS_ASSERTION(_test, _msg);
#else
#define TEST_ASSERTION(_test, _msg) \
  PR_BEGIN_MACRO \
    if (!(_test)) { \
      NS_DebugBreak(NS_DEBUG_ABORT, _msg, #_test, __FILE__, __LINE__); \
    } \
  PR_END_MACRO
#endif

 
#define FACTORY_CID1                                 \
{                                                    \
  0xf93f6bdc,                                        \
  0x88af,                                            \
  0x42d7,                                            \
  { 0x9d, 0x64, 0x1b, 0x43, 0xc6, 0x49, 0xa3, 0xe5 } \
}
NS_DEFINE_CID(kFactoryCID1, FACTORY_CID1);


#define FACTORY_CID2                                 \
{                                                    \
  0xef38ad65,                                        \
  0x6595,                                            \
  0x49f0,                                            \
  { 0x80, 0x48, 0xe8, 0x19, 0xf8, 0x1d, 0x15, 0xe2 } \
}
NS_DEFINE_CID(kFactoryCID2, FACTORY_CID2);

#define FACTORY_CONTRACTID                           \
  "TestRacingThreadManager/factory;1"

PRInt32 gComponent1Count = 0;
PRInt32 gComponent2Count = 0;

Monitor* gMonitor = nsnull;

PRBool gCreateInstanceCalled = PR_FALSE;
PRBool gMainThreadWaiting = PR_FALSE;

class AutoCreateAndDestroyMonitor
{
public:
  AutoCreateAndDestroyMonitor(Monitor** aMonitorPtr)
  : mMonitorPtr(aMonitorPtr) {
    *aMonitorPtr =
      new Monitor("TestRacingServiceManager::AutoMon");
    TEST_ASSERTION(*aMonitorPtr, "Out of memory!");
  }

  ~AutoCreateAndDestroyMonitor() {
    if (*mMonitorPtr) {
      delete *mMonitorPtr;
      *mMonitorPtr = nsnull;
    }
  }

private:
  Monitor** mMonitorPtr;
};

class Factory : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS

  Factory() : mFirstComponentCreated(PR_FALSE) { }

  NS_IMETHOD CreateInstance(nsISupports* aDelegate,
                            const nsIID& aIID,
                            void** aResult);

  NS_IMETHOD LockFactory(PRBool aLock) {
    return NS_OK;
  }

  PRBool mFirstComponentCreated;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(Factory, nsIFactory)

class Component1 : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  Component1() {
    
    PRInt32 count = PR_AtomicIncrement(&gComponent1Count);
    TEST_ASSERTION(count == 1, "Too many components created!");
  }
};

NS_IMPL_THREADSAFE_ADDREF(Component1)
NS_IMPL_THREADSAFE_RELEASE(Component1)

NS_INTERFACE_MAP_BEGIN(Component1)
  NS_INTERFACE_MAP_ENTRY(Component1)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

class Component2 : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  Component2() {
    
    PRInt32 count = PR_AtomicIncrement(&gComponent2Count);
    TEST_ASSERTION(count == 1, "Too many components created!");
  }
};

NS_IMPL_THREADSAFE_ADDREF(Component2)
NS_IMPL_THREADSAFE_RELEASE(Component2)

NS_INTERFACE_MAP_BEGIN(Component2)
  NS_INTERFACE_MAP_ENTRY(Component2)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
Factory::CreateInstance(nsISupports* aDelegate,
                        const nsIID& aIID,
                        void** aResult)
{
  
  
  TEST_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  {
    MonitorAutoEnter mon(*gMonitor);

    gCreateInstanceCalled = PR_TRUE;
    mon.Notify();

    mon.Wait(PR_MillisecondsToInterval(3000));
  }

  NS_ENSURE_FALSE(aDelegate, NS_ERROR_NO_AGGREGATION);
  NS_ENSURE_ARG_POINTER(aResult);

  nsCOMPtr<nsISupports> instance;

  if (!mFirstComponentCreated) {
    instance = new Component1();
  }
  else {
    instance = new Component2();
  }
  NS_ENSURE_TRUE(instance, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = instance->QueryInterface(aIID, aResult);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

class Runnable : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  Runnable() : mFirstRunnableDone(PR_FALSE) { }

  PRBool mFirstRunnableDone;
};

NS_IMETHODIMP
Runnable::Run()
{
  {
    MonitorAutoEnter mon(*gMonitor);

    while (!gMainThreadWaiting) {
      mon.Wait();
    }
  }

  nsresult rv;
  nsCOMPtr<nsISupports> component;

  if (!mFirstRunnableDone) {
    component = do_GetService(kFactoryCID1, &rv);
  }
  else {
    component = do_GetService(FACTORY_CONTRACTID, &rv);
  }
  TEST_ASSERTION(NS_SUCCEEDED(rv), "GetService failed!");

  return NS_OK;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("RacingServiceManager");
  NS_ENSURE_FALSE(xpcom.failed(), 1);

  nsCOMPtr<nsIComponentRegistrar> registrar;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, 1);

  nsRefPtr<Factory> factory = new Factory();
  NS_ENSURE_TRUE(factory, 1);

  rv = registrar->RegisterFactory(kFactoryCID1, nsnull, nsnull, factory);
  NS_ENSURE_SUCCESS(rv, 1);

  rv = registrar->RegisterFactory(kFactoryCID2, nsnull, FACTORY_CONTRACTID,
                                  factory);
  NS_ENSURE_SUCCESS(rv, 1);

  AutoCreateAndDestroyMonitor mon(&gMonitor);

  nsRefPtr<Runnable> runnable = new Runnable();
  NS_ENSURE_TRUE(runnable, 1);

  
  nsCOMPtr<nsIThread> newThread;
  rv = NS_NewThread(getter_AddRefs(newThread), runnable);
  NS_ENSURE_SUCCESS(rv, 1);

  {
    MonitorAutoEnter mon(*gMonitor);

    gMainThreadWaiting = PR_TRUE;
    mon.Notify();

    while (!gCreateInstanceCalled) {
      mon.Wait();
    }
  }

  nsCOMPtr<nsISupports> component(do_GetService(kFactoryCID1, &rv));
  NS_ENSURE_SUCCESS(rv, 1);

  
  gMainThreadWaiting = gCreateInstanceCalled = PR_FALSE;
  factory->mFirstComponentCreated = runnable->mFirstRunnableDone = PR_TRUE;
  component = nsnull;

  rv = newThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, 1);

  {
    MonitorAutoEnter mon(*gMonitor);

    gMainThreadWaiting = PR_TRUE;
    mon.Notify();

    while (!gCreateInstanceCalled) {
      mon.Wait();
    }
  }

  component = do_GetService(FACTORY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, 1);

  return 0;
}
