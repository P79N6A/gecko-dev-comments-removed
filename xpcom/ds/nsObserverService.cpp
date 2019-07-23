





































#include "prlog.h"
#include "nsAutoPtr.h"
#include "nsIFactory.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsISimpleEnumerator.h"
#include "nsObserverService.h"
#include "nsObserverList.h"
#include "nsHashtable.h"
#include "nsThreadUtils.h"
#include "nsIWeakReference.h"
#include "nsEnumeratorUtils.h"

#define NOTIFY_GLOBAL_OBSERVERS

#if defined(PR_LOGGING)









  PRLogModuleInfo* observerServiceLog = PR_NewLogModule("ObserverService");
  #define LOG(x)  PR_LOG(observerServiceLog, PR_LOG_DEBUG, x)
#else
  #define LOG(x)
#endif 





NS_IMPL_THREADSAFE_ISUPPORTS2(nsObserverService, nsIObserverService, nsObserverService)

nsObserverService::nsObserverService() :
    mShuttingDown(PR_FALSE)
{
    mObserverTopicTable.Init();
}

nsObserverService::~nsObserverService(void)
{
    Shutdown();
}

void
nsObserverService::Shutdown()
{
    mShuttingDown = PR_TRUE;

    if (mObserverTopicTable.IsInitialized())
        mObserverTopicTable.Clear();
}

NS_METHOD
nsObserverService::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    LOG(("nsObserverService::Create()"));

    nsRefPtr<nsObserverService> os = new nsObserverService();

    if (!os || !os->mObserverTopicTable.IsInitialized())
        return NS_ERROR_OUT_OF_MEMORY;

    return os->QueryInterface(aIID, aInstancePtr);
}

#define NS_ENSURE_VALIDCALL \
    if (!NS_IsMainThread()) {                                     \
        NS_ERROR("Using observer service off the main thread!");  \
        return NS_ERROR_UNEXPECTED;                               \
    }                                                             \
    if (mShuttingDown) {                                          \
        NS_ERROR("Using observer service after XPCOM shutdown!"); \
        return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;                  \
    }

NS_IMETHODIMP
nsObserverService::AddObserver(nsIObserver* anObserver, const char* aTopic,
                               PRBool ownsWeak)
{
    LOG(("nsObserverService::AddObserver(%p: %s)",
         (void*) anObserver, aTopic));

    NS_ENSURE_VALIDCALL
    NS_ENSURE_ARG(anObserver && aTopic);

    nsObserverList *observerList = mObserverTopicTable.PutEntry(aTopic);
    if (!observerList)
        return NS_ERROR_OUT_OF_MEMORY;

    return observerList->AddObserver(anObserver, ownsWeak);
}

NS_IMETHODIMP
nsObserverService::RemoveObserver(nsIObserver* anObserver, const char* aTopic)
{
    LOG(("nsObserverService::RemoveObserver(%p: %s)",
         (void*) anObserver, aTopic));
    NS_ENSURE_VALIDCALL
    NS_ENSURE_ARG(anObserver && aTopic);

    nsObserverList *observerList = mObserverTopicTable.GetEntry(aTopic);
    if (!observerList)
        return NS_ERROR_FAILURE;

    

    nsCOMPtr<nsIObserver> kungFuDeathGrip(anObserver);
    return observerList->RemoveObserver(anObserver);
}

NS_IMETHODIMP
nsObserverService::EnumerateObservers(const char* aTopic,
                                      nsISimpleEnumerator** anEnumerator)
{
    NS_ENSURE_VALIDCALL
    NS_ENSURE_ARG(aTopic && anEnumerator);

    nsObserverList *observerList = mObserverTopicTable.GetEntry(aTopic);
    if (!observerList)
        return NS_NewEmptyEnumerator(anEnumerator);

    return observerList->GetObserverList(anEnumerator);
}


NS_IMETHODIMP nsObserverService::NotifyObservers(nsISupports *aSubject,
                                                 const char *aTopic,
                                                 const PRUnichar *someData)
{
    LOG(("nsObserverService::NotifyObservers(%s)", aTopic));

    NS_ENSURE_VALIDCALL
    NS_ENSURE_ARG(aTopic);

    nsObserverList *observerList = mObserverTopicTable.GetEntry(aTopic);
    if (observerList)
        observerList->NotifyObservers(aSubject, aTopic, someData);

#ifdef NOTIFY_GLOBAL_OBSERVERS
    observerList = mObserverTopicTable.GetEntry("*");
    if (observerList)
        observerList->NotifyObservers(aSubject, aTopic, someData);
#endif

    return NS_OK;
}

