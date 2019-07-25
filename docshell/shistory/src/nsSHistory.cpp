







































#include "nsSHistory.h"


#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "mozilla/Preferences.h"


#include "nsILayoutHistoryState.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsISHContainer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIContentViewer.h"
#include "nsICacheService.h"
#include "nsIObserverService.h"
#include "prclist.h"
#include "mozilla/Services.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsDocShell.h"


#include "nspr.h"
#include <math.h>  

using namespace mozilla;

#define PREF_SHISTORY_SIZE "browser.sessionhistory.max_entries"
#define PREF_SHISTORY_MAX_TOTAL_VIEWERS "browser.sessionhistory.max_total_viewers"

static const char* kObservedPrefs[] = {
  PREF_SHISTORY_SIZE,
  PREF_SHISTORY_MAX_TOTAL_VIEWERS,
  nsnull
};

static PRInt32  gHistoryMaxSize = 50;

static const PRInt32  gHistoryMaxViewers = 3;

static PRCList gSHistoryList;


PRInt32 nsSHistory::sHistoryMaxTotalViewers = -1;



static PRUint32 gTouchCounter = 0;

static PRLogModuleInfo* gLogModule = PR_LOG_DEFINE("nsSHistory");
#define LOG(format) PR_LOG(gLogModule, PR_LOG_DEBUG, format)







#define LOG_SPEC(format, uri)                              \
  PR_BEGIN_MACRO                                           \
    if (PR_LOG_TEST(gLogModule, PR_LOG_DEBUG)) {           \
      nsCAutoString _specStr(NS_LITERAL_CSTRING("(null)"));\
      if (uri) {                                           \
        uri->GetSpec(_specStr);                            \
      }                                                    \
      const char* _spec = _specStr.get();                  \
      LOG(format);                                         \
    }                                                      \
  PR_END_MACRO







#define LOG_SHENTRY_SPEC(format, shentry)                  \
  PR_BEGIN_MACRO                                           \
    if (PR_LOG_TEST(gLogModule, PR_LOG_DEBUG)) {           \
      nsCOMPtr<nsIURI> uri;                                \
      shentry->GetURI(getter_AddRefs(uri));                \
      LOG_SPEC(format, uri);                               \
    }                                                      \
  PR_END_MACRO

enum HistCmd{
  HIST_CMD_BACK,
  HIST_CMD_FORWARD,
  HIST_CMD_GOTOINDEX,
  HIST_CMD_RELOAD
} ;





class nsSHistoryObserver : public nsIObserver
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsSHistoryObserver() {}

protected:
  ~nsSHistoryObserver() {}
};

static nsSHistoryObserver* gObserver = nsnull;

NS_IMPL_ISUPPORTS1(nsSHistoryObserver, nsIObserver)

NS_IMETHODIMP
nsSHistoryObserver::Observe(nsISupports *aSubject, const char *aTopic,
                            const PRUnichar *aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsSHistory::UpdatePrefs();
    nsSHistory::GloballyEvictContentViewers();
  } else if (!strcmp(aTopic, NS_CACHESERVICE_EMPTYCACHE_TOPIC_ID) ||
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
    return nsnull;
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

    
    
    ownerEntry->SetContentViewer(nsnull);
    ownerEntry->SyncPresentationState();
    viewer->Destroy();
  }
}

} 





nsSHistory::nsSHistory() : mListRoot(nsnull), mIndex(-1), mLength(0), mRequestedIndex(-1)
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






