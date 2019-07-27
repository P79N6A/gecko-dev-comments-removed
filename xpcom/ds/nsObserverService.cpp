





#include "mozilla/Logging.h"
#include "nsAutoPtr.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsObserverService.h"
#include "nsObserverList.h"
#include "nsThreadUtils.h"
#include "nsEnumeratorUtils.h"
#include "xpcpublic.h"
#include "mozilla/net/NeckoCommon.h"
#include "mozilla/Services.h"

#define NOTIFY_GLOBAL_OBSERVERS










static PRLogModuleInfo*
GetObserverServiceLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("ObserverService");
  }
  return sLog;
}
#define LOG(x)  MOZ_LOG(GetObserverServiceLog(), mozilla::LogLevel::Debug, x)

namespace mozilla {

struct SuspectObserver
{
  SuspectObserver(const char* aTopic, size_t aReferentCount)
    : topic(aTopic)
    , referentCount(aReferentCount)
  {
  }
  const char* topic;
  size_t referentCount;
};

struct ObserverServiceReferentCount
{
  ObserverServiceReferentCount()
    : numStrong(0)
    , numWeakAlive(0)
    , numWeakDead(0)
  {
  }
  size_t numStrong;
  size_t numWeakAlive;
  size_t numWeakDead;
  nsTArray<SuspectObserver> suspectObservers;
};

} 

using namespace mozilla;

