





































#include "AsyncFaviconHelpers.h"
#include "mozilla/storage.h"
#include "nsNetUtil.h"

#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsFaviconService.h"

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

  
  
  if (!canAddToHistory || history->IsHistoryDisabled()) {
    
    mozIStorageStatement* stmt = history->GetStatementById(DB_GET_PAGE_INFO);
    
    FAVICONSTEP_CANCEL_IF_TRUE(!stmt, PR_FALSE);
    mozStorageStatementScoper scoper(stmt);

    nsresult rv = BindStatementURI(stmt, 0, mStepper->mPageURI);
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
    
    rv = mStepper->mPageURI->SetSpec(spec);
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

    nsresult rv = stmt->BindInt64Parameter(0, mStepper->mPageId);
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




} 
} 
