





































#include "AsyncFaviconHelpers.h"
#include "mozilla/storage.h"
#include "nsNetUtil.h"

#include "nsStreamUtils.h"
#include "nsIContentSniffer.h"
#include "nsICacheService.h"
#include "nsICacheVisitor.h"
#include "nsICachingChannel.h"

#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsFaviconService.h"
#include "nsIAsyncVerifyRedirectCallback.h"

#include "nsCycleCollectionParticipant.h"

#define TO_CHARBUFFER(_buffer) \
  reinterpret_cast<char*>(const_cast<PRUint8*>(_buffer))
#define TO_INTBUFFER(_string) \
  reinterpret_cast<PRUint8*>(const_cast<char*>(_string.get()))

#ifdef DEBUG
#define INHERITED_ERROR_HANDLER \
  nsresult rv = AsyncStatementCallback::HandleError(aError); \
  NS_ENSURE_SUCCESS(rv, rv);
#else
#define INHERITED_ERROR_HANDLER
#endif

#define ASYNC_STATEMENT_HANDLEERROR_IMPL(_class) \
NS_IMETHODIMP \
_class::HandleError(mozIStorageError *aError) \
{ \
  INHERITED_ERROR_HANDLER \
  FAVICONSTEP_FAIL_IF_FALSE_RV(false, NS_OK); \
}

#define ASYNC_STATEMENT_EMPTY_HANDLERESULT_IMPL(_class) \
NS_IMETHODIMP \
_class::HandleResult(mozIStorageResultSet* aResultSet) \
{ \
  NS_NOTREACHED("Got an unexpected result?");\
  return NS_OK; \
}

#define CONTENT_SNIFFING_SERVICES "content-sniffing-services"







#define MAX_FAVICON_EXPIRATION ((PRTime)7 * 24 * 60 * 60 * PR_USEC_PER_SEC)

namespace mozilla {
namespace places {




NS_IMPL_ISUPPORTS0(
  AsyncFaviconStep
)





NS_IMPL_ISUPPORTS0(
  AsyncFaviconStepper
)


AsyncFaviconStepper::AsyncFaviconStepper(nsIFaviconDataCallback* aCallback)
  : mStepper(new AsyncFaviconStepperInternal(aCallback))
{
}


nsresult
AsyncFaviconStepper::Start()
{
  FAVICONSTEP_FAIL_IF_FALSE_RV(mStepper->mStatus == STEPPER_INITING,
                               NS_ERROR_FAILURE);
  mStepper->mStatus = STEPPER_RUNNING;
  nsresult rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);
  return NS_OK;
}


nsresult
AsyncFaviconStepper::AppendStep(AsyncFaviconStep* aStep)
{
  FAVICONSTEP_FAIL_IF_FALSE_RV(aStep, NS_ERROR_OUT_OF_MEMORY);
  FAVICONSTEP_FAIL_IF_FALSE_RV(mStepper->mStatus == STEPPER_INITING,
                               NS_ERROR_FAILURE);

  aStep->SetStepper(mStepper);
  nsresult rv = mStepper->mSteps.AppendObject(aStep);
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);
  return NS_OK;
}


nsresult
AsyncFaviconStepper::SetIconData(const nsACString& aMimeType,
                                 const PRUint8* _data,
                                 PRUint32 _dataLen)
{
  mStepper->mMimeType = aMimeType;
  mStepper->mData.Adopt(TO_CHARBUFFER(_data), _dataLen);
  mStepper->mIconStatus |= ICON_STATUS_CHANGED;
  return NS_OK;
}


nsresult
AsyncFaviconStepper::GetIconData(nsACString& aMimeType,
                                 const PRUint8** aData,
                                 PRUint32* aDataLen)
{
  PRUint32 dataLen = mStepper->mData.Length();
  NS_ENSURE_TRUE(dataLen > 0, NS_ERROR_NOT_AVAILABLE);
  aMimeType = mStepper->mMimeType;
  *aDataLen = dataLen;
  *aData = TO_INTBUFFER(mStepper->mData);
  return NS_OK;
}





NS_IMPL_ISUPPORTS0(
  AsyncFaviconStepperInternal
)


AsyncFaviconStepperInternal::AsyncFaviconStepperInternal(
  nsIFaviconDataCallback* aCallback
)
  : mCallback(aCallback)
  , mPageId(0)
  , mIconId(0)
  , mExpiration(0)
  , mIsRevisit(false)
  , mIconStatus(ICON_STATUS_UNKNOWN)
  , mStatus(STEPPER_INITING)
{
}