PRUint32
nsSHistory::CalcMaxTotalViewers()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRUint64 bytes = PR_GetPhysicalMemorySize();

  if (LL_IS_ZERO(bytes))
    return 0;

  
  
  
  if (LL_CMP(bytes, >, LL_MAXINT))
    bytes = LL_MAXINT;

  PRUint64 kbytes;
  LL_SHR(kbytes, bytes, 10);

  double kBytesD;
  LL_L2D(kBytesD, (PRInt64) kbytes);

  
  
  
  PRUint32 viewers = 0;
  double x = log(kBytesD)/log(2.0) - 14;
  if (x > 0) {
    viewers    = (PRUint32)(x * x - x + 2.001); 
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

  
  
  PRInt32 defaultHistoryMaxSize =
    Preferences::GetDefaultInt(PREF_SHISTORY_SIZE, 50);
  if (gHistoryMaxSize < defaultHistoryMaxSize) {
    gHistoryMaxSize = defaultHistoryMaxSize;
  }
  
  
  
  if (!gObserver) {
    gObserver = new nsSHistoryObserver();
    NS_ADDREF(gObserver);
    Preferences::AddStrongObservers(gObserver, kObservedPrefs);

    nsCOMPtr<nsIObserverService> obsSvc =
      mozilla::services::GetObserverService();
    if (obsSvc) {
      
      
      obsSvc->AddObserver(gObserver,
                          NS_CACHESERVICE_EMPTYCACHE_TOPIC_ID, PR_FALSE);

      
      obsSvc->AddObserver(gObserver, "memory-pressure", PR_FALSE);
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
      obsSvc->RemoveObserver(gObserver, NS_CACHESERVICE_EMPTYCACHE_TOPIC_ID);
      obsSvc->RemoveObserver(gObserver, "memory-pressure");
    }
    NS_RELEASE(gObserver);
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

  if(!currentPersist)
  {
    NS_ENSURE_SUCCESS(currentTxn->SetSHEntry(aSHEntry),NS_ERROR_FAILURE);
    currentTxn->SetPersist(aPersist);
    return NS_OK;
  }

  nsCOMPtr<nsISHTransaction> txn(do_CreateInstance(NS_SHTRANSACTION_CONTRACTID));
  NS_ENSURE_TRUE(txn, NS_ERROR_FAILURE);

  
  if (mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryReferent(mListener));
    if (listener) {
      nsCOMPtr<nsIURI> uri;
      nsCOMPtr<nsIHistoryEntry> hEntry(do_QueryInterface(aSHEntry));
      if (hEntry) {
        PRInt32 currentIndex = mIndex;
        hEntry->GetURI(getter_AddRefs(uri));
        listener->OnHistoryNewEntry(uri);

        
        
        if (currentIndex != mIndex)
          GetTransactionAtIndex(mIndex, getter_AddRefs(currentTxn));
      }
    }
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
nsSHistory::GetCount(PRInt32 * aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = mLength;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetIndex(PRInt32 * aResult)
{
  NS_PRECONDITION(aResult, "null out param?");
  *aResult = mIndex;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetRequestedIndex(PRInt32 * aResult)
{
  NS_PRECONDITION(aResult, "null out param?");
  *aResult = mRequestedIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetEntryAtIndex(PRInt32 aIndex, bool aModifyIndex, nsISHEntry** aResult)
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
nsSHistory::GetEntryAtIndex(PRInt32 aIndex, bool aModifyIndex, nsIHistoryEntry** aResult)
{
  nsresult rv;
  nsCOMPtr<nsISHEntry> shEntry;
  rv = GetEntryAtIndex(aIndex, aModifyIndex, getter_AddRefs(shEntry));
  if (NS_SUCCEEDED(rv) && shEntry) 
    rv = CallQueryInterface(shEntry, aResult);
 
  return rv;
}


NS_IMETHODIMP
nsSHistory::GetTransactionAtIndex(PRInt32 aIndex, nsISHTransaction ** aResult)
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
  PRInt32   cnt=0;
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
        *aResult = ptr;
        NS_ADDREF(*aResult);
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

#ifdef DEBUG
nsresult
nsSHistory::PrintHistory()
{

  nsCOMPtr<nsISHTransaction>   txn;
  PRInt32 index = 0;
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
    nsCOMPtr<nsIHistoryEntry> hEntry(do_QueryInterface(entry));
    if (hEntry) {
      hEntry->GetURI(getter_AddRefs(uri));
      hEntry->GetTitle(getter_Copies(title));              
    }

#if 0
    nsCAutoString url;
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
nsSHistory::GetMaxLength(PRInt32 * aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = gHistoryMaxSize;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::SetMaxLength(PRInt32 aMaxSize)
{
  if (aMaxSize < 0)
    return NS_ERROR_ILLEGAL_VALUE;

  gHistoryMaxSize = aMaxSize;
  if (mLength > aMaxSize)
    PurgeHistory(mLength-aMaxSize);
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::PurgeHistory(PRInt32 aEntries)
{
  if (mLength <= 0 || aEntries <= 0)
    return NS_ERROR_FAILURE;

  aEntries = NS_MIN(aEntries, mLength);
  
  bool purgeHistory = true;
  
  if (mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryReferent(mListener));
    if (listener) {
      listener->OnHistoryPurge(aEntries, &purgeHistory);
    } 
  }

  if (!purgeHistory) {
    
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }

  PRInt32 cnt = 0;
  while (cnt < aEntries) {
    nsCOMPtr<nsISHTransaction> nextTxn;
    if (mListRoot) {
      mListRoot->GetNext(getter_AddRefs(nextTxn));
      mListRoot->SetNext(nsnull);
    }
    mListRoot = nextTxn;
    if (mListRoot) {
      mListRoot->SetPrev(nsnull);
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
  mListener = listener;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::RemoveSHistoryListener(nsISHistoryListener * aListener)
{
  
  
  nsWeakPtr listener = do_GetWeakReference(aListener);  
  if (listener == mListener) {
    mListener = nsnull;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}





NS_IMETHODIMP
nsSHistory::ReplaceEntry(PRInt32 aIndex, nsISHEntry * aReplaceEntry)
{
  NS_ENSURE_ARG(aReplaceEntry);
  nsresult rv;
  nsCOMPtr<nsISHTransaction> currentTxn;

  if (!mListRoot) 
    return NS_ERROR_FAILURE;

  rv = GetTransactionAtIndex(aIndex, getter_AddRefs(currentTxn));

  if(currentTxn)
  {
    
    rv = currentTxn->SetSHEntry(aReplaceEntry);
    rv = currentTxn->SetPersist(PR_TRUE);
  }
  return rv;
}


NS_IMETHODIMP
nsSHistory::GetListener(nsISHistoryListener ** aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  if (mListener) 
    CallQueryReferent(mListener.get(),  aListener);
  
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::EvictOutOfRangeContentViewers(PRInt32 aIndex)
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
  *aCanGoBack = PR_FALSE;

  
  PRInt32 index = -1;
  NS_ENSURE_SUCCESS(GetRequestedIndex(&index), NS_ERROR_FAILURE);

  if(index != -1)
    return NS_OK;

  NS_ENSURE_SUCCESS(GetIndex(&index), NS_ERROR_FAILURE);
  if(index > 0)
     *aCanGoBack = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetCanGoForward(bool * aCanGoForward)
{
  NS_ENSURE_ARG_POINTER(aCanGoForward);
  *aCanGoForward = PR_FALSE;

  
  PRInt32 index = -1;
  PRInt32 count = -1;

  NS_ENSURE_SUCCESS(GetRequestedIndex(&index), NS_ERROR_FAILURE);

  if(index != -1)
    return NS_OK;

  NS_ENSURE_SUCCESS(GetIndex(&index), NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(GetCount(&count), NS_ERROR_FAILURE);

  if((index >= 0) && (index < (count - 1)))
    *aCanGoForward = PR_TRUE;

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
nsSHistory::Reload(PRUint32 aReloadFlags)
{
  nsresult rv;
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
  else
  {
    loadType = nsIDocShellLoadInfo::loadReloadNormal;
  }
  
  
  bool canNavigate = true;
  if (mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryReferent(mListener));
    
    
    
    
    if (listener) {
      nsCOMPtr<nsIURI> currentURI;
      rv = GetCurrentURI(getter_AddRefs(currentURI));
      listener->OnHistoryReload(currentURI, aReloadFlags, &canNavigate);
    }
  }
  if (!canNavigate)
    return NS_OK;

  return LoadEntry(mIndex, loadType, HIST_CMD_RELOAD);
}

NS_IMETHODIMP
nsSHistory::ReloadCurrentEntry()
{
  
  bool canNavigate = true;
  if (mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryReferent(mListener));
    if (listener) {
      nsCOMPtr<nsIURI> currentURI;
      GetCurrentURI(getter_AddRefs(currentURI));
      listener->OnHistoryGotoIndex(mIndex, currentURI, &canNavigate);
    }
  }
  if (!canNavigate)
    return NS_OK;

  return LoadEntry(mIndex, nsIDocShellLoadInfo::loadHistory, HIST_CMD_RELOAD);
}

void
nsSHistory::EvictOutOfRangeWindowContentViewers(PRInt32 aIndex)
{
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (aIndex < 0) {
    return;
  }
  NS_ASSERTION(aIndex < mLength, "aIndex is out of range");
  if (aIndex >= mLength) {
    return;
  }

  
  PRInt32 startSafeIndex = PR_MAX(0, aIndex - gHistoryMaxViewers);
  PRInt32 endSafeIndex = PR_MIN(mLength, aIndex + gHistoryMaxViewers);

  LOG(("EvictOutOfRangeWindowContentViewers(index=%d), "
       "mLength=%d. Safe range [%d, %d]",
       aIndex, mLength, startSafeIndex, endSafeIndex)); 

  
  
  
  nsCOMArray<nsIContentViewer> safeViewers;
  nsCOMPtr<nsISHTransaction> trans;
  GetTransactionAtIndex(startSafeIndex, getter_AddRefs(trans));
  for (PRUint32 i = startSafeIndex; trans && i <= endSafeIndex; i++) {
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
  TransactionAndDistance(nsISHTransaction *aTrans, PRUint32 aDist)
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
  PRUint32 mLastTouched;
  PRInt32 mDistance;
};

} 


void
nsSHistory::GloballyEvictContentViewers()
{
  
  
  

  nsTArray<TransactionAndDistance> transactions;

  nsSHistory *shist = static_cast<nsSHistory*>(PR_LIST_HEAD(&gSHistoryList));
  while (shist != &gSHistoryList) {

    
    
    
    nsTArray<TransactionAndDistance> shTransactions;

    
    
    
    
    
    
    
    
    
    
    
    
    
    PRInt32 startIndex = NS_MAX(0, shist->mIndex - gHistoryMaxViewers);
    PRInt32 endIndex = NS_MIN(shist->mLength - 1,
                              shist->mIndex + gHistoryMaxViewers);
    nsCOMPtr<nsISHTransaction> trans;
    shist->GetTransactionAtIndex(startIndex, getter_AddRefs(trans));
    for (PRInt32 i = startIndex; trans && i <= endIndex; i++) {
      nsCOMPtr<nsIContentViewer> contentViewer =
        GetContentViewerForTransaction(trans);

      if (contentViewer) {
        
        
        
        
        bool found = false;
        for (PRUint32 j = 0; j < shTransactions.Length(); j++) {
          TransactionAndDistance &container = shTransactions[j];
          if (container.mViewer == contentViewer) {
            container.mDistance = PR_MIN(container.mDistance,
                                         PR_ABS(i - shist->mIndex));
            found = PR_TRUE;
            break;
          }
        }

        
        
        if (!found) {
          TransactionAndDistance container(trans, PR_ABS(i - shist->mIndex));
          shTransactions.AppendElement(container);
        }
      }

      nsISHTransaction *temp = trans;
      temp->GetNext(getter_AddRefs(trans));
    }

    
    
    transactions.AppendElements(shTransactions);
    shist = static_cast<nsSHistory*>(PR_NEXT_LINK(shist));
  }

  
  
  if ((PRInt32)transactions.Length() <= sHistoryMaxTotalViewers) {
    return;
  }

  
  
  
  
  transactions.Sort();

  for (PRInt32 i = transactions.Length() - 1;
       i >= sHistoryMaxTotalViewers; --i) {

    EvictContentViewerForTransaction(transactions[i].mTransaction);

  }
}

nsresult
nsSHistory::EvictExpiredContentViewerForEntry(nsIBFCacheEntry *aEntry)
{
  PRInt32 startIndex = NS_MAX(0, mIndex - gHistoryMaxViewers);
  PRInt32 endIndex = NS_MIN(mLength - 1,
                            mIndex + gHistoryMaxViewers);
  nsCOMPtr<nsISHTransaction> trans;
  GetTransactionAtIndex(startIndex, getter_AddRefs(trans));

  PRInt32 i;
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
  PRInt32 maxViewers = sHistoryMaxTotalViewers;
  sHistoryMaxTotalViewers = 0;
  GloballyEvictContentViewers();
  sHistoryMaxTotalViewers = maxViewers;
}

void GetDynamicChildren(nsISHContainer* aContainer,
                        nsTArray<PRUint64>& aDocshellIDs,
                        bool aOnlyTopLevelDynamic)
{
  PRInt32 count = 0;
  aContainer->GetChildCount(&count);
  for (PRInt32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISHEntry> child;
    aContainer->GetChildAt(i, getter_AddRefs(child));
    if (child) {
      bool dynAdded = false;
      child->IsDynamicallyAdded(&dynAdded);
      if (dynAdded) {
        PRUint64 docshellID = 0;
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
                                  nsTArray<PRUint64>& aDocshellIDs)
{
  nsCOMPtr<nsISHEntry> root = do_QueryInterface(aContainer);
  NS_ENSURE_TRUE(root, PR_FALSE);

  bool didRemove = false;
  PRInt32 childCount = 0;
  aContainer->GetChildCount(&childCount);
  for (PRInt32 i = childCount - 1; i >= 0; --i) {
    nsCOMPtr<nsISHEntry> child;
    aContainer->GetChildAt(i, getter_AddRefs(child));
    if (child) {
      PRUint64 docshelldID = 0;
      child->GetDocshellID(&docshelldID);
      if (aDocshellIDs.Contains(docshelldID)) {
        didRemove = PR_TRUE;
        aContainer->RemoveChild(child);
      } else {
        nsCOMPtr<nsISHContainer> container = do_QueryInterface(child);
        if (container) {
          bool childRemoved =
            RemoveFromSessionHistoryContainer(container, aDocshellIDs);
          if (childRemoved) {
            didRemove = PR_TRUE;
          }
        }
      }
    }
  }
  return didRemove;
}

bool RemoveChildEntries(nsISHistory* aHistory, PRInt32 aIndex,
                          nsTArray<PRUint64>& aEntryIDs)
{
  nsCOMPtr<nsIHistoryEntry> rootHE;
  aHistory->GetEntryAtIndex(aIndex, PR_FALSE, getter_AddRefs(rootHE));
  nsCOMPtr<nsISHContainer> root = do_QueryInterface(rootHE);
  return root ? RemoveFromSessionHistoryContainer(root, aEntryIDs) : PR_FALSE;
}

bool IsSameTree(nsISHEntry* aEntry1, nsISHEntry* aEntry2)
{
  if (!aEntry1 && !aEntry2) {
    return PR_TRUE;
  }
  if ((!aEntry1 && aEntry2) || (aEntry1 && !aEntry2)) {
    return PR_FALSE;
  }
  PRUint32 id1, id2;
  aEntry1->GetID(&id1);
  aEntry2->GetID(&id2);
  if (id1 != id2) {
    return PR_FALSE;
  }

  nsCOMPtr<nsISHContainer> container1 = do_QueryInterface(aEntry1);
  nsCOMPtr<nsISHContainer> container2 = do_QueryInterface(aEntry2);
  PRInt32 count1, count2;
  container1->GetChildCount(&count1);
  container2->GetChildCount(&count2);
  
  PRInt32 count = NS_MAX(count1, count2);
  for (PRInt32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISHEntry> child1, child2;
    container1->GetChildAt(i, getter_AddRefs(child1));
    container2->GetChildAt(i, getter_AddRefs(child2));
    if (!IsSameTree(child1, child2)) {
      return PR_FALSE;
    }
  }
  
  return PR_TRUE;
}

bool
nsSHistory::RemoveDuplicate(PRInt32 aIndex, bool aKeepNext)
{
  NS_ASSERTION(aIndex >= 0, "aIndex must be >= 0!");
  NS_ASSERTION(aIndex != 0 || aKeepNext,
               "If we're removing index 0 we must be keeping the next");
  NS_ASSERTION(aIndex != mIndex, "Shouldn't remove mIndex!");
  PRInt32 compareIndex = aKeepNext ? aIndex + 1 : aIndex - 1;
  nsCOMPtr<nsIHistoryEntry> rootHE1, rootHE2;
  GetEntryAtIndex(aIndex, PR_FALSE, getter_AddRefs(rootHE1));
  GetEntryAtIndex(compareIndex, PR_FALSE, getter_AddRefs(rootHE2));
  nsCOMPtr<nsISHEntry> root1 = do_QueryInterface(rootHE1);
  nsCOMPtr<nsISHEntry> root2 = do_QueryInterface(rootHE2);
  if (IsSameTree(root1, root2)) {
    nsCOMPtr<nsISHTransaction> txToRemove, txToKeep, txNext, txPrev;
    GetTransactionAtIndex(aIndex, getter_AddRefs(txToRemove));
    GetTransactionAtIndex(compareIndex, getter_AddRefs(txToKeep));
    NS_ENSURE_TRUE(txToRemove, PR_FALSE);
    NS_ENSURE_TRUE(txToKeep, PR_FALSE);
    txToRemove->GetNext(getter_AddRefs(txNext));
    txToRemove->GetPrev(getter_AddRefs(txPrev));
    txToRemove->SetNext(nsnull);
    txToRemove->SetPrev(nsnull);
    if (aKeepNext) {
      if (txPrev) {
        txPrev->SetNext(txToKeep);
      } else {
        txToKeep->SetPrev(nsnull);
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
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP_(void)
nsSHistory::RemoveEntries(nsTArray<PRUint64>& aIDs, PRInt32 aStartIndex)
{
  PRInt32 index = aStartIndex;
  while(index >= 0 && RemoveChildEntries(this, --index, aIDs));
  PRInt32 minIndex = index;
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
    nsRefPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(static_cast<nsDocShell*>(mRootDocShell),
                           &nsDocShell::FireDummyOnLocationChange);
    NS_DispatchToCurrentThread(ev);
  }
}

void
nsSHistory::RemoveDynEntries(PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  
  
  nsCOMPtr<nsISHEntry> originalSH;
  GetEntryAtIndex(aOldIndex, PR_FALSE, getter_AddRefs(originalSH));
  nsCOMPtr<nsISHContainer> originalContainer = do_QueryInterface(originalSH);
  nsAutoTArray<PRUint64, 16> toBeRemovedEntries;
  if (originalContainer) {
    nsTArray<PRUint64> originalDynDocShellIDs;
    GetDynamicChildren(originalContainer, originalDynDocShellIDs, PR_TRUE);
    if (originalDynDocShellIDs.Length()) {
      nsCOMPtr<nsISHEntry> currentSH;
      GetEntryAtIndex(aNewIndex, PR_FALSE, getter_AddRefs(currentSH));
      nsCOMPtr<nsISHContainer> newContainer = do_QueryInterface(currentSH);
      if (newContainer) {
        nsTArray<PRUint64> newDynDocShellIDs;
        GetDynamicChildren(newContainer, newDynDocShellIDs, PR_FALSE);
        for (PRUint32 i = 0; i < originalDynDocShellIDs.Length(); ++i) {
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
nsSHistory::Stop(PRUint32 aStopFlags)
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

  nsCOMPtr<nsIHistoryEntry> currentEntry;
  rv = GetEntryAtIndex(mIndex, PR_FALSE, getter_AddRefs(currentEntry));
  if (NS_FAILED(rv) && !currentEntry) return rv;
  rv = currentEntry->GetURI(aResultURI);
  return rv;
}


NS_IMETHODIMP
nsSHistory::GetReferringURI(nsIURI** aURI)
{
  *aURI = nsnull;
  
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
nsSHistory::LoadURI(const PRUnichar* aURI,
                    PRUint32 aLoadFlags,
                    nsIURI* aReferringURI,
                    nsIInputStream* aPostStream,
                    nsIInputStream* aExtraHeaderStream)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GotoIndex(PRInt32 aIndex)
{
  return LoadEntry(aIndex, nsIDocShellLoadInfo::loadHistory, HIST_CMD_GOTOINDEX);
}

nsresult
nsSHistory::LoadNextPossibleEntry(PRInt32 aNewIndex, long aLoadType, PRUint32 aHistCmd)
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
nsSHistory::LoadEntry(PRInt32 aIndex, long aLoadType, PRUint32 aHistCmd)
{
  nsCOMPtr<nsIDocShell> docShell;
  
  mRequestedIndex = aIndex;

  nsCOMPtr<nsISHEntry> prevEntry;
  GetEntryAtIndex(mIndex, PR_FALSE, getter_AddRefs(prevEntry));

  nsCOMPtr<nsISHEntry> nextEntry;
  GetEntryAtIndex(mRequestedIndex, PR_FALSE, getter_AddRefs(nextEntry));
  nsCOMPtr<nsIHistoryEntry> nHEntry(do_QueryInterface(nextEntry));
  if (!nextEntry || !prevEntry || !nHEntry) {
    mRequestedIndex = -1;
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsISHEntryInternal> entryInternal = do_QueryInterface(nextEntry);

  if (entryInternal) {
    entryInternal->SetLastTouched(++gTouchCounter);
  }

  
  bool canNavigate = true;
  
  nsCOMPtr<nsIURI> nextURI;
  nHEntry->GetURI(getter_AddRefs(nextURI));

  if(mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryReferent(mListener));
    if (listener) {
      if (aHistCmd == HIST_CMD_BACK) {
        
        listener->OnHistoryGoBack(nextURI, &canNavigate);
      }
      else if (aHistCmd == HIST_CMD_FORWARD) {
        
        listener->OnHistoryGoForward(nextURI, &canNavigate);
      }
      else if (aHistCmd == HIST_CMD_GOTOINDEX) {
        
        listener->OnHistoryGotoIndex(aIndex, nextURI, &canNavigate);
      }
    }
  }

  if (!canNavigate) {
    
    
    mRequestedIndex = -1;
    return NS_OK;  
  }

  nsCOMPtr<nsIURI> nexturi;
  PRInt32 pCount=0, nCount=0;
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
      
      PRUint32 prevID = 0;
      PRUint32 nextID = 0;
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

  
  
  
  PRUint64 prevdID, nextdID;
  aPrevEntry->GetDocshellID(&prevdID);
  aNextEntry->GetDocshellID(&nextdID);
  NS_ENSURE_STATE(prevdID == nextdID);

  nsresult result = NS_OK;
  PRUint32 prevID, nextID;

  aPrevEntry->GetID(&prevID);
  aNextEntry->GetID(&nextID);
 
  
  if (prevID != nextID) {
    if (aIsFrameFound)
      *aIsFrameFound = PR_TRUE;
    
    
    aNextEntry->SetIsSubFrame(PR_TRUE);
    InitiateLoad(aNextEntry, aParent, aLoadType);
    return NS_OK;
  }

  
  PRInt32 pcnt=0, ncnt=0, dsCount=0;
  nsCOMPtr<nsISHContainer>  prevContainer(do_QueryInterface(aPrevEntry));
  nsCOMPtr<nsISHContainer>  nextContainer(do_QueryInterface(aNextEntry));
  nsCOMPtr<nsIDocShellTreeNode> dsTreeNode(do_QueryInterface(aParent));

  if (!dsTreeNode)
    return NS_ERROR_FAILURE;
  if (!prevContainer || !nextContainer)
    return NS_ERROR_FAILURE;

  prevContainer->GetChildCount(&pcnt);
  nextContainer->GetChildCount(&ncnt);
  dsTreeNode->GetChildCount(&dsCount);

  
  nsCOMArray<nsIDocShell> docshells;
  for (PRInt32 i = 0; i < dsCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> treeItem;
    dsTreeNode->GetChildAt(i, getter_AddRefs(treeItem));
    nsCOMPtr<nsIDocShell> shell = do_QueryInterface(treeItem);
    if (shell) {
      docshells.AppendObject(shell);
    }
  }

  
  for (PRInt32 i = 0; i < ncnt; ++i) {
    
    nsCOMPtr<nsISHEntry> nChild;
    nextContainer->GetChildAt(i, getter_AddRefs(nChild));
    if (!nChild) {
      continue;
    }
    PRUint64 docshellID = 0;
    nChild->GetDocshellID(&docshellID);

    
    nsIDocShell* dsChild = nsnull;
    PRInt32 count = docshells.Count();
    for (PRInt32 j = 0; j < count; ++j) {
      PRUint64 shellID = 0;
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
    for (PRInt32 k = 0; k < pcnt; ++k) {
      nsCOMPtr<nsISHEntry> child;
      prevContainer->GetChildAt(k, getter_AddRefs(child));
      if (child) {
        PRUint64 dID = 0;
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
  nsCOMPtr<nsIHistoryEntry> hEntry(do_QueryInterface(aFrameEntry));
  hEntry->GetURI(getter_AddRefs(nextURI));
  
  return aFrameDS->LoadURI(nextURI, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE, PR_FALSE);

}



NS_IMETHODIMP
nsSHistory::SetRootDocShell(nsIDocShell * aDocShell)
{
  mRootDocShell = aDocShell;
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetRootDocShell(nsIDocShell ** aDocShell)
{
  NS_ENSURE_ARG_POINTER(aDocShell);

  *aDocShell = mRootDocShell;
  
  
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetSHistoryEnumerator(nsISimpleEnumerator** aEnumerator)
{
  nsresult status = NS_OK;

  NS_ENSURE_ARG_POINTER(aEnumerator);
  nsSHEnumerator * iterator = new nsSHEnumerator(this);
  if (iterator && NS_FAILED(status = CallQueryInterface(iterator, aEnumerator)))
    delete iterator;
  return status;
}






nsSHEnumerator::nsSHEnumerator(nsSHistory * aSHistory):mIndex(-1)
{
  mSHistory = aSHistory;
}

nsSHEnumerator::~nsSHEnumerator()
{
  mSHistory = nsnull;
}

NS_IMPL_ISUPPORTS1(nsSHEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsSHEnumerator::HasMoreElements(bool * aReturn)
{
  PRInt32 cnt;
  *aReturn = PR_FALSE;
  mSHistory->GetCount(&cnt);
  if (mIndex >= -1 && mIndex < (cnt-1) ) { 
    *aReturn = PR_TRUE;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsSHEnumerator::GetNext(nsISupports **aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  PRInt32 cnt= 0;

  nsresult  result = NS_ERROR_FAILURE;
  mSHistory->GetCount(&cnt);
  if (mIndex < (cnt-1)) {
    mIndex++;
    nsCOMPtr<nsIHistoryEntry> hEntry;
    result = mSHistory->GetEntryAtIndex(mIndex, PR_FALSE, getter_AddRefs(hEntry));
    if (hEntry)
      result = CallQueryInterface(hEntry, aItem);
  }
  return result;
}
