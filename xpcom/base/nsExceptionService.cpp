





































#include "nsISupports.h"
#include "nsExceptionService.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "prthread.h"
#include "prlock.h"
static const PRUintn BAD_TLS_INDEX = (PRUintn) -1;

#define CHECK_SERVICE_USE_OK() if (!lock) return NS_ERROR_NOT_INITIALIZED
#define CHECK_MANAGER_USE_OK() if (!mService || !nsExceptionService::lock) return NS_ERROR_NOT_INITIALIZED


class nsProviderKey : public nsHashKey {
protected:
  PRUint32 mKey;
public:
  nsProviderKey(PRUint32 key) : mKey(key) {}
  PRUint32 HashCode(void) const {
    return mKey;
  }
  PRBool Equals(const nsHashKey *aKey) const {
    return mKey == ((const nsProviderKey *) aKey)->mKey;
  }
  nsHashKey *Clone() const {
    return new nsProviderKey(mKey);
  }
  PRUint32 GetValue() { return mKey; }
};


class nsExceptionManager : public nsIExceptionManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEXCEPTIONMANAGER

  nsExceptionManager(nsExceptionService *svc);
  
  nsCOMPtr<nsIException> mCurrentException;
  nsExceptionManager *mNextThread; 
  nsExceptionService *mService; 
#ifdef NS_DEBUG
  static PRInt32 totalInstances;
#endif

private:
  ~nsExceptionManager();
};


#ifdef NS_DEBUG
PRInt32 nsExceptionManager::totalInstances = 0;
#endif





NS_IMPL_ISUPPORTS1(nsExceptionManager, nsIExceptionManager)

nsExceptionManager::nsExceptionManager(nsExceptionService *svc) :
  mNextThread(nsnull),
  mService(svc)
{
  
#ifdef NS_DEBUG
  PR_AtomicIncrement(&totalInstances);
#endif
}

nsExceptionManager::~nsExceptionManager()
{
  
#ifdef NS_DEBUG
  PR_AtomicDecrement(&totalInstances);
#endif 
}


NS_IMETHODIMP nsExceptionManager::SetCurrentException(nsIException *error)
{
    CHECK_MANAGER_USE_OK();
    mCurrentException = error;
    return NS_OK;
}


NS_IMETHODIMP nsExceptionManager::GetCurrentException(nsIException **_retval)
{
    CHECK_MANAGER_USE_OK();
    *_retval = mCurrentException;
    NS_IF_ADDREF(*_retval);
    return NS_OK;
}


NS_IMETHODIMP nsExceptionManager::GetExceptionFromProvider(nsresult rc, nsIException * defaultException, nsIException **_retval)
{
    CHECK_MANAGER_USE_OK();
    
    return mService->GetExceptionFromProvider(rc, defaultException, _retval);
}



PRUintn nsExceptionService::tlsIndex = BAD_TLS_INDEX;
PRLock *nsExceptionService::lock = nsnull;
nsExceptionManager *nsExceptionService::firstThread = nsnull;

#ifdef NS_DEBUG
PRInt32 nsExceptionService::totalInstances = 0;
#endif

NS_IMPL_THREADSAFE_ISUPPORTS2(nsExceptionService, nsIExceptionService, nsIObserver)

nsExceptionService::nsExceptionService()
  : mProviders(4, PR_TRUE) 
{
#ifdef NS_DEBUG
  if (PR_AtomicIncrement(&totalInstances)!=1) {
    NS_ERROR("The nsExceptionService is a singleton!");
  }
#endif
  
  if (tlsIndex == BAD_TLS_INDEX) {
    PRStatus status;
    status = PR_NewThreadPrivateIndex( &tlsIndex, ThreadDestruct );
    NS_ASSERTION(status==0, "ScriptErrorService could not allocate TLS storage.");
  }
  lock = PR_NewLock();
  NS_ASSERTION(lock, "Error allocating ExceptionService lock");

  
  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  NS_ASSERTION(observerService, "Could not get observer service!");
  if (observerService)
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
}

nsExceptionService::~nsExceptionService()
{
  Shutdown();
  
#ifdef NS_DEBUG
  PR_AtomicDecrement(&totalInstances);
#endif
}


void nsExceptionService::ThreadDestruct( void *data )
{
  if (!lock) {
    NS_WARNING("nsExceptionService ignoring thread destruction after shutdown");
    return;
  }
  DropThread( (nsExceptionManager *)data );
}


void nsExceptionService::Shutdown()
{
  mProviders.Reset();
  if (lock) {
    DropAllThreads();
    PR_DestroyLock(lock);
    lock = nsnull;
  }
  PR_SetThreadPrivate(tlsIndex, nsnull);
}