nsresult
AsyncFaviconStepperInternal::Step()
{
  if (mStatus != STEPPER_RUNNING) {
    Failure();
    return NS_ERROR_FAILURE;
  }

  PRInt32 stepCount = mSteps.Count();
  if (!stepCount) {
    mStatus = STEPPER_COMPLETED;
    
    if (mCallback) {
      (void)mCallback->OnFaviconDataAvailable(mIconURI,
                                              mData.Length(),
                                              TO_INTBUFFER(mData),
                                              mMimeType);
    }
    return NS_OK;
  }

  
  nsCOMPtr<AsyncFaviconStep> step = mSteps[0];
  if (!step) {
    Failure();
    return NS_ERROR_UNEXPECTED;
  }

  
  nsresult rv = mSteps.RemoveObjectAt(0);
  if (NS_FAILED(rv)) {
    Failure();
    return NS_ERROR_UNEXPECTED;
  }

  
  step->Run();

  return NS_OK;
}


void
AsyncFaviconStepperInternal::Failure()
{
  mStatus = STEPPER_FAILED;

  
  mSteps.Clear();
}


void
AsyncFaviconStepperInternal::Cancel(bool aNotify)
{
  mStatus = STEPPER_CANCELED;

  
  mSteps.Clear();

  if (aNotify && mCallback) {
    (void)mCallback->OnFaviconDataAvailable(mIconURI,
                                            mData.Length(),
                                            TO_INTBUFFER(mData),
                                            mMimeType);
  }
}





NS_IMPL_ISUPPORTS_INHERITED0(
  GetEffectivePageStep
, AsyncFaviconStep
)
ASYNC_STATEMENT_HANDLEERROR_IMPL(GetEffectivePageStep)


GetEffectivePageStep::GetEffectivePageStep()
  : mSubStep(0)
  , mIsBookmarked(false)
{
}


void
GetEffectivePageStep::Run()
{
  NS_ASSERTION(mStepper, "Step is not associated to a stepper");
  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mPageURI);
  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mIconURI);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  FAVICONSTEP_FAIL_IF_FALSE(history);
  PRBool canAddToHistory;
  nsresult rv = history->CanAddURI(mStepper->mPageURI, &canAddToHistory);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  
  
  if (!canAddToHistory) {
    
    mozIStorageStatement* stmt = history->GetStatementById(DB_GET_PAGE_INFO_BY_URL);
    
    FAVICONSTEP_CANCEL_IF_TRUE(!stmt, PR_FALSE);
    mozStorageStatementScoper scoper(stmt);

    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                                  mStepper->mPageURI);
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

    nsCOMPtr<mozIStoragePendingStatement> ps;
    rv = stmt->ExecuteAsync(this, getter_AddRefs(ps));
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

    
    scoper.Abandon();
  }
  else {
    CheckPageAndProceed();
  }
}


NS_IMETHODIMP
GetEffectivePageStep::HandleResult(mozIStorageResultSet* aResultSet)
{
  nsCOMPtr<mozIStorageRow> row;
  nsresult rv = aResultSet->GetNextRow(getter_AddRefs(row));
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  if (mSubStep == 0) {
    rv = row->GetInt64(0, &mStepper->mPageId);
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);
  }
  else {
    NS_ASSERTION(mSubStep == 1, "Wrong sub-step?");
    nsCAutoString spec;
    rv = row->GetUTF8String(0, spec);
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);
    
    rv = NS_NewURI(getter_AddRefs(mStepper->mPageURI), spec);
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);
    
    mIsBookmarked = true;
  }

  return NS_OK;
}