PLDHashOperator
nsObserverService::CountReferents(nsObserverList* aObserverList,
                                  void* aClosure)
{
  if (!aObserverList) {
    return PL_DHASH_NEXT;
  }

  ObserverServiceReferentCount* referentCount =
    static_cast<ObserverServiceReferentCount*>(aClosure);

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
nsObserverService::CollectReports(nsIHandleReportCallback* aHandleReport,
                                  nsISupports* aData, bool aAnonymize)
{
  ObserverServiceReferentCount referentCount;
  mObserverTopicTable.EnumerateEntries(CountReferents, &referentCount);

  
  nsresult rv;
  for (uint32_t i = 0; i < referentCount.suspectObservers.Length(); i++) {
    SuspectObserver& suspect = referentCount.suspectObservers[i];
    nsPrintfCString suspectPath("observer-service-suspect/"
                                "referent(topic=%s)",
                                suspect.topic);
    rv = aHandleReport->Callback(
       EmptyCString(),
      suspectPath, KIND_OTHER, UNITS_COUNT, suspect.referentCount,
      NS_LITERAL_CSTRING("A topic with a suspiciously large number of "
                         "referents.  This may be symptomatic of a leak "
                         "if the number of referents is high with "
                         "respect to the number of windows."),
      aData);

    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  rv = aHandleReport->Callback(
          EmptyCString(),
         NS_LITERAL_CSTRING("observer-service/referent/strong"),
         KIND_OTHER, UNITS_COUNT, referentCount.numStrong,
         NS_LITERAL_CSTRING("The number of strong references held by the "
                            "observer service."),
         aData);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aHandleReport->Callback(
          EmptyCString(),
         NS_LITERAL_CSTRING("observer-service/referent/weak/alive"),
         KIND_OTHER, UNITS_COUNT, referentCount.numWeakAlive,
         NS_LITERAL_CSTRING("The number of weak references held by the "
                            "observer service that are still alive."),
         aData);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = aHandleReport->Callback(
          EmptyCString(),
         NS_LITERAL_CSTRING("observer-service/referent/weak/dead"),
         KIND_OTHER, UNITS_COUNT, referentCount.numWeakDead,
         NS_LITERAL_CSTRING("The number of weak references held by the "
                            "observer service that are dead."),
         aData);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}




NS_IMPL_ISUPPORTS(nsObserverService,
                  nsIObserverService,
                  nsObserverService,
                  nsIMemoryReporter)

nsObserverService::nsObserverService()
  : mShuttingDown(false)
{
}

nsObserverService::~nsObserverService(void)
{
  Shutdown();
}

void
nsObserverService::RegisterReporter()
{
#if !defined(MOZILLA_XPCOMRT_API)
  RegisterWeakMemoryReporter(this);
#endif 
}

void
nsObserverService::Shutdown()
{
#if !defined(MOZILLA_XPCOMRT_API)
  UnregisterWeakMemoryReporter(this);
#endif 

  mShuttingDown = true;

  mObserverTopicTable.Clear();
}

nsresult
nsObserverService::Create(nsISupports* aOuter, const nsIID& aIID,
                          void** aInstancePtr)
{
  LOG(("nsObserverService::Create()"));

  nsRefPtr<nsObserverService> os = new nsObserverService();

  if (!os) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  nsRefPtr<nsRunnableMethod<nsObserverService>> registerRunnable =
    NS_NewRunnableMethod(os, &nsObserverService::RegisterReporter);
  NS_DispatchToCurrentThread(registerRunnable);

  return os->QueryInterface(aIID, aInstancePtr);
}

#define NS_ENSURE_VALIDCALL \
    if (!NS_IsMainThread()) {                                     \
        MOZ_CRASH("Using observer service off the main thread!"); \
        return NS_ERROR_UNEXPECTED;                               \
    }                                                             \
    if (mShuttingDown) {                                          \
        NS_ERROR("Using observer service after XPCOM shutdown!"); \
        return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;                  \
    }

NS_IMETHODIMP
nsObserverService::AddObserver(nsIObserver* aObserver, const char* aTopic,
                               bool aOwnsWeak)
{
  LOG(("nsObserverService::AddObserver(%p: %s)",
       (void*)aObserver, aTopic));

  NS_ENSURE_VALIDCALL
  if (NS_WARN_IF(!aObserver) || NS_WARN_IF(!aTopic)) {
    return NS_ERROR_INVALID_ARG;
  }

  if (mozilla::net::IsNeckoChild() && !strncmp(aTopic, "http-on-", 8)) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsObserverList* observerList = mObserverTopicTable.PutEntry(aTopic);
  if (!observerList) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return observerList->AddObserver(aObserver, aOwnsWeak);
}

NS_IMETHODIMP
nsObserverService::RemoveObserver(nsIObserver* aObserver, const char* aTopic)
{
  LOG(("nsObserverService::RemoveObserver(%p: %s)",
       (void*)aObserver, aTopic));
  NS_ENSURE_VALIDCALL
  if (NS_WARN_IF(!aObserver) || NS_WARN_IF(!aTopic)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsObserverList* observerList = mObserverTopicTable.GetEntry(aTopic);
  if (!observerList) {
    return NS_ERROR_FAILURE;
  }

  

  nsCOMPtr<nsIObserver> kungFuDeathGrip(aObserver);
  return observerList->RemoveObserver(aObserver);
}

NS_IMETHODIMP
nsObserverService::EnumerateObservers(const char* aTopic,
                                      nsISimpleEnumerator** anEnumerator)
{
  NS_ENSURE_VALIDCALL
  if (NS_WARN_IF(!anEnumerator) || NS_WARN_IF(!aTopic)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsObserverList* observerList = mObserverTopicTable.GetEntry(aTopic);
  if (!observerList) {
    return NS_NewEmptyEnumerator(anEnumerator);
  }

  return observerList->GetObserverList(anEnumerator);
}


NS_IMETHODIMP nsObserverService::NotifyObservers(nsISupports* aSubject,
                                                 const char* aTopic,
                                                 const char16_t* aSomeData)
{
  LOG(("nsObserverService::NotifyObservers(%s)", aTopic));

  NS_ENSURE_VALIDCALL
  if (NS_WARN_IF(!aTopic)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsObserverList* observerList = mObserverTopicTable.GetEntry(aTopic);
  if (observerList) {
    observerList->NotifyObservers(aSubject, aTopic, aSomeData);
  }

#ifdef NOTIFY_GLOBAL_OBSERVERS
  observerList = mObserverTopicTable.GetEntry("*");
  if (observerList) {
    observerList->NotifyObservers(aSubject, aTopic, aSomeData);
  }
#endif

  return NS_OK;
}

#if !defined(MOZILLA_XPCOMRT_API)
static PLDHashOperator
AppendStrongObservers(nsObserverList* aObserverList, void* aClosure)
{
  nsCOMArray<nsIObserver>* array = static_cast<nsCOMArray<nsIObserver>*>(aClosure);

  if (aObserverList) {
    aObserverList->AppendStrongObservers(*array);
  }
  return PL_DHASH_NEXT;
}
#endif 

NS_IMETHODIMP
nsObserverService::UnmarkGrayStrongObservers()
{
  NS_ENSURE_VALIDCALL

#if !defined(MOZILLA_XPCOMRT_API)
  nsCOMArray<nsIObserver> strongObservers;
  mObserverTopicTable.EnumerateEntries(AppendStrongObservers, &strongObservers);

  for (uint32_t i = 0; i < strongObservers.Length(); ++i) {
    xpc_TryUnmarkWrappedGrayObject(strongObservers[i]);
  }
#endif 

  return NS_OK;
}
