






#include "nsSHistory.h"
#include <algorithm>


#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"


#include "nsILayoutHistoryState.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsISHContainer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIURI.h"
#include "nsIContentViewer.h"
#include "nsIObserverService.h"
#include "prclist.h"
#include "mozilla/Services.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsDocShell.h"
#include "mozilla/Attributes.h"
#include "nsISHEntry.h"
#include "nsISHTransaction.h"
#include "nsISHistoryListener.h"
#include "nsComponentManagerUtils.h"


#include "prsystem.h"
#include "mozilla/MathAlgorithms.h"

using namespace mozilla;

#define PREF_SHISTORY_SIZE "browser.sessionhistory.max_entries"
#define PREF_SHISTORY_MAX_TOTAL_VIEWERS "browser.sessionhistory.max_total_viewers"

static const char* kObservedPrefs[] = {
  PREF_SHISTORY_SIZE,
  PREF_SHISTORY_MAX_TOTAL_VIEWERS,
  nullptr
};

static int32_t  gHistoryMaxSize = 50;

static const int32_t  gHistoryMaxViewers = 3;

static PRCList gSHistoryList;


int32_t nsSHistory::sHistoryMaxTotalViewers = -1;



static uint32_t gTouchCounter = 0;

#ifdef PR_LOGGING

static PRLogModuleInfo*
GetSHistoryLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("nsSHistory");
  return sLog;
}
#define LOG(format) PR_LOG(GetSHistoryLog(), PR_LOG_DEBUG, format)







#define LOG_SPEC(format, uri)                              \
  PR_BEGIN_MACRO                                           \
    if (PR_LOG_TEST(GetSHistoryLog(), PR_LOG_DEBUG)) {     \
      nsAutoCString _specStr(NS_LITERAL_CSTRING("(null)"));\
      if (uri) {                                           \
        uri->GetSpec(_specStr);                            \
      }                                                    \
      const char* _spec = _specStr.get();                  \
      LOG(format);                                         \
    }                                                      \
  PR_END_MACRO







#define LOG_SHENTRY_SPEC(format, shentry)                  \
  PR_BEGIN_MACRO                                           \
    if (PR_LOG_TEST(GetSHistoryLog(), PR_LOG_DEBUG)) {     \
      nsCOMPtr<nsIURI> uri;                                \
      shentry->GetURI(getter_AddRefs(uri));                \
      LOG_SPEC(format, uri);                               \
    }                                                      \
  PR_END_MACRO

#else 

#define LOG(format)
#define LOG_SPEC(format, uri)
#define LOG_SHENTRY_SPEC(format, shentry)

#endif 


#define ITERATE_LISTENERS(body)                            \
  PR_BEGIN_MACRO                                           \
  {                                                        \
    nsAutoTObserverArray<nsWeakPtr, 2>::EndLimitedIterator \
      iter(mListeners);                                    \
    while (iter.HasMore()) {                               \
      nsCOMPtr<nsISHistoryListener> listener =             \
        do_QueryReferent(iter.GetNext());                  \
      if (listener) {                                      \
        body                                               \
      }                                                    \
    }                                                      \
  }                                                        \
  PR_END_MACRO


#define NOTIFY_LISTENERS(method, args)                     \
  ITERATE_LISTENERS(                                       \
    listener->method args;                                 \
  );




#define NOTIFY_LISTENERS_CANCELABLE(method, retval, args)  \
  PR_BEGIN_MACRO                                           \
  {                                                        \
    bool canceled = false;                                 \
    retval = true;                                         \
    ITERATE_LISTENERS(                                     \
      listener->method args;                               \
      if (!retval) {                                       \
        canceled = true;                                   \
      }                                                    \
    );                                                     \
    if (canceled) {                                        \
      retval = false;                                      \
    }                                                      \
  }                                                        \
  PR_END_MACRO

enum HistCmd{
  HIST_CMD_BACK,
  HIST_CMD_FORWARD,
  HIST_CMD_GOTOINDEX,
  HIST_CMD_RELOAD
} ;





class nsSHistoryObserver final : public nsIObserver
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsSHistoryObserver() {}

protected:
  ~nsSHistoryObserver() {}
};

StaticRefPtr<nsSHistoryObserver> gObserver;

NS_IMPL_ISUPPORTS(nsSHistoryObserver, nsIObserver)

NS_IMETHODIMP
nsSHistoryObserver::Observe(nsISupports *aSubject, const char *aTopic,
                            const char16_t *aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsSHistory::UpdatePrefs();
    nsSHistory::GloballyEvictContentViewers();
  } else if (!strcmp(aTopic, "cacheservice:empty-cache") ||
             !strcmp(aTopic, "memory-pressure")) {
    nsSHistory::GloballyEvictAllContentViewers();
  }

  return NS_OK;
}

namespace {

already_AddRefed<nsIContentViewer>
GetContentViewerForTransaction(nsISHTransaction *aTrans)
{
  nsCOMPtr<nsISHEntry> entry;
  aTrans->GetSHEntry(getter_AddRefs(entry));
  if (!entry) {
    return nullptr;
  }

  nsCOMPtr<nsISHEntry> ownerEntry;
  nsCOMPtr<nsIContentViewer> viewer;
  entry->GetAnyContentViewer(getter_AddRefs(ownerEntry),
                             getter_AddRefs(viewer));
  return viewer.forget();
}

void
EvictContentViewerForTransaction(nsISHTransaction *aTrans)
{
  nsCOMPtr<nsISHEntry> entry;
  aTrans->GetSHEntry(getter_AddRefs(entry));
  nsCOMPtr<nsIContentViewer> viewer;
  nsCOMPtr<nsISHEntry> ownerEntry;
  entry->GetAnyContentViewer(getter_AddRefs(ownerEntry),
                             getter_AddRefs(viewer));
  if (viewer) {
    NS_ASSERTION(ownerEntry,
                 "Content viewer exists but its SHEntry is null");

    LOG_SHENTRY_SPEC(("Evicting content viewer 0x%p for "
                      "owning SHEntry 0x%p at %s.",
                      viewer.get(), ownerEntry.get(), _spec), ownerEntry);

    
    
    ownerEntry->SetContentViewer(nullptr);
    ownerEntry->SyncPresentationState();
    viewer->Destroy();
  }
}

} 





