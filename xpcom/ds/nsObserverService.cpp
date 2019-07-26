




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
#include "nsIMemoryReporter.h"
#include "mozilla/net/NeckoCommon.h"
#include "mozilla/Services.h"

#define NOTIFY_GLOBAL_OBSERVERS

#if defined(PR_LOGGING)









static PRLogModuleInfo*
GetObserverServiceLog()
{
    static PRLogModuleInfo *sLog;
    if (!sLog)
        sLog = PR_NewLogModule("ObserverService");
    return sLog;
}
  #define LOG(x)  PR_LOG(GetObserverServiceLog(), PR_LOG_DEBUG, x)
#else
  #define LOG(x)
#endif 

namespace mozilla {

class ObserverServiceReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYREPORTER
protected:
    static const size_t kSuspectReferentCount = 1000;
    static PLDHashOperator CountReferents(nsObserverList* aObserverList,
                                          void* aClosure);
};

NS_IMPL_ISUPPORTS1(ObserverServiceReporter, nsIMemoryReporter)

NS_IMETHODIMP
ObserverServiceReporter::GetName(nsACString& aName)
{
    aName.AssignLiteral("observer-service");
    return NS_OK;
}

struct SuspectObserver {
    SuspectObserver(const char* aTopic, size_t aReferentCount)
        : topic(aTopic), referentCount(aReferentCount) {}
    const char* topic;
    size_t referentCount;
};

struct ReferentCount {
    ReferentCount() : numStrong(0), numWeakAlive(0), numWeakDead(0) {}
    size_t numStrong;
    size_t numWeakAlive;
    size_t numWeakDead;
    nsTArray<SuspectObserver> suspectObservers;
};

PLDHashOperator
ObserverServiceReporter::CountReferents(nsObserverList* aObserverList,
                                        void* aClosure)
{
    if (!aObserverList) {
        return PL_DHASH_NEXT;
    }

    ReferentCount* referentCount = static_cast<ReferentCount*>(aClosure);

    size_t numStrong = 0;
    size_t numWeakAlive = 0;
    size_t numWeakDead = 0;

    nsTArray<ObserverRef>& observers = aObserverList->mObservers;
    for (uint32_t i = 0; i < observers.Length(); i++) {
        if (observers[i].isWeakRef) {
            nsCOMPtr<nsIObserver> observerRef(
                do_QueryReferent(observers[i].asWeak()));
            if (observerRef) {
                numWeakAlive++;
            } else {
                numWeakDead++;
            }
        } else {
            numStrong++;
        }
    }

    referentCount->numStrong += numStrong;
    referentCount->numWeakAlive += numWeakAlive;
    referentCount->numWeakDead += numWeakDead;

    
    
    size_t total = numStrong + numWeakAlive + numWeakDead;
    if (total > kSuspectReferentCount) {
        SuspectObserver suspect(aObserverList->GetKey(), total);
        referentCount->suspectObservers.AppendElement(suspect);
    }

    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
ObserverServiceReporter::CollectReports(nsIMemoryReporterCallback* cb,
                                        nsISupports* aClosure)
{
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    nsObserverService* service = static_cast<nsObserverService*>(os.get());
    if (!service) {
        return NS_OK;
    }

    ReferentCount referentCount;
    service->mObserverTopicTable.EnumerateEntries(CountReferents,
                                                  &referentCount);

    nsresult rv;
    for (uint32_t i = 0; i < referentCount.suspectObservers.Length(); i++) {
        SuspectObserver& suspect = referentCount.suspectObservers[i];
        nsPrintfCString suspectPath("observer-service-suspect/"
                                    "referent(topic=%s)",
                                    suspect.topic);
        rv = cb->Callback( EmptyCString(),
            suspectPath,
            nsIMemoryReporter::KIND_OTHER,
            nsIMemoryReporter::UNITS_COUNT,
            suspect.referentCount,
            NS_LITERAL_CSTRING("A topic with a suspiciously large number "
                               "referents (symptom of a leak)."),
            aClosure);

        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = cb->Callback( EmptyCString(),
        NS_LITERAL_CSTRING("observer-service/referent/strong"),
        nsIMemoryReporter::KIND_OTHER,
        nsIMemoryReporter::UNITS_COUNT,
        referentCount.numStrong,
        NS_LITERAL_CSTRING("The number of strong references held by the "
                           "observer service."),
        aClosure);

    NS_ENSURE_SUCCESS(rv, rv);

    rv = cb->Callback( EmptyCString(),
        NS_LITERAL_CSTRING("observer-service/referent/weak/alive"),
        nsIMemoryReporter::KIND_OTHER,
        nsIMemoryReporter::UNITS_COUNT,
        referentCount.numWeakAlive,
        NS_LITERAL_CSTRING("The number of weak references held by the "
                           "observer service that are still alive."),
        aClosure);

    NS_ENSURE_SUCCESS(rv, rv);

    rv = cb->Callback( EmptyCString(),
        NS_LITERAL_CSTRING("observer-service/referent/weak/dead"),
        nsIMemoryReporter::KIND_OTHER,
        nsIMemoryReporter::UNITS_COUNT,
        referentCount.numWeakDead,
        NS_LITERAL_CSTRING("The number of weak references held by the "
                           "observer service that are dead."),
        aClosure);

    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

} 

using namespace mozilla;





NS_IMPL_ISUPPORTS2(nsObserverService, nsIObserverService, nsObserverService)

nsObserverService::nsObserverService() :
    mShuttingDown(false), mReporter(nullptr)
{
    mObserverTopicTable.Init();
}

nsObserverService::~nsObserverService(void)
{
    Shutdown();
}

void
nsObserverService::RegisterReporter()
{
    mReporter = new ObserverServiceReporter();
    NS_RegisterMemoryReporter(mReporter);
}

void
nsObserverService::Shutdown()
{
    if (mReporter) {
        NS_UnregisterMemoryReporter(mReporter);
    }

    mShuttingDown = true;

    if (mObserverTopicTable.IsInitialized())
        mObserverTopicTable.Clear();
}

nsresult
nsObserverService::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    LOG(("nsObserverService::Create()"));

    nsRefPtr<nsObserverService> os = new nsObserverService();

    if (!os || !os->mObserverTopicTable.IsInitialized())
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    nsRefPtr<nsRunnableMethod<nsObserverService> > registerRunnable =
        NS_NewRunnableMethod(os, &nsObserverService::RegisterReporter);
    NS_DispatchToCurrentThread(registerRunnable);

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
                               bool ownsWeak)
{
    LOG(("nsObserverService::AddObserver(%p: %s)",
         (void*) anObserver, aTopic));

    NS_ENSURE_VALIDCALL
    NS_ENSURE_ARG(anObserver && aTopic);

    if (mozilla::net::IsNeckoChild() && !strncmp(aTopic, "http-on-", 8)) {
      return NS_ERROR_NOT_IMPLEMENTED;
    }

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

static PLDHashOperator
UnmarkGrayObserverEntry(nsObserverList* aObserverList, void* aClosure)
{
    if (aObserverList) {
        aObserverList->UnmarkGrayStrongObservers();
    }
    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsObserverService::UnmarkGrayStrongObservers()
{
    NS_ENSURE_VALIDCALL

    mObserverTopicTable.EnumerateEntries(UnmarkGrayObserverEntry, nullptr);

    return NS_OK;
}