NS_IMETHODIMP nsExceptionService::SetCurrentException(nsIException *error)
{
    CHECK_SERVICE_USE_OK();
    nsCOMPtr<nsIExceptionManager> sm;
    nsresult nr = GetCurrentExceptionManager(getter_AddRefs(sm));
    if (NS_FAILED(nr))
        return nr;
    return sm->SetCurrentException(error);
}


NS_IMETHODIMP nsExceptionService::GetCurrentException(nsIException **_retval)
{
    CHECK_SERVICE_USE_OK();
    nsCOMPtr<nsIExceptionManager> sm;
    nsresult nr = GetCurrentExceptionManager(getter_AddRefs(sm));
    if (NS_FAILED(nr))
        return nr;
    return sm->GetCurrentException(_retval);
}


NS_IMETHODIMP nsExceptionService::GetExceptionFromProvider(nsresult rc, 
    nsIException * defaultException, nsIException **_retval)
{
    CHECK_SERVICE_USE_OK();
    return DoGetExceptionFromProvider(rc, defaultException, _retval);
}


NS_IMETHODIMP nsExceptionService::GetCurrentExceptionManager(nsIExceptionManager * *aCurrentScriptManager)
{
    CHECK_SERVICE_USE_OK();
    nsExceptionManager *mgr = (nsExceptionManager *)PR_GetThreadPrivate(tlsIndex);
    if (mgr == nsnull) {
        
        mgr = new nsExceptionManager(this);
        if (mgr == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        PR_SetThreadPrivate(tlsIndex, mgr);
        
        AddThread(mgr);
    }
    *aCurrentScriptManager = mgr;
    NS_ADDREF(*aCurrentScriptManager);
    return NS_OK;
}


NS_IMETHODIMP nsExceptionService::RegisterExceptionProvider(nsIExceptionProvider *provider, PRUint32 errorModule)
{
    CHECK_SERVICE_USE_OK();

    nsProviderKey key(errorModule);
    if (mProviders.Put(&key, provider)) {
        NS_WARNING("Registration of exception provider overwrote another provider with the same module code!");
    }
    return NS_OK;
}


NS_IMETHODIMP nsExceptionService::UnregisterExceptionProvider(nsIExceptionProvider *provider, PRUint32 errorModule)
{
    CHECK_SERVICE_USE_OK();
    nsProviderKey key(errorModule);
    if (!mProviders.Remove(&key)) {
        NS_WARNING("Attempt to unregister an unregistered exception provider!");
        return NS_ERROR_UNEXPECTED;
    }
    return NS_OK;
}


NS_IMETHODIMP nsExceptionService::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
     Shutdown();
     return NS_OK;
}

nsresult
nsExceptionService::DoGetExceptionFromProvider(nsresult errCode, 
                                               nsIException * defaultException,
                                               nsIException **_exc)
{
    
    nsresult nr = GetCurrentException(_exc);
    if (NS_SUCCEEDED(nr) && *_exc) {
        (*_exc)->GetResult(&nr);
        
        if (nr == errCode)
            return NS_OK;
        NS_RELEASE(*_exc);
    }
    nsProviderKey key(NS_ERROR_GET_MODULE(errCode));
    nsCOMPtr<nsIExceptionProvider> provider =
        dont_AddRef((nsIExceptionProvider *)mProviders.Get(&key));

    
    if (!provider) {
        *_exc = defaultException;
        NS_IF_ADDREF(*_exc);
        return NS_OK;
    }

    return provider->GetException(errCode, defaultException, _exc);
}


 void nsExceptionService::AddThread(nsExceptionManager *thread)
{
    PR_Lock(lock);
    thread->mNextThread = firstThread;
    firstThread = thread;
    NS_ADDREF(thread);
    PR_Unlock(lock);
}

 void nsExceptionService::DoDropThread(nsExceptionManager *thread)
{
    nsExceptionManager **emp = &firstThread;
    while (*emp != thread) {
        NS_ABORT_IF_FALSE(*emp, "Could not find the thread to drop!");
        emp = &(*emp)->mNextThread;
    }
    *emp = thread->mNextThread;
    NS_RELEASE(thread);
}

 void nsExceptionService::DropThread(nsExceptionManager *thread)
{
    PR_Lock(lock);
    DoDropThread(thread);
    PR_Unlock(lock);
}

 void nsExceptionService::DropAllThreads()
{
    PR_Lock(lock);
    while (firstThread)
        DoDropThread(firstThread);
    PR_Unlock(lock);
}