nsSHistory::nsSHistory() : mListRoot(nullptr), mIndex(-1), mLength(0), mRequestedIndex(-1)
{
  
  PR_APPEND_LINK(this, &gSHistoryList);
}


nsSHistory::~nsSHistory()
{
  
  PR_REMOVE_LINK(this);
}





NS_IMPL_ADDREF(nsSHistory)
NS_IMPL_RELEASE(nsSHistory)

NS_INTERFACE_MAP_BEGIN(nsSHistory)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISHistory)
   NS_INTERFACE_MAP_ENTRY(nsISHistory)
   NS_INTERFACE_MAP_ENTRY(nsIWebNavigation)
   NS_INTERFACE_MAP_ENTRY(nsISHistoryInternal)
NS_INTERFACE_MAP_END






uint32_t
nsSHistory::CalcMaxTotalViewers()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint64_t bytes = PR_GetPhysicalMemorySize();

  if (bytes == 0)
    return 0;

  
  
  
  if (bytes > INT64_MAX)
    bytes = INT64_MAX;

  double kBytesD = (double)(bytes >> 10);

  
  
  
  uint32_t viewers = 0;
  double x = std::log(kBytesD)/std::log(2.0) - 14;
  if (x > 0) {
    viewers    = (uint32_t)(x * x - x + 2.001); 
    viewers   /= 4;
  }

  
  if (viewers > 8) {
    viewers = 8;
  }
  return viewers;
}


void
nsSHistory::UpdatePrefs()
{
  Preferences::GetInt(PREF_SHISTORY_SIZE, &gHistoryMaxSize);
  Preferences::GetInt(PREF_SHISTORY_MAX_TOTAL_VIEWERS,
                      &sHistoryMaxTotalViewers);
  
  
  if (sHistoryMaxTotalViewers < 0) {
    sHistoryMaxTotalViewers = CalcMaxTotalViewers();
  }
}


nsresult
nsSHistory::Startup()
{
  UpdatePrefs();

  
  
  int32_t defaultHistoryMaxSize =
    Preferences::GetDefaultInt(PREF_SHISTORY_SIZE, 50);
  if (gHistoryMaxSize < defaultHistoryMaxSize) {
    gHistoryMaxSize = defaultHistoryMaxSize;
  }
  
  
  
  if (!gObserver) {
    gObserver = new nsSHistoryObserver();
    Preferences::AddStrongObservers(gObserver, kObservedPrefs);

    nsCOMPtr<nsIObserverService> obsSvc =
      mozilla::services::GetObserverService();
    if (obsSvc) {
      
      
      obsSvc->AddObserver(gObserver,
                          "cacheservice:empty-cache", false);

      
      obsSvc->AddObserver(gObserver, "memory-pressure", false);
    }
  }

  
  PR_INIT_CLIST(&gSHistoryList);
  return NS_OK;
}


void
nsSHistory::Shutdown()
{
  if (gObserver) {
    Preferences::RemoveObservers(gObserver, kObservedPrefs);
    nsCOMPtr<nsIObserverService> obsSvc =
      mozilla::services::GetObserverService();
    if (obsSvc) {
      obsSvc->RemoveObserver(gObserver, "cacheservice:empty-cache");
      obsSvc->RemoveObserver(gObserver, "memory-pressure");
    }
    gObserver = nullptr;
  }
}