NS_IMETHODIMP
GetEffectivePageStep::HandleCompletion(PRUint16 aReason)
{
  FAVICONSTEP_FAIL_IF_FALSE_RV(aReason == mozIStorageStatementCallback::REASON_FINISHED, NS_OK);

  if (mSubStep == 0) {
    
    mSubStep++;
    
    FAVICONSTEP_CANCEL_IF_TRUE_RV(mStepper->mPageId == 0, PR_FALSE, NS_OK);

    
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    FAVICONSTEP_FAIL_IF_FALSE_RV(bookmarks, NS_ERROR_OUT_OF_MEMORY);
    mozIStorageStatement* stmt = bookmarks->GetStatementById(DB_FIND_REDIRECTED_BOOKMARK);
    
    FAVICONSTEP_CANCEL_IF_TRUE_RV(!stmt, PR_FALSE, NS_OK);
    mozStorageStatementScoper scoper(stmt);

    nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"),
                                        mStepper->mPageId);
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

    nsCOMPtr<mozIStoragePendingStatement> ps;
    rv = stmt->ExecuteAsync(this, getter_AddRefs(ps));
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

    
    scoper.Abandon();
  }
  else {
    NS_ASSERTION(mSubStep == 1, "Wrong sub-step?");
    
    FAVICONSTEP_CANCEL_IF_TRUE_RV(!mIsBookmarked, PR_FALSE, NS_OK);

    CheckPageAndProceed();
  }

  return NS_OK;
}


void
GetEffectivePageStep::CheckPageAndProceed()
{
  
  
  
  PRBool pageEqualsFavicon;
  nsresult rv = mStepper->mPageURI->Equals(mStepper->mIconURI,
                                           &pageEqualsFavicon);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  FAVICONSTEP_CANCEL_IF_TRUE(pageEqualsFavicon, PR_FALSE);

  
  nsCOMPtr<nsIURI> errorPageFaviconURI;
  rv = NS_NewURI(getter_AddRefs(errorPageFaviconURI),
                 NS_LITERAL_CSTRING(FAVICON_ERRORPAGE_URL));
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  PRBool isErrorPage;
  rv = mStepper->mIconURI->Equals(errorPageFaviconURI, &isErrorPage);
  FAVICONSTEP_CANCEL_IF_TRUE(isErrorPage, PR_FALSE);

  
  rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
}





NS_IMPL_ISUPPORTS_INHERITED0(
  FetchDatabaseIconStep
, AsyncFaviconStep
)
ASYNC_STATEMENT_HANDLEERROR_IMPL(FetchDatabaseIconStep)


void
FetchDatabaseIconStep::Run()
{
  NS_ASSERTION(mStepper, "Step is not associated to a stepper");
  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mIconURI);

  
  
  nsFaviconService* fs = nsFaviconService::GetFaviconService();
  FAVICONSTEP_FAIL_IF_FALSE(fs);
  mozIStorageStatement* stmt =
    fs->GetStatementById(mozilla::places::DB_GET_ICON_INFO_WITH_PAGE);
  FAVICONSTEP_CANCEL_IF_TRUE(!stmt, PR_FALSE);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"),
                                mStepper->mIconURI);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  if (mStepper->mPageURI) {
    rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                         mStepper->mPageURI);
  }
  else {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("page_url"));
  }
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = stmt->ExecuteAsync(this, getter_AddRefs(ps));
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  
  scoper.Abandon();
}


NS_IMETHODIMP
FetchDatabaseIconStep::HandleResult(mozIStorageResultSet* aResultSet)
{
  nsCOMPtr<mozIStorageRow> row;
  nsresult rv = aResultSet->GetNextRow(getter_AddRefs(row));
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  rv = row->GetInt64(0, &mStepper->mIconId);
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  
  
  

  rv = row->GetInt64(2, &mStepper->mExpiration);
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  PRUint8* data;
  PRUint32 dataLen = 0;
  rv = row->GetBlob(3, &dataLen, &data);
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);
  mStepper->mData.Adopt(TO_CHARBUFFER(data), dataLen);
  rv = row->GetUTF8String(4, mStepper->mMimeType);
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  PRInt32 isRevisit;
  rv = row->GetInt32(5, &isRevisit);
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);
  mStepper->mIsRevisit = !!isRevisit;

  return NS_OK;
}


