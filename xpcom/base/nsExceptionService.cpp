




#include "mozilla/Attributes.h"

#include "nsISupports.h"
#include "nsExceptionService.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "prthread.h"
#include "mozilla/Services.h"

using namespace mozilla;

static const unsigned BAD_TLS_INDEX = (unsigned) -1;

#define CHECK_SERVICE_USE_OK() if (!sLock) return NS_ERROR_NOT_INITIALIZED
#define CHECK_MANAGER_USE_OK() if (!mService || !nsExceptionService::sLock) return NS_ERROR_NOT_INITIALIZED


class nsProviderKey : public nsHashKey {
protected:
  uint32_t mKey;
public:
  nsProviderKey(uint32_t key) : mKey(key) {}
  uint32_t HashCode(void) const {
    return mKey;
  }
  bool Equals(const nsHashKey *aKey) const {
    return mKey == ((const nsProviderKey *) aKey)->mKey;
  }
  nsHashKey *Clone() const {
    return new nsProviderKey(mKey);
  }
  uint32_t GetValue() { return mKey; }
};


class nsExceptionManager MOZ_FINAL : public nsIExceptionManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEXCEPTIONMANAGER

  nsExceptionManager(nsExceptionService *svc);
  
  nsCOMPtr<nsIException> mCurrentException;
  nsExceptionManager *mNextThread; 
  nsExceptionService *mService; 
#ifdef DEBUG
  static int32_t totalInstances;
#endif

private:
  ~nsExceptionManager();
};


#ifdef DEBUG
int32_t nsExceptionManager::totalInstances = 0;
#endif





NS_IMPL_THREADSAFE_ISUPPORTS1(nsExceptionManager, nsIExceptionManager)

nsExceptionManager::nsExceptionManager(nsExceptionService *svc) :
  mNextThread(nullptr),
  mService(svc)
{
  
#ifdef DEBUG
  PR_ATOMIC_INCREMENT(&totalInstances);
#endif
}

nsExceptionManager::~nsExceptionManager()
{
  
#ifdef DEBUG
  PR_ATOMIC_DECREMENT(&totalInstances);
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



unsigned nsExceptionService::tlsIndex = BAD_TLS_INDEX;
Mutex *nsExceptionService::sLock = nullptr;
nsExceptionManager *nsExceptionService::firstThread = nullptr;

#ifdef DEBUG
int32_t nsExceptionService::totalInstances = 0;
#endif

NS_IMPL_THREADSAFE_ISUPPORTS3(nsExceptionService,
                              nsIExceptionService,
                              nsIExceptionManager,
                              nsIObserver)

nsExceptionService::nsExceptionService()
  : mProviders(4, true) 
{
#ifdef DEBUG
  if (PR_ATOMIC_INCREMENT(&totalInstances)!=1) {
    NS_ERROR("The nsExceptionService is a singleton!");
  }
#endif
  
  if (tlsIndex == BAD_TLS_INDEX) {
    PRStatus status;
    status = PR_NewThreadPrivateIndex( &tlsIndex, ThreadDestruct );
    NS_ASSERTION(status==0, "ScriptErrorService could not allocate TLS storage.");
  }
  sLock = new Mutex("nsExceptionService.sLock");

  
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  NS_ASSERTION(observerService, "Could not get observer service!");
  if (observerService)
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}

nsExceptionService::~nsExceptionService()
{
  Shutdown();
  
#ifdef DEBUG
  PR_ATOMIC_DECREMENT(&totalInstances);
#endif
}


void nsExceptionService::ThreadDestruct( void *data )
{
  if (!sLock) {
    NS_WARNING("nsExceptionService ignoring thread destruction after shutdown");
    return;
  }
  DropThread( (nsExceptionManager *)data );
}


void nsExceptionService::Shutdown()
{
  mProviders.Reset();
  if (sLock) {
    DropAllThreads();
    delete sLock;
    sLock = nullptr;
  }
  PR_SetThreadPrivate(tlsIndex, nullptr);
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
    if (mgr == nullptr) {
        
        mgr = new nsExceptionManager(this);
        PR_SetThreadPrivate(tlsIndex, mgr);
        
        AddThread(mgr);
    }
    *aCurrentScriptManager = mgr;
    NS_ADDREF(*aCurrentScriptManager);
    return NS_OK;
}


NS_IMETHODIMP nsExceptionService::RegisterExceptionProvider(nsIExceptionProvider *provider, uint32_t errorModule)
{
    CHECK_SERVICE_USE_OK();

    nsProviderKey key(errorModule);
    if (mProviders.Put(&key, provider)) {
        NS_WARNING("Registration of exception provider overwrote another provider with the same module code!");
    }
    return NS_OK;
}


NS_IMETHODIMP nsExceptionService::UnregisterExceptionProvider(nsIExceptionProvider *provider, uint32_t errorModule)
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
    MutexAutoLock lock(*sLock);
    thread->mNextThread = firstThread;
    firstThread = thread;
    NS_ADDREF(thread);
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
    MutexAutoLock lock(*sLock);
    DoDropThread(thread);
}

 void nsExceptionService::DropAllThreads()
{
    MutexAutoLock lock(*sLock);
    while (firstThread)
        DoDropThread(firstThread);
}