NS_IMETHODIMP
nsSHistory::AddEntry(nsISHEntry * aSHEntry, bool aPersist)
{
  NS_ENSURE_ARG(aSHEntry);

  nsCOMPtr<nsISHTransaction> currentTxn;

  if(mListRoot)
    GetTransactionAtIndex(mIndex, getter_AddRefs(currentTxn));

  bool currentPersist = true;
  if(currentTxn)
    currentTxn->GetPersist(&currentPersist);

  int32_t currentIndex = mIndex;

  if(!currentPersist)
  {
    NOTIFY_LISTENERS(OnHistoryReplaceEntry, (currentIndex));
    NS_ENSURE_SUCCESS(currentTxn->SetSHEntry(aSHEntry),NS_ERROR_FAILURE);
    currentTxn->SetPersist(aPersist);
    return NS_OK;
  }

  nsCOMPtr<nsISHTransaction> txn(do_CreateInstance(NS_SHTRANSACTION_CONTRACTID));
  NS_ENSURE_TRUE(txn, NS_ERROR_FAILURE);

  nsCOMPtr<nsIURI> uri;
  aSHEntry->GetURI(getter_AddRefs(uri));
  NOTIFY_LISTENERS(OnHistoryNewEntry, (uri));

  
  
  if (currentIndex != mIndex) {
    GetTransactionAtIndex(mIndex, getter_AddRefs(currentTxn));
  }

  
  
  txn->SetPersist(aPersist);
  NS_ENSURE_SUCCESS(txn->Create(aSHEntry, currentTxn), NS_ERROR_FAILURE);
   
  
  
  
  mLength = (++mIndex + 1);

  
  if(!mListRoot)
    mListRoot = txn;

  
  if ((gHistoryMaxSize >= 0) && (mLength > gHistoryMaxSize))
    PurgeHistory(mLength-gHistoryMaxSize);
  
  RemoveDynEntries(mIndex - 1, mIndex);
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetCount(int32_t * aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = mLength;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetIndex(int32_t * aResult)
{
  NS_PRECONDITION(aResult, "null out param?");
  *aResult = mIndex;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetRequestedIndex(int32_t * aResult)
{
  NS_PRECONDITION(aResult, "null out param?");
  *aResult = mRequestedIndex;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetEntryAtIndex(int32_t aIndex, bool aModifyIndex, nsISHEntry** aResult)
{
  nsresult rv;
  nsCOMPtr<nsISHTransaction> txn;

  
  rv = GetTransactionAtIndex(aIndex, getter_AddRefs(txn));
  if (NS_SUCCEEDED(rv) && txn) {
    
    rv = txn->GetSHEntry(aResult);
    if (NS_SUCCEEDED(rv) && (*aResult)) {
      
      if (aModifyIndex) {
        mIndex = aIndex;
      }
    } 
  }  
  return rv;
}


NS_IMETHODIMP
nsSHistory::GetTransactionAtIndex(int32_t aIndex, nsISHTransaction ** aResult)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aResult);

  if ((mLength <= 0) || (aIndex < 0) || (aIndex >= mLength))
    return NS_ERROR_FAILURE;

  if (!mListRoot) 
    return NS_ERROR_FAILURE;

  if (aIndex == 0)
  {
    *aResult = mListRoot;
    NS_ADDREF(*aResult);
    return NS_OK;
  } 
  int32_t   cnt=0;
  nsCOMPtr<nsISHTransaction>  tempPtr;

  rv = GetRootTransaction(getter_AddRefs(tempPtr));
  if (NS_FAILED(rv) || !tempPtr)
    return NS_ERROR_FAILURE;

  while(1) {
    nsCOMPtr<nsISHTransaction> ptr;
    rv = tempPtr->GetNext(getter_AddRefs(ptr));
    if (NS_SUCCEEDED(rv) && ptr) {
      cnt++;
      if (cnt == aIndex) {
        ptr.forget(aResult);
        break;
      }
      else {
        tempPtr = ptr;
        continue;
      }
    }  
    else 
      return NS_ERROR_FAILURE;
  }  
  
  return NS_OK;
}



NS_IMETHODIMP
nsSHistory::GetIndexOfEntry(nsISHEntry* aSHEntry, int32_t* aResult) {
  NS_ENSURE_ARG(aSHEntry);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = -1;

  if (mLength <= 0) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsISHTransaction> currentTxn;
  int32_t cnt = 0;

  nsresult rv = GetRootTransaction(getter_AddRefs(currentTxn));
  if (NS_FAILED(rv) || !currentTxn) {
    return NS_ERROR_FAILURE;
  }

  while (true) {
    nsCOMPtr<nsISHEntry> entry;
    rv = currentTxn->GetSHEntry(getter_AddRefs(entry));
    if (NS_FAILED(rv) || !entry) {
      return NS_ERROR_FAILURE;
    }

    if (aSHEntry == entry) {
      *aResult = cnt;
      break;
    }

    rv = currentTxn->GetNext(getter_AddRefs(currentTxn));
    if (NS_FAILED(rv) || !currentTxn) {
      return NS_ERROR_FAILURE;
    }

    cnt++;
  }

  return NS_OK;
}


#ifdef DEBUG
nsresult
nsSHistory::PrintHistory()
{

  nsCOMPtr<nsISHTransaction>   txn;
  int32_t index = 0;
  nsresult rv;

  if (!mListRoot) 
    return NS_ERROR_FAILURE;

  txn = mListRoot;
    
  while (1) {
    if (!txn)
      break;
    nsCOMPtr<nsISHEntry>  entry;
    rv = txn->GetSHEntry(getter_AddRefs(entry));
    if (NS_FAILED(rv) && !entry)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsILayoutHistoryState> layoutHistoryState;
    nsCOMPtr<nsIURI>  uri;
    nsXPIDLString title;

    entry->GetLayoutHistoryState(getter_AddRefs(layoutHistoryState));
    entry->GetURI(getter_AddRefs(uri));
    entry->GetTitle(getter_Copies(title));

#if 0
    nsAutoCString url;
    if (uri)
     uri->GetSpec(url);

    printf("**** SH Transaction #%d, Entry = %x\n", index, entry.get());
    printf("\t\t URL = %s\n", url.get());

    printf("\t\t Title = %s\n", NS_LossyConvertUTF16toASCII(title).get());
    printf("\t\t layout History Data = %x\n", layoutHistoryState.get());
#endif

    nsCOMPtr<nsISHTransaction> next;
    rv = txn->GetNext(getter_AddRefs(next));
    if (NS_SUCCEEDED(rv) && next) {
      txn = next;
      index++;
      continue;
    }
    else
      break;
  }

  return NS_OK;
}
#endif


NS_IMETHODIMP
nsSHistory::GetRootTransaction(nsISHTransaction ** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult=mListRoot;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetMaxLength(int32_t * aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = gHistoryMaxSize;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::SetMaxLength(int32_t aMaxSize)
{
  if (aMaxSize < 0)
    return NS_ERROR_ILLEGAL_VALUE;

  gHistoryMaxSize = aMaxSize;
  if (mLength > aMaxSize)
    PurgeHistory(mLength-aMaxSize);
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::PurgeHistory(int32_t aEntries)
{
  if (mLength <= 0 || aEntries <= 0)
    return NS_ERROR_FAILURE;

  aEntries = std::min(aEntries, mLength);
  
  bool purgeHistory = true;
  NOTIFY_LISTENERS_CANCELABLE(OnHistoryPurge, purgeHistory,
                              (aEntries, &purgeHistory));

  if (!purgeHistory) {
    
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }

  int32_t cnt = 0;
  while (cnt < aEntries) {
    nsCOMPtr<nsISHTransaction> nextTxn;
    if (mListRoot) {
      mListRoot->GetNext(getter_AddRefs(nextTxn));
      mListRoot->SetNext(nullptr);
    }
    mListRoot = nextTxn;
    if (mListRoot) {
      mListRoot->SetPrev(nullptr);
    }
    cnt++;        
  }
  mLength -= cnt;
  mIndex -= cnt;

  
  
  if (mIndex < -1) {
    mIndex = -1;
  }

  if (mRootDocShell)
    mRootDocShell->HistoryPurged(cnt);

  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::AddSHistoryListener(nsISHistoryListener * aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  
  
  
  nsWeakPtr listener = do_GetWeakReference(aListener);
  if (!listener) return NS_ERROR_FAILURE;

  return mListeners.AppendElementUnlessExists(listener) ?
    NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsSHistory::RemoveSHistoryListener(nsISHistoryListener * aListener)
{
  
  
  nsWeakPtr listener = do_GetWeakReference(aListener);
  mListeners.RemoveElement(listener);
  return NS_OK;
}





NS_IMETHODIMP
nsSHistory::ReplaceEntry(int32_t aIndex, nsISHEntry * aReplaceEntry)
{
  NS_ENSURE_ARG(aReplaceEntry);
  nsresult rv;
  nsCOMPtr<nsISHTransaction> currentTxn;

  if (!mListRoot) 
    return NS_ERROR_FAILURE;

  rv = GetTransactionAtIndex(aIndex, getter_AddRefs(currentTxn));

  if(currentTxn)
  {
    NOTIFY_LISTENERS(OnHistoryReplaceEntry, (aIndex));

    
    rv = currentTxn->SetSHEntry(aReplaceEntry);
    rv = currentTxn->SetPersist(true);
  }
  return rv;
}

NS_IMETHODIMP
nsSHistory::NotifyOnHistoryReload(nsIURI* aReloadURI, uint32_t aReloadFlags,
                                  bool* aCanReload)
{
  NOTIFY_LISTENERS_CANCELABLE(OnHistoryReload, *aCanReload,
                              (aReloadURI, aReloadFlags, aCanReload));
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::EvictOutOfRangeContentViewers(int32_t aIndex)
{
  
  EvictOutOfRangeWindowContentViewers(aIndex);
  
  GloballyEvictContentViewers();
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::EvictAllContentViewers()
{
  
  
  nsCOMPtr<nsISHTransaction> trans = mListRoot;
  while (trans) {
    EvictContentViewerForTransaction(trans);

    nsISHTransaction *temp = trans;
    temp->GetNext(getter_AddRefs(trans));
  }

  return NS_OK;
}







NS_IMETHODIMP
nsSHistory::GetCanGoBack(bool * aCanGoBack)
{
  NS_ENSURE_ARG_POINTER(aCanGoBack);
  *aCanGoBack = false;

  int32_t index = -1;
  NS_ENSURE_SUCCESS(GetIndex(&index), NS_ERROR_FAILURE);
  if(index > 0)
     *aCanGoBack = true;

  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetCanGoForward(bool * aCanGoForward)
{
  NS_ENSURE_ARG_POINTER(aCanGoForward);
  *aCanGoForward = false;

  int32_t index = -1;
  int32_t count = -1;

  NS_ENSURE_SUCCESS(GetIndex(&index), NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(GetCount(&count), NS_ERROR_FAILURE);

  if((index >= 0) && (index < (count - 1)))
    *aCanGoForward = true;

  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GoBack()
{
  bool canGoBack = false;

  GetCanGoBack(&canGoBack);
  if (!canGoBack)  
    return NS_ERROR_UNEXPECTED;
  return LoadEntry(mIndex-1, nsIDocShellLoadInfo::loadHistory, HIST_CMD_BACK);
}


NS_IMETHODIMP
nsSHistory::GoForward()
{
  bool canGoForward = false;

  GetCanGoForward(&canGoForward);
  if (!canGoForward)  
    return NS_ERROR_UNEXPECTED;
  return LoadEntry(mIndex+1, nsIDocShellLoadInfo::loadHistory, HIST_CMD_FORWARD);
}

NS_IMETHODIMP
nsSHistory::Reload(uint32_t aReloadFlags)
{
  nsDocShellInfoLoadType loadType;
  if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY && 
      aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE)
  {
    loadType = nsIDocShellLoadInfo::loadReloadBypassProxyAndCache;
  }
  else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY)
  {
    loadType = nsIDocShellLoadInfo::loadReloadBypassProxy;
  }
  else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE)
  {
    loadType = nsIDocShellLoadInfo::loadReloadBypassCache;
  }
  else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_CHARSET_CHANGE)
  {
    loadType = nsIDocShellLoadInfo::loadReloadCharsetChange;
  }
  else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_ALLOW_MIXED_CONTENT)
  {
    loadType = nsIDocShellLoadInfo::loadReloadMixedContent;
  }
  else
  {
    loadType = nsIDocShellLoadInfo::loadReloadNormal;
  }

  
  
  
  
  bool canNavigate = true;
  nsCOMPtr<nsIURI> currentURI;
  GetCurrentURI(getter_AddRefs(currentURI));
  NOTIFY_LISTENERS_CANCELABLE(OnHistoryReload, canNavigate,
                              (currentURI, aReloadFlags, &canNavigate));
  if (!canNavigate)
    return NS_OK;

  return LoadEntry(mIndex, loadType, HIST_CMD_RELOAD);
}

NS_IMETHODIMP
nsSHistory::ReloadCurrentEntry()
{
  
  bool canNavigate = true;
  nsCOMPtr<nsIURI> currentURI;
  GetCurrentURI(getter_AddRefs(currentURI));
  NOTIFY_LISTENERS_CANCELABLE(OnHistoryGotoIndex, canNavigate,
                              (mIndex, currentURI, &canNavigate));
  if (!canNavigate)
    return NS_OK;

  return LoadEntry(mIndex, nsIDocShellLoadInfo::loadHistory, HIST_CMD_RELOAD);
}

void
nsSHistory::EvictOutOfRangeWindowContentViewers(int32_t aIndex)
{
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (aIndex < 0) {
    return;
  }
  NS_ENSURE_TRUE_VOID(aIndex < mLength);

  
  int32_t startSafeIndex = std::max(0, aIndex - gHistoryMaxViewers);
  int32_t endSafeIndex = std::min(mLength, aIndex + gHistoryMaxViewers);

  LOG(("EvictOutOfRangeWindowContentViewers(index=%d), "
       "mLength=%d. Safe range [%d, %d]",
       aIndex, mLength, startSafeIndex, endSafeIndex)); 

  
  
  
  nsCOMArray<nsIContentViewer> safeViewers;
  nsCOMPtr<nsISHTransaction> trans;
  GetTransactionAtIndex(startSafeIndex, getter_AddRefs(trans));
  for (int32_t i = startSafeIndex; trans && i <= endSafeIndex; i++) {
    nsCOMPtr<nsIContentViewer> viewer = GetContentViewerForTransaction(trans);
    safeViewers.AppendObject(viewer);
    nsISHTransaction *temp = trans;
    temp->GetNext(getter_AddRefs(trans));
  }

  
  GetTransactionAtIndex(0, getter_AddRefs(trans));
  while (trans) {
    nsCOMPtr<nsIContentViewer> viewer = GetContentViewerForTransaction(trans);
    if (safeViewers.IndexOf(viewer) == -1) {
      EvictContentViewerForTransaction(trans);
    }

    nsISHTransaction *temp = trans;
    temp->GetNext(getter_AddRefs(trans));
  }
}

namespace {

class TransactionAndDistance
{
public:
  TransactionAndDistance(nsISHTransaction *aTrans, uint32_t aDist)
    : mTransaction(aTrans)
    , mDistance(aDist)
  {
    mViewer = GetContentViewerForTransaction(aTrans);
    NS_ASSERTION(mViewer, "Transaction should have a content viewer");

    nsCOMPtr<nsISHEntry> shentry;
    mTransaction->GetSHEntry(getter_AddRefs(shentry));

    nsCOMPtr<nsISHEntryInternal> shentryInternal = do_QueryInterface(shentry);
    if (shentryInternal) {
      shentryInternal->GetLastTouched(&mLastTouched);
    } else {
      NS_WARNING("Can't cast to nsISHEntryInternal?");
      mLastTouched = 0;
    }
  }

  bool operator<(const TransactionAndDistance &aOther) const
  {
    
    if (aOther.mDistance != this->mDistance) {
      return this->mDistance < aOther.mDistance;
    }

    return this->mLastTouched < aOther.mLastTouched;
  }

  bool operator==(const TransactionAndDistance &aOther) const
  {
    
    
    
    return aOther.mDistance == this->mDistance &&
           aOther.mLastTouched == this->mLastTouched;
  }

  nsCOMPtr<nsISHTransaction> mTransaction;
  nsCOMPtr<nsIContentViewer> mViewer;
  uint32_t mLastTouched;
  int32_t mDistance;
};

} 


void
nsSHistory::GloballyEvictContentViewers()
{
  
  
  

  nsTArray<TransactionAndDistance> transactions;

  PRCList* listEntry = PR_LIST_HEAD(&gSHistoryList);
  while (listEntry != &gSHistoryList) {
    nsSHistory* shist = static_cast<nsSHistory*>(listEntry);

    
    
    
    nsTArray<TransactionAndDistance> shTransactions;

    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t startIndex = std::max(0, shist->mIndex - gHistoryMaxViewers);
    int32_t endIndex = std::min(shist->mLength - 1,
                              shist->mIndex + gHistoryMaxViewers);
    nsCOMPtr<nsISHTransaction> trans;
    shist->GetTransactionAtIndex(startIndex, getter_AddRefs(trans));
    for (int32_t i = startIndex; trans && i <= endIndex; i++) {
      nsCOMPtr<nsIContentViewer> contentViewer =
        GetContentViewerForTransaction(trans);

      if (contentViewer) {
        
        
        
        
        bool found = false;
        for (uint32_t j = 0; j < shTransactions.Length(); j++) {
          TransactionAndDistance &container = shTransactions[j];
          if (container.mViewer == contentViewer) {
            container.mDistance = std::min(container.mDistance, DeprecatedAbs(i - shist->mIndex));
            found = true;
            break;
          }
        }

        
        
        if (!found) {
          TransactionAndDistance container(trans, DeprecatedAbs(i - shist->mIndex));
          shTransactions.AppendElement(container);
        }
      }

      nsISHTransaction *temp = trans;
      temp->GetNext(getter_AddRefs(trans));
    }

    
    
    transactions.AppendElements(shTransactions);
    listEntry = PR_NEXT_LINK(shist);
  }

  
  
  if ((int32_t)transactions.Length() <= sHistoryMaxTotalViewers) {
    return;
  }

  
  
  
  
  transactions.Sort();

  for (int32_t i = transactions.Length() - 1;
       i >= sHistoryMaxTotalViewers; --i) {

    EvictContentViewerForTransaction(transactions[i].mTransaction);

  }
}

nsresult
nsSHistory::EvictExpiredContentViewerForEntry(nsIBFCacheEntry *aEntry)
{
  int32_t startIndex = std::max(0, mIndex - gHistoryMaxViewers);
  int32_t endIndex = std::min(mLength - 1,
                            mIndex + gHistoryMaxViewers);
  nsCOMPtr<nsISHTransaction> trans;
  GetTransactionAtIndex(startIndex, getter_AddRefs(trans));

  int32_t i;
  for (i = startIndex; trans && i <= endIndex; ++i) {
    nsCOMPtr<nsISHEntry> entry;
    trans->GetSHEntry(getter_AddRefs(entry));

    
    if (entry->HasBFCacheEntry(aEntry)) {
      break;
    }

    nsISHTransaction *temp = trans;
    temp->GetNext(getter_AddRefs(trans));
  }
  if (i > endIndex)
    return NS_OK;
  
  if (i == mIndex) {
    NS_WARNING("How did the current SHEntry expire?");
    return NS_OK;
  }

  EvictContentViewerForTransaction(trans);

  return NS_OK;
}







void
nsSHistory::GloballyEvictAllContentViewers()
{
  int32_t maxViewers = sHistoryMaxTotalViewers;
  sHistoryMaxTotalViewers = 0;
  GloballyEvictContentViewers();
  sHistoryMaxTotalViewers = maxViewers;
}

void GetDynamicChildren(nsISHContainer* aContainer,
                        nsTArray<uint64_t>& aDocshellIDs,
                        bool aOnlyTopLevelDynamic)
{
  int32_t count = 0;
  aContainer->GetChildCount(&count);
  for (int32_t i = 0; i < count; ++i) {
    nsCOMPtr<nsISHEntry> child;
    aContainer->GetChildAt(i, getter_AddRefs(child));
    if (child) {
      bool dynAdded = false;
      child->IsDynamicallyAdded(&dynAdded);
      if (dynAdded) {
        uint64_t docshellID = 0;
        child->GetDocshellID(&docshellID);
        aDocshellIDs.AppendElement(docshellID);
      }
      if (!dynAdded || !aOnlyTopLevelDynamic) {
        nsCOMPtr<nsISHContainer> childAsContainer = do_QueryInterface(child);
        if (childAsContainer) {
          GetDynamicChildren(childAsContainer, aDocshellIDs,
                             aOnlyTopLevelDynamic);
        }
      }
    }
  }
}

bool
RemoveFromSessionHistoryContainer(nsISHContainer* aContainer,
                                  nsTArray<uint64_t>& aDocshellIDs)
{
  nsCOMPtr<nsISHEntry> root = do_QueryInterface(aContainer);
  NS_ENSURE_TRUE(root, false);

  bool didRemove = false;
  int32_t childCount = 0;
  aContainer->GetChildCount(&childCount);
  for (int32_t i = childCount - 1; i >= 0; --i) {
    nsCOMPtr<nsISHEntry> child;
    aContainer->GetChildAt(i, getter_AddRefs(child));
    if (child) {
      uint64_t docshelldID = 0;
      child->GetDocshellID(&docshelldID);
      if (aDocshellIDs.Contains(docshelldID)) {
        didRemove = true;
        aContainer->RemoveChild(child);
      } else {
        nsCOMPtr<nsISHContainer> container = do_QueryInterface(child);
        if (container) {
          bool childRemoved =
            RemoveFromSessionHistoryContainer(container, aDocshellIDs);
          if (childRemoved) {
            didRemove = true;
          }
        }
      }
    }
  }
  return didRemove;
}

bool RemoveChildEntries(nsISHistory* aHistory, int32_t aIndex,
                          nsTArray<uint64_t>& aEntryIDs)
{
  nsCOMPtr<nsISHEntry> rootHE;
  aHistory->GetEntryAtIndex(aIndex, false, getter_AddRefs(rootHE));
  nsCOMPtr<nsISHContainer> root = do_QueryInterface(rootHE);
  return root ? RemoveFromSessionHistoryContainer(root, aEntryIDs) : false;
}

bool IsSameTree(nsISHEntry* aEntry1, nsISHEntry* aEntry2)
{
  if (!aEntry1 && !aEntry2) {
    return true;
  }
  if ((!aEntry1 && aEntry2) || (aEntry1 && !aEntry2)) {
    return false;
  }
  uint32_t id1, id2;
  aEntry1->GetID(&id1);
  aEntry2->GetID(&id2);
  if (id1 != id2) {
    return false;
  }

  nsCOMPtr<nsISHContainer> container1 = do_QueryInterface(aEntry1);
  nsCOMPtr<nsISHContainer> container2 = do_QueryInterface(aEntry2);
  int32_t count1, count2;
  container1->GetChildCount(&count1);
  container2->GetChildCount(&count2);
  
  int32_t count = std::max(count1, count2);
  for (int32_t i = 0; i < count; ++i) {
    nsCOMPtr<nsISHEntry> child1, child2;
    container1->GetChildAt(i, getter_AddRefs(child1));
    container2->GetChildAt(i, getter_AddRefs(child2));
    if (!IsSameTree(child1, child2)) {
      return false;
    }
  }
  
  return true;
}

bool
nsSHistory::RemoveDuplicate(int32_t aIndex, bool aKeepNext)
{
  NS_ASSERTION(aIndex >= 0, "aIndex must be >= 0!");
  NS_ASSERTION(aIndex != 0 || aKeepNext,
               "If we're removing index 0 we must be keeping the next");
  NS_ASSERTION(aIndex != mIndex, "Shouldn't remove mIndex!");
  int32_t compareIndex = aKeepNext ? aIndex + 1 : aIndex - 1;
  nsCOMPtr<nsISHEntry> root1, root2;
  GetEntryAtIndex(aIndex, false, getter_AddRefs(root1));
  GetEntryAtIndex(compareIndex, false, getter_AddRefs(root2));
  if (IsSameTree(root1, root2)) {
    nsCOMPtr<nsISHTransaction> txToRemove, txToKeep, txNext, txPrev;
    GetTransactionAtIndex(aIndex, getter_AddRefs(txToRemove));
    GetTransactionAtIndex(compareIndex, getter_AddRefs(txToKeep));
    NS_ENSURE_TRUE(txToRemove, false);
    NS_ENSURE_TRUE(txToKeep, false);
    txToRemove->GetNext(getter_AddRefs(txNext));
    txToRemove->GetPrev(getter_AddRefs(txPrev));
    txToRemove->SetNext(nullptr);
    txToRemove->SetPrev(nullptr);
    if (aKeepNext) {
      if (txPrev) {
        txPrev->SetNext(txToKeep);
      } else {
        txToKeep->SetPrev(nullptr);
      }
    } else {
      txToKeep->SetNext(txNext);
    }

    if (aIndex == 0 && aKeepNext) {
      NS_ASSERTION(txToRemove == mListRoot,
                   "Transaction at index 0 should be mListRoot!");
      
      mListRoot = txToKeep;
    }
    if (mRootDocShell) {
      static_cast<nsDocShell*>(mRootDocShell)->HistoryTransactionRemoved(aIndex);
    }

    
    if (mIndex > aIndex) {
      mIndex = mIndex - 1;
    }

    
    
    
    
    
    
    

    
    
    
    
    if (mRequestedIndex > aIndex || (mRequestedIndex == aIndex && !aKeepNext)) {
      mRequestedIndex = mRequestedIndex - 1;
    }
    --mLength;
    return true;
  }
  return false;
}

NS_IMETHODIMP_(void)
nsSHistory::RemoveEntries(nsTArray<uint64_t>& aIDs, int32_t aStartIndex)
{
  int32_t index = aStartIndex;
  while(index >= 0 && RemoveChildEntries(this, --index, aIDs));
  int32_t minIndex = index;
  index = aStartIndex;
  while(index >= 0 && RemoveChildEntries(this, index++, aIDs));
  
  
  bool didRemove = false;
  while (index > minIndex) {
    if (index != mIndex) {
      didRemove = RemoveDuplicate(index, index < mIndex) || didRemove;
    }
    --index;
  }
  if (didRemove && mRootDocShell) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(static_cast<nsDocShell*>(mRootDocShell),
                           &nsDocShell::FireDummyOnLocationChange);
    NS_DispatchToCurrentThread(ev);
  }
}

void
nsSHistory::RemoveDynEntries(int32_t aOldIndex, int32_t aNewIndex)
{
  
  
  nsCOMPtr<nsISHEntry> originalSH;
  GetEntryAtIndex(aOldIndex, false, getter_AddRefs(originalSH));
  nsCOMPtr<nsISHContainer> originalContainer = do_QueryInterface(originalSH);
  nsAutoTArray<uint64_t, 16> toBeRemovedEntries;
  if (originalContainer) {
    nsTArray<uint64_t> originalDynDocShellIDs;
    GetDynamicChildren(originalContainer, originalDynDocShellIDs, true);
    if (originalDynDocShellIDs.Length()) {
      nsCOMPtr<nsISHEntry> currentSH;
      GetEntryAtIndex(aNewIndex, false, getter_AddRefs(currentSH));
      nsCOMPtr<nsISHContainer> newContainer = do_QueryInterface(currentSH);
      if (newContainer) {
        nsTArray<uint64_t> newDynDocShellIDs;
        GetDynamicChildren(newContainer, newDynDocShellIDs, false);
        for (uint32_t i = 0; i < originalDynDocShellIDs.Length(); ++i) {
          if (!newDynDocShellIDs.Contains(originalDynDocShellIDs[i])) {
            toBeRemovedEntries.AppendElement(originalDynDocShellIDs[i]);
          }
        }
      }
    }
  }
  if (toBeRemovedEntries.Length()) {
    RemoveEntries(toBeRemovedEntries, aOldIndex);
  }
}

NS_IMETHODIMP
nsSHistory::UpdateIndex()
{
  
  if (mIndex != mRequestedIndex && mRequestedIndex != -1) {
    RemoveDynEntries(mIndex, mRequestedIndex);
    mIndex = mRequestedIndex;
  }

  mRequestedIndex = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::Stop(uint32_t aStopFlags)
{
  
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetDocument(nsIDOMDocument** aDocument)
{
  
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetCurrentURI(nsIURI** aResultURI)
{
  NS_ENSURE_ARG_POINTER(aResultURI);
  nsresult rv;

  nsCOMPtr<nsISHEntry> currentEntry;
  rv = GetEntryAtIndex(mIndex, false, getter_AddRefs(currentEntry));
  if (NS_FAILED(rv) && !currentEntry) return rv;
  rv = currentEntry->GetURI(aResultURI);
  return rv;
}


NS_IMETHODIMP
nsSHistory::GetReferringURI(nsIURI** aURI)
{
  *aURI = nullptr;
  
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::SetSessionHistory(nsISHistory* aSessionHistory)
{
  
  return NS_OK;
}

	
NS_IMETHODIMP
nsSHistory::GetSessionHistory(nsISHistory** aSessionHistory)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::LoadURIWithOptions(const char16_t* aURI,
                               uint32_t aLoadFlags,
                               nsIURI* aReferringURI,
                               uint32_t aReferrerPolicy,
                               nsIInputStream* aPostStream,
                               nsIInputStream* aExtraHeaderStream,
                               nsIURI* aBaseURI)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::LoadURI(const char16_t* aURI,
                    uint32_t aLoadFlags,
                    nsIURI* aReferringURI,
                    nsIInputStream* aPostStream,
                    nsIInputStream* aExtraHeaderStream)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GotoIndex(int32_t aIndex)
{
  return LoadEntry(aIndex, nsIDocShellLoadInfo::loadHistory, HIST_CMD_GOTOINDEX);
}

nsresult
nsSHistory::LoadNextPossibleEntry(int32_t aNewIndex, long aLoadType, uint32_t aHistCmd)
{
  mRequestedIndex = -1;
  if (aNewIndex < mIndex) {
    return LoadEntry(aNewIndex - 1, aLoadType, aHistCmd);
  }
  if (aNewIndex > mIndex) {
    return LoadEntry(aNewIndex + 1, aLoadType, aHistCmd);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSHistory::LoadEntry(int32_t aIndex, long aLoadType, uint32_t aHistCmd)
{
  nsCOMPtr<nsIDocShell> docShell;
  
  mRequestedIndex = aIndex;

  nsCOMPtr<nsISHEntry> prevEntry;
  GetEntryAtIndex(mIndex, false, getter_AddRefs(prevEntry));

  nsCOMPtr<nsISHEntry> nextEntry;
  GetEntryAtIndex(mRequestedIndex, false, getter_AddRefs(nextEntry));
  if (!nextEntry || !prevEntry) {
    mRequestedIndex = -1;
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsISHEntryInternal> entryInternal = do_QueryInterface(nextEntry);

  if (entryInternal) {
    entryInternal->SetLastTouched(++gTouchCounter);
  }

  
  bool canNavigate = true;
  
  nsCOMPtr<nsIURI> nextURI;
  nextEntry->GetURI(getter_AddRefs(nextURI));

  if (aHistCmd == HIST_CMD_BACK) {
    
    NOTIFY_LISTENERS_CANCELABLE(OnHistoryGoBack, canNavigate,
                                (nextURI, &canNavigate));
  } else if (aHistCmd == HIST_CMD_FORWARD) {
    
    NOTIFY_LISTENERS_CANCELABLE(OnHistoryGoForward, canNavigate,
                                (nextURI, &canNavigate));
  } else if (aHistCmd == HIST_CMD_GOTOINDEX) {
    
    NOTIFY_LISTENERS_CANCELABLE(OnHistoryGotoIndex, canNavigate,
                                (aIndex, nextURI, &canNavigate));
  }

  if (!canNavigate) {
    
    
    mRequestedIndex = -1;
    return NS_OK;  
  }

  nsCOMPtr<nsIURI> nexturi;
  int32_t pCount=0, nCount=0;
  nsCOMPtr<nsISHContainer> prevAsContainer(do_QueryInterface(prevEntry));
  nsCOMPtr<nsISHContainer> nextAsContainer(do_QueryInterface(nextEntry));
  if (prevAsContainer && nextAsContainer) {
    prevAsContainer->GetChildCount(&pCount);
    nextAsContainer->GetChildCount(&nCount);
  }
  
  nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
  if (mRequestedIndex == mIndex) {
    
    docShell = mRootDocShell;
  }
  else {
    
    if ((pCount > 0) && (nCount > 0)) {
      


      bool frameFound = false;
      nsresult rv = CompareFrames(prevEntry, nextEntry, mRootDocShell, aLoadType, &frameFound);
      if (!frameFound) {
        
        
        return LoadNextPossibleEntry(aIndex, aLoadType, aHistCmd);
      }
      return rv;
    }   
    else {
      
      uint32_t prevID = 0;
      uint32_t nextID = 0;
      prevEntry->GetID(&prevID);
      nextEntry->GetID(&nextID);
      if (prevID == nextID) {
        
        
        return LoadNextPossibleEntry(aIndex, aLoadType, aHistCmd);
      }
      docShell = mRootDocShell;
    }
  }

  if (!docShell) {
    
    
    mRequestedIndex = -1;
    return NS_ERROR_FAILURE;
  }

  
  return InitiateLoad(nextEntry, docShell, aLoadType);
}

nsresult
nsSHistory::CompareFrames(nsISHEntry * aPrevEntry, nsISHEntry * aNextEntry, nsIDocShell * aParent, long aLoadType, bool * aIsFrameFound)
{
  if (!aPrevEntry || !aNextEntry || !aParent)
    return NS_ERROR_FAILURE;

  
  
  
  uint64_t prevdID, nextdID;
  aPrevEntry->GetDocshellID(&prevdID);
  aNextEntry->GetDocshellID(&nextdID);
  NS_ENSURE_STATE(prevdID == nextdID);

  nsresult result = NS_OK;
  uint32_t prevID, nextID;

  aPrevEntry->GetID(&prevID);
  aNextEntry->GetID(&nextID);
 
  
  if (prevID != nextID) {
    if (aIsFrameFound)
      *aIsFrameFound = true;
    
    
    aNextEntry->SetIsSubFrame(true);
    InitiateLoad(aNextEntry, aParent, aLoadType);
    return NS_OK;
  }

  
  int32_t pcnt=0, ncnt=0, dsCount=0;
  nsCOMPtr<nsISHContainer>  prevContainer(do_QueryInterface(aPrevEntry));
  nsCOMPtr<nsISHContainer>  nextContainer(do_QueryInterface(aNextEntry));

  if (!aParent)
    return NS_ERROR_FAILURE;
  if (!prevContainer || !nextContainer)
    return NS_ERROR_FAILURE;

  prevContainer->GetChildCount(&pcnt);
  nextContainer->GetChildCount(&ncnt);
  aParent->GetChildCount(&dsCount);

  
  nsCOMArray<nsIDocShell> docshells;
  for (int32_t i = 0; i < dsCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> treeItem;
    aParent->GetChildAt(i, getter_AddRefs(treeItem));
    nsCOMPtr<nsIDocShell> shell = do_QueryInterface(treeItem);
    if (shell) {
      docshells.AppendObject(shell);
    }
  }

  
  for (int32_t i = 0; i < ncnt; ++i) {
    
    nsCOMPtr<nsISHEntry> nChild;
    nextContainer->GetChildAt(i, getter_AddRefs(nChild));
    if (!nChild) {
      continue;
    }
    uint64_t docshellID = 0;
    nChild->GetDocshellID(&docshellID);

    
    nsIDocShell* dsChild = nullptr;
    int32_t count = docshells.Count();
    for (int32_t j = 0; j < count; ++j) {
      uint64_t shellID = 0;
      nsIDocShell* shell = docshells[j];
      shell->GetHistoryID(&shellID);
      if (shellID == docshellID) {
        dsChild = shell;
        break;
      }
    }
    if (!dsChild) {
      continue;
    }

    
    
    nsCOMPtr<nsISHEntry> pChild;
    for (int32_t k = 0; k < pcnt; ++k) {
      nsCOMPtr<nsISHEntry> child;
      prevContainer->GetChildAt(k, getter_AddRefs(child));
      if (child) {
        uint64_t dID = 0;
        child->GetDocshellID(&dID);
        if (dID == docshellID) {
          pChild = child;
          break;
        }
      }
    }

    
    
    
    CompareFrames(pChild, nChild, dsChild, aLoadType, aIsFrameFound);
  }     
  return result;
}


nsresult 
nsSHistory::InitiateLoad(nsISHEntry * aFrameEntry, nsIDocShell * aFrameDS, long aLoadType)
{
  NS_ENSURE_STATE(aFrameDS && aFrameEntry);

  nsCOMPtr<nsIDocShellLoadInfo> loadInfo;

  



  aFrameEntry->SetLoadType(aLoadType);    
  aFrameDS->CreateLoadInfo (getter_AddRefs(loadInfo));

  loadInfo->SetLoadType(aLoadType);
  loadInfo->SetSHEntry(aFrameEntry);

  nsCOMPtr<nsIURI> nextURI;
  aFrameEntry->GetURI(getter_AddRefs(nextURI));
  
  return aFrameDS->LoadURI(nextURI, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE, false);

}

NS_IMETHODIMP
nsSHistory::SetRootDocShell(nsIDocShell * aDocShell)
{
  mRootDocShell = aDocShell;
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetSHistoryEnumerator(nsISimpleEnumerator** aEnumerator)
{
  NS_ENSURE_ARG_POINTER(aEnumerator);
  nsRefPtr<nsSHEnumerator> iterator = new nsSHEnumerator(this);
  iterator.forget(aEnumerator);
  return NS_OK;
}






nsSHEnumerator::nsSHEnumerator(nsSHistory * aSHistory):mIndex(-1)
{
  mSHistory = aSHistory;
}

nsSHEnumerator::~nsSHEnumerator()
{
  mSHistory = nullptr;
}

NS_IMPL_ISUPPORTS(nsSHEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsSHEnumerator::HasMoreElements(bool * aReturn)
{
  int32_t cnt;
  *aReturn = false;
  mSHistory->GetCount(&cnt);
  if (mIndex >= -1 && mIndex < (cnt-1) ) { 
    *aReturn = true;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsSHEnumerator::GetNext(nsISupports **aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  int32_t cnt= 0;

  nsresult  result = NS_ERROR_FAILURE;
  mSHistory->GetCount(&cnt);
  if (mIndex < (cnt-1)) {
    mIndex++;
    nsCOMPtr<nsISHEntry> hEntry;
    result = mSHistory->GetEntryAtIndex(mIndex, false, getter_AddRefs(hEntry));
    if (hEntry)
      result = CallQueryInterface(hEntry, aItem);
  }
  return result;
}