NS_IMETHODIMP
FetchDatabaseIconStep::HandleCompletion(PRUint16 aReason)
{
  FAVICONSTEP_FAIL_IF_FALSE_RV(aReason == mozIStorageStatementCallback::REASON_FINISHED, NS_OK);

  
  nsresult rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED0(
  EnsureDatabaseEntryStep
, AsyncFaviconStep
)
ASYNC_STATEMENT_HANDLEERROR_IMPL(EnsureDatabaseEntryStep)
ASYNC_STATEMENT_EMPTY_HANDLERESULT_IMPL(EnsureDatabaseEntryStep)


void
EnsureDatabaseEntryStep::Run()
{
  NS_ASSERTION(mStepper, "Step is not associated to a stepper");
  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mIconURI);
  nsresult rv;

  
  if (mStepper->mIconId > 0 || mStepper->mIsRevisit) {
    rv = mStepper->Step();
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
    return;
  }

  
  nsFaviconService* fs = nsFaviconService::GetFaviconService();
  FAVICONSTEP_FAIL_IF_FALSE(fs);
  mozIStorageStatement* stmt =
    fs->GetStatementById(mozilla::places::DB_INSERT_ICON);
  
  FAVICONSTEP_CANCEL_IF_TRUE(!stmt, PR_FALSE);
  mozStorageStatementScoper scoper(stmt);
  rv = stmt->BindNullByName(NS_LITERAL_CSTRING("icon_id"));
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"),
                       mStepper->mIconURI);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = stmt->ExecuteAsync(this, getter_AddRefs(ps));
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  
  scoper.Abandon();
}


NS_IMETHODIMP
EnsureDatabaseEntryStep::HandleCompletion(PRUint16 aReason)
{
  FAVICONSTEP_FAIL_IF_FALSE_RV(aReason == mozIStorageStatementCallback::REASON_FINISHED, NS_OK);

  
  nsresult rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  return NS_OK;
}





NS_IMPL_CYCLE_COLLECTION_CLASS(FetchNetworkIconStep)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FetchNetworkIconStep)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
NS_INTERFACE_MAP_END_INHERITING(AsyncFaviconStep)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(FetchNetworkIconStep)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannel)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(FetchNetworkIconStep)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChannel)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(FetchNetworkIconStep)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FetchNetworkIconStep)


FetchNetworkIconStep::FetchNetworkIconStep(enum AsyncFaviconFetchMode aFetchMode)
  : mFetchMode(aFetchMode)
{
}


void
FetchNetworkIconStep::Run()
{
  NS_ASSERTION(mStepper, "Step is not associated to a stepper");

  if (mFetchMode == FETCH_NEVER) {
    
    FAVICONSTEP_CANCEL_IF_TRUE(mStepper->mData.Length() == 0, PR_FALSE);
    
    nsresult rv = mStepper->Step();
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  }

  bool isExpired = PR_Now() < mStepper->mExpiration;
  if (mStepper->mData.Length() > 0 && !isExpired &&
      mFetchMode == FETCH_IF_MISSING) {
    
    nsresult rv = mStepper->Step();
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
    return;
  }

  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mIconURI);

  nsresult rv = NS_NewChannel(getter_AddRefs(mChannel), mStepper->mIconURI);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsIInterfaceRequestor> listenerRequestor =
    do_QueryInterface(reinterpret_cast<nsISupports*>(this), &rv);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  rv = mChannel->SetNotificationCallbacks(listenerRequestor);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  rv = mChannel->AsyncOpen(this, nsnull);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
}


NS_IMETHODIMP
FetchNetworkIconStep::OnStartRequest(nsIRequest* aRequest,
                                     nsISupports* aContext)
{
  return NS_OK;
}


