




































#include <stdio.h>

#include "nsXPCOM.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIServiceManager.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

#include "nscore.h"
#include "nspr.h"

#include "nsITestProxy.h"
#include "nsISupportsPrimitives.h"

#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Mutex.h"
#include "nsIRunnable.h"
#include "nsIProxyObjectManager.h"
#include "nsXPCOMCIDInternal.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsISupportsUtils.h"

using namespace mozilla;































class ProxyTest : public nsIRunnable,
                  public nsITestProxy,
                  public nsISupportsPrimitive
{
public:
    ProxyTest()
        : mCounterLock("ProxyTest.mCounterLock")
        , mEvilReentrantMonitor("ProxyTest.mEvilReentrantMonitor")
        , mCounter(0)
    {}

    NS_IMETHOD Run()
    {
        nsresult rv;
        nsCOMPtr<nsIProxyObjectManager> pom =
            do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        
        nsCOMPtr<nsISupportsPrimitive> prim;
        rv = pom->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                    NS_GET_IID(nsISupportsPrimitive),
                                    NS_ISUPPORTS_CAST(nsIRunnable*, this),
                                    NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                    getter_AddRefs(prim));
        NS_ENSURE_SUCCESS(rv, rv);
        
        return NS_OK;
    }

    NS_IMETHOD Test(PRInt32 p1, PRInt32 p2, PRInt32 *_retval)
    {
        nsresult rv;

        if (!NS_IsMainThread())
            return NS_ERROR_UNEXPECTED;

        

        rv = NS_NewThread(getter_AddRefs(mThreadOne),
                          static_cast<nsIRunnable*>(this));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = NS_NewThread(getter_AddRefs(mThreadTwo),
                          static_cast<nsIRunnable*>(this));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mThreadOne->Shutdown();
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mThreadTwo->Shutdown();
        NS_ENSURE_SUCCESS(rv, rv);

        return NS_OK;
    }

    NS_IMETHOD Test2(void)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD Test3(nsISupports *p1, nsISupports **p2)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD GetType(PRUint16 *_retval)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr)
    {
        NS_ASSERTION(aInstancePtr,
                     "QueryInterface requires a non-NULL destination!");
        nsISupports* foundInterface;
        if ( aIID.Equals(NS_GET_IID(nsIRunnable)) ) {
            foundInterface = static_cast<nsIRunnable*>(this);
        } else if ( aIID.Equals(NS_GET_IID(nsITestProxy)) ) {
            foundInterface = static_cast<nsITestProxy*>(this);
        } else if ( aIID.Equals(NS_GET_IID(nsISupports)) ) {
            foundInterface = NS_ISUPPORTS_CAST(nsIRunnable*, this);
        } else if ( aIID.Equals(NS_GET_IID(nsISupportsPrimitive)) ) {
            {
                MutexAutoLock counterLock(mCounterLock);
                switch(mCounter) {
                    case 0:
                        ++mCounter;
                    {
                        
                        MutexAutoUnlock counterUnlock(mCounterLock);
                        ReentrantMonitorAutoEnter evilReentrantMonitor(mEvilReentrantMonitor);
                        nsresult rv = evilReentrantMonitor.Wait();
                        NS_ENSURE_SUCCESS(rv, rv);
                        break;
                    }
                    case 1:
                        ++mCounter;
                    {
                        
                        MutexAutoUnlock counterUnlock(mCounterLock);
                        ReentrantMonitorAutoEnter evilReentrantMonitor(mEvilReentrantMonitor);
                        nsresult rv = evilReentrantMonitor.Notify();
                        NS_ENSURE_SUCCESS(rv, rv);
                        break;
                    }
                    default: {
                        
                        ++mCounter;
                    }
                }
                ++mCounter;
            }
            
            foundInterface = static_cast<nsISupportsPrimitive*>(this);
        } else {
            foundInterface = nsnull;
        }
        nsresult status;
        if (!foundInterface) {
            status = NS_ERROR_NO_INTERFACE;
        } else {
            NS_ADDREF(foundInterface);
            status = NS_OK;
        }
        *aInstancePtr = foundInterface;
        return status;
    }

    NS_IMETHOD_(nsrefcnt) AddRef(void);
    NS_IMETHOD_(nsrefcnt) Release(void);

protected:
    nsAutoRefCnt mRefCnt;
    NS_DECL_OWNINGTHREAD

private:
    Mutex mCounterLock;
    ReentrantMonitor mEvilReentrantMonitor;
    PRInt32 mCounter;
    nsCOMPtr<nsIThread> mThreadOne;
    nsCOMPtr<nsIThread> mThreadTwo;
};

NS_IMPL_THREADSAFE_ADDREF(ProxyTest)
NS_IMPL_THREADSAFE_RELEASE(ProxyTest)

int
main(int argc, char **argv)
{
    NS_InitXPCOM2(nsnull, nsnull, nsnull);

    
    {
        nsCOMPtr<nsITestProxy> tester = new ProxyTest();
        tester->Test(0, 0, nsnull);
    }

    NS_ShutdownXPCOM(nsnull);

    return 0;
}