NS_IMETHODIMP
FetchNetworkIconStep::OnStopRequest(nsIRequest* aRequest,
                                    nsISupports* aContext,
                                    nsresult aStatusCode)
{
  nsFaviconService* fs = nsFaviconService::GetFaviconService();
  FAVICONSTEP_FAIL_IF_FALSE_RV(fs, NS_ERROR_OUT_OF_MEMORY);

  if (NS_FAILED(aStatusCode) || mData.Length() == 0) {
    
    fs->AddFailedFavicon(mStepper->mIconURI);
    FAVICONSTEP_CANCEL_IF_TRUE_RV(true, PR_FALSE, NS_OK);
  }

  
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  FAVICONSTEP_FAIL_IF_FALSE_RV(categoryManager, NS_ERROR_OUT_OF_MEMORY);
  nsCOMPtr<nsISimpleEnumerator> sniffers;
  nsresult rv = categoryManager->EnumerateCategory(CONTENT_SNIFFING_SERVICES,
                                                   getter_AddRefs(sniffers));
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  nsCAutoString mimeType;
  PRBool hasMore = PR_FALSE;
  while (mimeType.IsEmpty() &&
         NS_SUCCEEDED(sniffers->HasMoreElements(&hasMore)) &&
         hasMore) {
    nsCOMPtr<nsISupports> snifferCIDSupports;
    rv = sniffers->GetNext(getter_AddRefs(snifferCIDSupports));
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

    nsCOMPtr<nsISupportsCString> snifferCIDSupportsCString =
      do_QueryInterface(snifferCIDSupports, &rv);
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

    nsCAutoString snifferCID;
    rv = snifferCIDSupportsCString->GetData(snifferCID);
    FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

    nsCOMPtr<nsIContentSniffer> sniffer = do_GetService(snifferCID.get());
    FAVICONSTEP_FAIL_IF_FALSE_RV(sniffer, rv);

     
    (void)sniffer->GetMIMETypeFromContent(aRequest, TO_INTBUFFER(mData),
                                          mData.Length(), mimeType);
  }

  if (mimeType.IsEmpty()) {
    
    fs->AddFailedFavicon(mStepper->mIconURI);
    FAVICONSTEP_CANCEL_IF_TRUE_RV(true, PR_FALSE, NS_OK);
  }

  
  
  PRTime expiration = -1;
  nsCOMPtr<nsICachingChannel> cachingChannel(do_QueryInterface(mChannel));
  if (cachingChannel) {
    nsCOMPtr<nsISupports> cacheToken;
    rv = cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsICacheEntryInfo> cacheEntry(do_QueryInterface(cacheToken));
      PRUint32 seconds;
      rv = cacheEntry->GetExpirationTime(&seconds);
      if (NS_SUCCEEDED(rv)) {
        
        expiration = PR_Now() + NS_MIN((PRTime)seconds * PR_USEC_PER_SEC,
                                       MAX_FAVICON_EXPIRATION);
      }
    }
  }
  
  if (expiration < 0) {
    expiration = PR_Now() + MAX_FAVICON_EXPIRATION;
  }

  mStepper->mExpiration = expiration;
  mStepper->mMimeType = mimeType;
  mStepper->mData = mData;
  mStepper->mIconStatus |= ICON_STATUS_CHANGED;

  
  rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  return NS_OK;
}


NS_IMETHODIMP
FetchNetworkIconStep::OnDataAvailable(nsIRequest* aRequest,
                                      nsISupports* aContext,
                                      nsIInputStream* aInputStream,
                                      PRUint32 aOffset,
                                      PRUint32 aCount)
{
  nsCAutoString buffer;
  nsresult rv = NS_ConsumeStream(aInputStream, aCount, buffer);
  if (rv != NS_BASE_STREAM_WOULD_BLOCK && NS_FAILED(rv)) {
    return rv;
  }

  mData.Append(buffer);
  return NS_OK;
}


NS_IMETHODIMP
FetchNetworkIconStep::GetInterface(const nsIID& uuid,
                                   void** aResult)
{
  return QueryInterface(uuid, aResult);
}


NS_IMETHODIMP
FetchNetworkIconStep::AsyncOnChannelRedirect(nsIChannel* oldChannel,
                                             nsIChannel* newChannel,
                                             PRUint32 flags,
                                             nsIAsyncVerifyRedirectCallback *cb)
{
  mChannel = newChannel;
  cb->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED0(
  SetFaviconDataStep
, AsyncFaviconStep
)
ASYNC_STATEMENT_HANDLEERROR_IMPL(SetFaviconDataStep)
ASYNC_STATEMENT_EMPTY_HANDLERESULT_IMPL(SetFaviconDataStep)


void
SetFaviconDataStep::Run()
{
  NS_ASSERTION(mStepper, "Step is not associated to a stepper");
  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mIconURI);
  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mData.Length() > 0);
  FAVICONSTEP_FAIL_IF_FALSE(!mStepper->mMimeType.IsEmpty());

  nsresult rv;
  if (!(mStepper->mIconStatus & ICON_STATUS_CHANGED)) {
    
    rv = mStepper->Step();
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
    return;
  }

  nsFaviconService* fs = nsFaviconService::GetFaviconService();
  FAVICONSTEP_FAIL_IF_FALSE(fs);

  
  
  
  nsCAutoString newData, newMimeType;
  if (mStepper->mData.Length() > MAX_ICON_FILESIZE(fs->GetOptimizedIconDimension())) {
    rv = fs->OptimizeFaviconImage(TO_INTBUFFER(mStepper->mData),
                                  mStepper->mData.Length(),
                                  mStepper->mMimeType,
                                  newData,
                                  newMimeType);
    if (NS_SUCCEEDED(rv) && newData.Length() < mStepper->mData.Length()) {
      mStepper->mData = newData;
      mStepper->mMimeType = newMimeType;
    }

    
    
    FAVICONSTEP_CANCEL_IF_TRUE(mStepper->mData.Length() > MAX_FAVICON_SIZE, PR_FALSE);
  }

  
  mozIStorageStatement* stmt =
    fs->GetStatementById(mozilla::places::DB_INSERT_ICON);
  
  FAVICONSTEP_CANCEL_IF_TRUE(!stmt, PR_FALSE);
  mozStorageStatementScoper scoper(stmt);

  if (!mStepper->mIconId) {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("icon_id"));
  }
  else {
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("icon_id"),
                               mStepper->mIconId);
  }
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"),
                       mStepper->mIconURI);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  rv = stmt->BindBlobByName(NS_LITERAL_CSTRING("data"),
                            TO_INTBUFFER(mStepper->mData),
                            mStepper->mData.Length());
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("mime_type"),
                                  mStepper->mMimeType);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("expiration"),
                             mStepper->mExpiration);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = stmt->ExecuteAsync(this, getter_AddRefs(ps));
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  
  scoper.Abandon();
}


NS_IMETHODIMP
SetFaviconDataStep::HandleCompletion(PRUint16 aReason)
{
  FAVICONSTEP_FAIL_IF_FALSE_RV(aReason == mozIStorageStatementCallback::REASON_FINISHED, NS_OK);

  mStepper->mIconStatus |= ICON_STATUS_SAVED;

  
  nsresult rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED0(
  AssociateIconWithPageStep
, AsyncFaviconStep
)
ASYNC_STATEMENT_HANDLEERROR_IMPL(AssociateIconWithPageStep)
ASYNC_STATEMENT_EMPTY_HANDLERESULT_IMPL(AssociateIconWithPageStep)


void
AssociateIconWithPageStep::Run() {
  NS_ASSERTION(mStepper, "Step is not associated to a stepper");

  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mIconURI);
  FAVICONSTEP_FAIL_IF_FALSE(mStepper->mPageURI);

  
  
  
  
  
  
  
  if (!mStepper->mPageId) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    FAVICONSTEP_FAIL_IF_FALSE(history);
    nsresult rv = history->GetUrlIdFor(mStepper->mPageURI,
                                       &mStepper->mPageId,
                                       PR_TRUE); 
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  }

  nsFaviconService* fs = nsFaviconService::GetFaviconService();
  FAVICONSTEP_FAIL_IF_FALSE(fs);
  mozIStorageStatement* stmt =
    fs->GetStatementById(mozilla::places::DB_ASSOCIATE_ICONURI_TO_PAGEURI);
  
  FAVICONSTEP_CANCEL_IF_TRUE(!stmt, PR_FALSE);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"),
                                mStepper->mIconURI);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                       mStepper->mPageURI);
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = stmt->ExecuteAsync(this, getter_AddRefs(ps));
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));

  
  scoper.Abandon();
}


NS_IMETHODIMP
AssociateIconWithPageStep::HandleCompletion(PRUint16 aReason)
{
  FAVICONSTEP_FAIL_IF_FALSE_RV(aReason == mozIStorageStatementCallback::REASON_FINISHED, NS_OK);

  mStepper->mIconStatus |= ICON_STATUS_ASSOCIATED;

  
  nsresult rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE_RV(NS_SUCCEEDED(rv), rv);

  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED0(
  NotifyStep
, AsyncFaviconStep
)


void
NotifyStep::Run()
{
  NS_ASSERTION(mStepper, "Step is not associated to a stepper");
  
  
  FAVICONSTEP_CANCEL_IF_TRUE(mStepper->mData.Length() == 0, PR_FALSE);

  if (mStepper->mIconStatus & ICON_STATUS_SAVED ||
      mStepper->mIconStatus & ICON_STATUS_ASSOCIATED) {
    nsFaviconService* fs = nsFaviconService::GetFaviconService();
    FAVICONSTEP_FAIL_IF_FALSE(fs);
    fs->SendFaviconNotifications(mStepper->mPageURI, mStepper->mIconURI);
    nsresult rv = fs->UpdateBookmarkRedirectFavicon(mStepper->mPageURI,
                                                    mStepper->mIconURI);
    FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
  }

  
  nsresult rv = mStepper->Step();
  FAVICONSTEP_FAIL_IF_FALSE(NS_SUCCEEDED(rv));
}

} 
} 
