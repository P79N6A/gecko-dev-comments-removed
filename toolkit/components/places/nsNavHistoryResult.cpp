









































#include <stdio.h>
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsFaviconService.h"
#include "nsITaggingService.h"
#include "nsAnnotationService.h"
#include "Helpers.h"

#include "nsDebug.h"
#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "prtime.h"
#include "prprf.h"

#include "nsIDynamicContainer.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIClassInfo.h"
#include "nsIProgrammingLanguage.h"
#include "nsIXPCScriptable.h"

#define TO_ICONTAINER(_node)                                                  \
    static_cast<nsINavHistoryContainerResultNode*>(_node)                      

#define TO_CONTAINER(_node)                                                   \
    static_cast<nsNavHistoryContainerResultNode*>(_node)

#define NOTIFY_RESULT_OBSERVERS(_result, _method)                             \
  PR_BEGIN_MACRO                                                              \
  NS_ENSURE_STATE(_result);                                                   \
  if (!_result->mSuppressNotifications) {                                     \
    ENUMERATE_WEAKARRAY(_result->mObservers, nsINavHistoryResultObserver,     \
                        _method)                                              \
  }                                                                           \
  PR_END_MACRO






#define NS_INTERFACE_MAP_STATIC_AMBIGUOUS(_class) \
  if (aIID.Equals(NS_GET_IID(_class))) { \
    NS_ADDREF(this); \
    *aInstancePtr = this; \
    return NS_OK; \
  } else


inline PRInt32 ComparePRTime(PRTime a, PRTime b)
{
  if (LL_CMP(a, <, b))
    return -1;
  else if (LL_CMP(a, >, b))
    return 1;
  return 0;
}
inline PRInt32 CompareIntegers(PRUint32 a, PRUint32 b)
{
  return a - b;
}

namespace mozilla {
  namespace places {
    
    
    class ResultNodeClassInfo : public nsIClassInfo
                              , public nsIXPCScriptable
    {
      NS_DECL_ISUPPORTS
      NS_DECL_NSIXPCSCRIPTABLE

      
      NS_IMETHODIMP
      GetInterfaces(PRUint32 *_count, nsIID ***_array)
      {
        *_count = 0;
        *_array = nsnull;

        return NS_OK;
      }

      NS_IMETHODIMP
      GetHelperForLanguage(PRUint32 aLanguage, nsISupports **_helper)
      {
        if (aLanguage == nsIProgrammingLanguage::JAVASCRIPT) {
          *_helper = static_cast<nsIXPCScriptable *>(this);
          NS_ADDREF(*_helper);
        }
        else
          *_helper = nsnull;

        return NS_OK;
      }

      NS_IMETHODIMP
      GetContractID(char **_contractID)
      {
        *_contractID = nsnull;
        return NS_OK;
      }

      NS_IMETHODIMP
      GetClassDescription(char **_desc)
      {
        *_desc = nsnull;
        return NS_OK;
      }

      NS_IMETHODIMP
      GetClassID(nsCID **_id)
      {
        *_id = nsnull;
        return NS_OK;
      }

      NS_IMETHODIMP
      GetImplementationLanguage(PRUint32 *_language)
      {
        *_language = nsIProgrammingLanguage::CPLUSPLUS;
        return NS_OK;
      }

      NS_IMETHODIMP
      GetFlags(PRUint32 *_flags)
      {
        *_flags = 0;
        return NS_OK;
      }

      NS_IMETHODIMP
      GetClassIDNoAlloc(nsCID *_cid)
      {
        return NS_ERROR_NOT_AVAILABLE;
      }
    };

    




    NS_IMETHODIMP_(nsrefcnt) ResultNodeClassInfo::AddRef()
    {
      return 2;
    }
    NS_IMETHODIMP_(nsrefcnt) ResultNodeClassInfo::Release()
    {
      return 1;
    }

    NS_IMPL_QUERY_INTERFACE2(ResultNodeClassInfo, nsIClassInfo, nsIXPCScriptable)

#define XPC_MAP_CLASSNAME ResultNodeClassInfo
#define XPC_MAP_QUOTED_CLASSNAME "ResultNodeClassInfo"
#define XPC_MAP_FLAGS nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY | \
                      nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY | \
                      nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY



#include "xpc_map_end.h"    

    static ResultNodeClassInfo sResultNodeClassInfo;
  } 
} 

using namespace mozilla::places;

NS_IMPL_CYCLE_COLLECTION_CLASS(nsNavHistoryResultNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsNavHistoryResultNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END 

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsNavHistoryResultNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mParent, nsINavHistoryContainerResultNode);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNavHistoryResultNode)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryResultNode)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo)))
    foundInterface = static_cast<nsIClassInfo *>(&mozilla::places::sResultNodeClassInfo);
  else
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryResultNode)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNavHistoryResultNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNavHistoryResultNode)

nsNavHistoryResultNode::nsNavHistoryResultNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI) :
  mParent(nsnull),
  mURI(aURI),
  mTitle(aTitle),
  mAreTagsSorted(false),
  mAccessCount(aAccessCount),
  mTime(aTime),
  mFaviconURI(aIconURI),
  mBookmarkIndex(-1),
  mItemId(-1),
  mFolderId(-1),
  mDateAdded(0),
  mLastModified(0),
  mIndentLevel(-1),
  mFrecency(0)
{
  mTags.SetIsVoid(PR_TRUE);
}


NS_IMETHODIMP
nsNavHistoryResultNode::GetIcon(nsACString& aIcon)
{
  if (mFaviconURI.IsEmpty()) {
    aIcon.Truncate();
    return NS_OK;
  }

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);
  faviconService->GetFaviconSpecForIconString(mFaviconURI, aIcon);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResultNode::GetParent(nsINavHistoryContainerResultNode** aParent)
{
  NS_IF_ADDREF(*aParent = mParent);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResultNode::GetParentResult(nsINavHistoryResult** aResult)
{
  *aResult = nsnull;
  if (IsContainer())
    NS_IF_ADDREF(*aResult = GetAsContainer()->mResult);
  else if (mParent)
    NS_IF_ADDREF(*aResult = mParent->mResult);

  NS_ENSURE_STATE(*aResult);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResultNode::GetTags(nsAString& aTags) {
  
  if (!IsURI()) {
    aTags.Truncate();
    return NS_OK;
  }

  
  
  
  
  if (!mTags.IsVoid()) {
    
    
    if (!mAreTagsSorted) {
      nsTArray<nsCString> tags;
      ParseString(NS_ConvertUTF16toUTF8(mTags), ',', tags);
      tags.Sort();
      mTags.SetIsVoid(PR_TRUE);
      for (nsTArray<nsCString>::index_type i = 0; i < tags.Length(); ++i) {
        AppendUTF8toUTF16(tags[i], mTags);
        if (i < tags.Length() - 1 )
          mTags.AppendLiteral(", ");
      }
      mAreTagsSorted = true;
    }
    aTags.Assign(mTags);
    return NS_OK;
  }

  
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  mozIStorageStatement *stmt = history->GetStatementById(DB_GET_TAGS);
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("tags_folder"),
                                      history->GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasTags = PR_FALSE;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasTags)) && hasTags) {
    rv = stmt->GetString(0, mTags);
    NS_ENSURE_SUCCESS(rv, rv);
    aTags.Assign(mTags);
    mAreTagsSorted = true;
  }

  
  
  if (mParent && mParent->IsQuery() &&
      mParent->mOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY) {
    nsNavHistoryQueryResultNode* query = mParent->GetAsQuery();
    nsNavHistoryResult* result = query->GetResult();
    NS_ENSURE_STATE(result);
    result->AddAllBookmarksObserver(query);
  }

  return NS_OK;
}


void
nsNavHistoryResultNode::OnRemoving()
{
  mParent = nsnull;
}







nsNavHistoryResult*
nsNavHistoryResultNode::GetResult()
{
  nsNavHistoryResultNode* node = this;
  do {
    if (node->IsContainer()) {
      nsNavHistoryContainerResultNode* container = TO_CONTAINER(node);
      NS_ASSERTION(container->mResult, "Containers must have valid results");
      return container->mResult;
    }
    node = node->mParent;
  } while (node);
  NS_NOTREACHED("No container node found in hierarchy!");
  return nsnull;
}











nsNavHistoryQueryOptions*
nsNavHistoryResultNode::GetGeneratingOptions()
{
  if (!mParent) {
    
    
    
    
    if (IsContainer())
      return GetAsContainer()->mOptions;

    NS_NOTREACHED("Can't find a generating node for this container, perhaps FillStats has not been called on this tree yet?");
    return nsnull;
  }

  
  
  
  nsNavHistoryContainerResultNode* cur = mParent;
  while (cur) {
    if (cur->IsContainer())
      return cur->GetAsContainer()->mOptions;
    cur = cur->mParent;
  }

  
  NS_NOTREACHED("Can't find a generating node for this container, the tree seemes corrupted.");
  return nsnull;
}


NS_IMPL_ISUPPORTS_INHERITED1(nsNavHistoryVisitResultNode,
                             nsNavHistoryResultNode,
                             nsINavHistoryVisitResultNode)

nsNavHistoryVisitResultNode::nsNavHistoryVisitResultNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, PRInt64 aSession) :
  nsNavHistoryResultNode(aURI, aTitle, aAccessCount, aTime, aIconURI),
  mSessionId(aSession)
{
}


NS_IMPL_ISUPPORTS_INHERITED1(nsNavHistoryFullVisitResultNode,
                             nsNavHistoryVisitResultNode,
                             nsINavHistoryFullVisitResultNode)

nsNavHistoryFullVisitResultNode::nsNavHistoryFullVisitResultNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, PRInt64 aSession,
    PRInt64 aVisitId, PRInt64 aReferringVisitId, PRInt32 aTransitionType) :
  nsNavHistoryVisitResultNode(aURI, aTitle, aAccessCount, aTime, aIconURI,
                              aSession),
  mVisitId(aVisitId),
  mReferringVisitId(aReferringVisitId),
  mTransitionType(aTransitionType)
{
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsNavHistoryContainerResultNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mResult)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mChildren)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END 

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mResult, nsINavHistoryResult)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mChildren)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)
NS_IMPL_RELEASE_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsNavHistoryContainerResultNode)
  NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsNavHistoryContainerResultNode)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryContainerResultNode)
NS_INTERFACE_MAP_END_INHERITING(nsNavHistoryResultNode)

nsNavHistoryContainerResultNode::nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    const nsACString& aIconURI, PRUint32 aContainerType, PRBool aReadOnly,
    const nsACString& aDynamicContainerType, nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryResultNode(aURI, aTitle, 0, 0, aIconURI),
  mResult(nsnull),
  mContainerType(aContainerType),
  mExpanded(PR_FALSE),
  mChildrenReadOnly(aReadOnly),
  mOptions(aOptions),
  mDynamicContainerType(aDynamicContainerType),
  mAsyncCanceledState(NOT_CANCELED)
{
}

nsNavHistoryContainerResultNode::nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    PRTime aTime,
    const nsACString& aIconURI, PRUint32 aContainerType, PRBool aReadOnly,
    const nsACString& aDynamicContainerType, 
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryResultNode(aURI, aTitle, 0, aTime, aIconURI),
  mResult(nsnull),
  mContainerType(aContainerType),
  mExpanded(PR_FALSE),
  mChildrenReadOnly(aReadOnly),
  mOptions(aOptions),
  mDynamicContainerType(aDynamicContainerType),
  mAsyncCanceledState(NOT_CANCELED)
{
}


nsNavHistoryContainerResultNode::~nsNavHistoryContainerResultNode()
{
  
  
  mChildren.Clear();
}






void
nsNavHistoryContainerResultNode::OnRemoving()
{
  nsNavHistoryResultNode::OnRemoving();
  for (PRInt32 i = 0; i < mChildren.Count(); ++i)
    mChildren[i]->OnRemoving();
  mChildren.Clear();
}


PRBool
nsNavHistoryContainerResultNode::AreChildrenVisible()
{
  nsNavHistoryResult* result = GetResult();
  if (!result) {
    NS_NOTREACHED("Invalid result");
    return PR_FALSE;
  }

  if (!mExpanded)
    return PR_FALSE;

  
  nsNavHistoryContainerResultNode* ancestor = mParent;
  while (ancestor) {
    if (!ancestor->mExpanded)
      return PR_FALSE;

    ancestor = ancestor->mParent;
  }

  return PR_TRUE;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetContainerOpen(PRBool *aContainerOpen)
{
  *aContainerOpen = mExpanded;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::SetContainerOpen(PRBool aContainerOpen)
{
  if (aContainerOpen) {
    if (!mExpanded) {
      nsNavHistoryQueryOptions* options = GetGeneratingOptions();
      if (options && options->AsyncEnabled())
        OpenContainerAsync();
      else
        OpenContainer();
    }
  }
  else {
    if (mExpanded)
      CloseContainer();
    else if (mAsyncPendingStmt)
      CancelAsyncOpen(PR_FALSE);
  }

  return NS_OK;
}










nsresult
nsNavHistoryContainerResultNode::NotifyOnStateChange(PRUint16 aOldState)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  nsresult rv;
  PRUint16 currState;
  rv = GetState(&currState);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NOTIFY_RESULT_OBSERVERS(result,
                          ContainerStateChanged(this, aOldState, currState));

  
  if (currState == STATE_OPENED)
    NOTIFY_RESULT_OBSERVERS(result, ContainerOpened(this));
  else if (currState == STATE_CLOSED)
    NOTIFY_RESULT_OBSERVERS(result, ContainerClosed(this));

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetState(PRUint16* _state)
{
  NS_ENSURE_ARG_POINTER(_state);

  *_state = mExpanded ? (PRUint16)STATE_OPENED
                      : mAsyncPendingStmt ? (PRUint16)STATE_LOADING
                                          : (PRUint16)STATE_CLOSED;

  return NS_OK;
}






nsresult
nsNavHistoryContainerResultNode::OpenContainer()
{
  NS_ASSERTION(!mExpanded, "Container must not be expanded to open it");
  mExpanded = PR_TRUE;

  nsresult rv;

  if (IsDynamicContainer()) {
    
    nsCOMPtr<nsIDynamicContainer> svc = do_GetService(mDynamicContainerType.get(), &rv);
    if (NS_SUCCEEDED(rv)) {
      svc->OnContainerNodeOpening(this, GetGeneratingOptions());
    } else {
      NS_WARNING("Unable to get dynamic container for ");
      NS_WARNING(mDynamicContainerType.get());
    }
    PRInt32 oldAccessCount = mAccessCount;
    FillStats();
    rv = ReverseUpdateStats(mAccessCount - oldAccessCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = NotifyOnStateChange(STATE_CLOSED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








nsresult
nsNavHistoryContainerResultNode::CloseContainer(PRBool aSuppressNotifications)
{
  NS_ASSERTION((mExpanded && !mAsyncPendingStmt) ||
               (!mExpanded && mAsyncPendingStmt),
               "Container must be expanded or loading to close it");

  nsresult rv;
  PRUint16 oldState;
  rv = GetState(&oldState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mExpanded) {
    
    for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
      if (mChildren[i]->IsContainer() &&
          mChildren[i]->GetAsContainer()->mExpanded)
        mChildren[i]->GetAsContainer()->CloseContainer(PR_TRUE);
    }

    mExpanded = PR_FALSE;

    if (IsDynamicContainer()) {
      
      nsCOMPtr<nsIDynamicContainer> svc =
        do_GetService(mDynamicContainerType.get());
      if (svc)
        svc->OnContainerNodeClosed(this);
    }
  }

  
  
  mAsyncPendingStmt = nsnull;

  if (!aSuppressNotifications) {
    rv = NotifyOnStateChange(oldState);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  if (result->mRootNode == this) {
    result->StopObserving();
    
    
    
    if (this->IsQuery())
      this->GetAsQuery()->ClearChildren(PR_TRUE);
    else if (this->IsFolder())
      this->GetAsFolder()->ClearChildren(PR_TRUE);
  }

  return NS_OK;
}





nsresult
nsNavHistoryContainerResultNode::OpenContainerAsync()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}











void
nsNavHistoryContainerResultNode::CancelAsyncOpen(PRBool aRestart)
{
  NS_ASSERTION(mAsyncPendingStmt, "Async execution canceled but not pending");

  mAsyncCanceledState = aRestart ? CANCELED_RESTART_NEEDED : CANCELED;

  
  
  
  (void)mAsyncPendingStmt->Cancel();
}











void
nsNavHistoryContainerResultNode::FillStats()
{
  PRUint32 accessCount = 0;
  PRTime newTime = 0;

  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    nsNavHistoryResultNode* node = mChildren[i];
    node->mParent = this;
    node->mIndentLevel = mIndentLevel + 1;
    if (node->IsContainer()) {
      nsNavHistoryContainerResultNode* container = node->GetAsContainer();
      container->mResult = mResult;
      container->FillStats();
    }
    accessCount += node->mAccessCount;
    
    
    if (node->mTime > newTime)
      newTime = node->mTime;
  }

  if (mExpanded) {
    mAccessCount = accessCount;
    if (!IsQuery() || newTime > mTime)
      mTime = newTime;
  }
}



















nsresult
nsNavHistoryContainerResultNode::ReverseUpdateStats(PRInt32 aAccessCountChange)
{
  if (mParent) {
    nsNavHistoryResult* result = GetResult();
    PRBool shouldNotify = result && mParent->mParent &&
                          mParent->mParent->AreChildrenVisible();

    mParent->mAccessCount += aAccessCountChange;
    PRBool timeChanged = PR_FALSE;
    if (mTime > mParent->mTime) {
      timeChanged = PR_TRUE;
      mParent->mTime = mTime;
    }

    if (shouldNotify) {
      NOTIFY_RESULT_OBSERVERS(result,
                              NodeHistoryDetailsChanged(TO_ICONTAINER(mParent),
                                                        mParent->mTime,
                                                        mParent->mAccessCount));
    }

    
    
    PRUint16 sortMode = mParent->GetSortType();
    PRBool sortingByVisitCount =
      sortMode == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING ||
      sortMode == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING;
    PRBool sortingByTime =
      sortMode == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING ||
      sortMode == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING;

    if ((sortingByVisitCount && aAccessCountChange != 0) ||
        (sortingByTime && timeChanged)) {
      PRUint32 ourIndex = mParent->FindChild(this);
      EnsureItemPosition(ourIndex);
    }

    nsresult rv = mParent->ReverseUpdateStats(aAccessCountChange);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






PRUint16
nsNavHistoryContainerResultNode::GetSortType()
{
  if (mParent)
    return mParent->GetSortType();
  if (mResult)
    return mResult->mSortingMode;

  NS_NOTREACHED("We should always have a result");
  return nsINavHistoryQueryOptions::SORT_BY_NONE;
}


nsresult nsNavHistoryContainerResultNode::Refresh() {
  NS_WARNING("Refresh() is supported by queries or folders, not generic containers.");
  return NS_OK;
}

void
nsNavHistoryContainerResultNode::GetSortingAnnotation(nsACString& aAnnotation)
{
  if (mParent)
    mParent->GetSortingAnnotation(aAnnotation);
  else if (mResult)
    aAnnotation.Assign(mResult->mSortingAnnotation);
  else
    NS_NOTREACHED("We should always have a result");
}





nsNavHistoryContainerResultNode::SortComparator
nsNavHistoryContainerResultNode::GetSortingComparator(PRUint16 aSortType)
{
  switch (aSortType)
  {
    case nsINavHistoryQueryOptions::SORT_BY_NONE:
      return &SortComparison_Bookmark;
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING:
      return &SortComparison_TitleLess;
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING:
      return &SortComparison_TitleGreater;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING:
      return &SortComparison_DateLess;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING:
      return &SortComparison_DateGreater;
    case nsINavHistoryQueryOptions::SORT_BY_URI_ASCENDING:
      return &SortComparison_URILess;
    case nsINavHistoryQueryOptions::SORT_BY_URI_DESCENDING:
      return &SortComparison_URIGreater;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING:
      return &SortComparison_VisitCountLess;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING:
      return &SortComparison_VisitCountGreater;
    case nsINavHistoryQueryOptions::SORT_BY_KEYWORD_ASCENDING:
      return &SortComparison_KeywordLess;
    case nsINavHistoryQueryOptions::SORT_BY_KEYWORD_DESCENDING:
      return &SortComparison_KeywordGreater;
    case nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_ASCENDING:
      return &SortComparison_AnnotationLess;
    case nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_DESCENDING:
      return &SortComparison_AnnotationGreater;
    case nsINavHistoryQueryOptions::SORT_BY_DATEADDED_ASCENDING:
      return &SortComparison_DateAddedLess;
    case nsINavHistoryQueryOptions::SORT_BY_DATEADDED_DESCENDING:
      return &SortComparison_DateAddedGreater;
    case nsINavHistoryQueryOptions::SORT_BY_LASTMODIFIED_ASCENDING:
      return &SortComparison_LastModifiedLess;
    case nsINavHistoryQueryOptions::SORT_BY_LASTMODIFIED_DESCENDING:
      return &SortComparison_LastModifiedGreater;
    case nsINavHistoryQueryOptions::SORT_BY_TAGS_ASCENDING:
      return &SortComparison_TagsLess;
    case nsINavHistoryQueryOptions::SORT_BY_TAGS_DESCENDING:
      return &SortComparison_TagsGreater;
    case nsINavHistoryQueryOptions::SORT_BY_FRECENCY_ASCENDING:
      return &SortComparison_FrecencyLess;
    case nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING:
      return &SortComparison_FrecencyGreater;
    default:
      NS_NOTREACHED("Bad sorting type");
      return nsnull;
  }
}









void
nsNavHistoryContainerResultNode::RecursiveSort(
    const char* aData, SortComparator aComparator)
{
  void* data = const_cast<void*>(static_cast<const void*>(aData));

  mChildren.Sort(aComparator, data);
  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->IsContainer())
      mChildren[i]->GetAsContainer()->RecursiveSort(aData, aComparator);
  }
}






PRUint32
nsNavHistoryContainerResultNode::FindInsertionPoint(
    nsNavHistoryResultNode* aNode, SortComparator aComparator,
    const char* aData, PRBool* aItemExists)
{
  if (aItemExists)
    (*aItemExists) = PR_FALSE;

  if (mChildren.Count() == 0)
    return 0;

  void* data = const_cast<void*>(static_cast<const void*>(aData));

  
  
  PRInt32 res;
  res = aComparator(aNode, mChildren[0], data);
  if (res <= 0) {
    if (aItemExists && res == 0)
      (*aItemExists) = PR_TRUE;
    return 0;
  }
  res = aComparator(aNode, mChildren[mChildren.Count() - 1], data);
  if (res >= 0) {
    if (aItemExists && res == 0)
      (*aItemExists) = PR_TRUE;
    return mChildren.Count();
  }

  PRUint32 beginRange = 0; 
  PRUint32 endRange = mChildren.Count(); 
  while (1) {
    if (beginRange == endRange)
      return endRange;
    PRUint32 center = beginRange + (endRange - beginRange) / 2;
    PRInt32 res = aComparator(aNode, mChildren[center], data);
    if (res <= 0) {
      endRange = center; 
      if (aItemExists && res == 0)
        (*aItemExists) = PR_TRUE;
    }
    else {
      beginRange = center + 1; 
    }
  }
}









PRBool
nsNavHistoryContainerResultNode::DoesChildNeedResorting(PRUint32 aIndex,
    SortComparator aComparator, const char* aData)
{
  NS_ASSERTION(aIndex >= 0 && aIndex < PRUint32(mChildren.Count()),
               "Input index out of range");
  if (mChildren.Count() == 1)
    return PR_FALSE;

  void* data = const_cast<void*>(static_cast<const void*>(aData));

  if (aIndex > 0) {
    
    if (aComparator(mChildren[aIndex - 1], mChildren[aIndex], data) > 0)
      return PR_TRUE;
  }
  if (aIndex < PRUint32(mChildren.Count()) - 1) {
    
    if (aComparator(mChildren[aIndex], mChildren[aIndex + 1], data) > 0)
      return PR_TRUE;
  }
  return PR_FALSE;
}



PRInt32 nsNavHistoryContainerResultNode::SortComparison_StringLess(
    const nsAString& a, const nsAString& b) {

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, 0);
  nsICollation* collation = history->GetCollation();
  NS_ENSURE_TRUE(collation, 0);

  PRInt32 res = 0;
  collation->CompareString(nsICollation::kCollationCaseInSensitive, a, b, &res);
  return res;
}







PRInt32 nsNavHistoryContainerResultNode::SortComparison_Bookmark(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return a->mBookmarkIndex - b->mBookmarkIndex;
}









PRInt32 nsNavHistoryContainerResultNode::SortComparison_TitleLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRUint32 aType;
  a->GetType(&aType);

  PRInt32 value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                            NS_ConvertUTF8toUTF16(b->mTitle));
  if (value == 0) {
    
    if (a->IsURI()) {
      value = a->mURI.Compare(b->mURI.get());
    }
    if (value == 0) {
      
      value = ComparePRTime(a->mTime, b->mTime);
      if (value == 0)
        value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
    }
  }
  return value;
}
PRInt32 nsNavHistoryContainerResultNode::SortComparison_TitleGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_TitleLess(a, b, closure);
}





PRInt32 nsNavHistoryContainerResultNode::SortComparison_DateLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = ComparePRTime(a->mTime, b->mTime);
  if (value == 0) {
    value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                      NS_ConvertUTF8toUTF16(b->mTitle));
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
PRInt32 nsNavHistoryContainerResultNode::SortComparison_DateGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_DateLess(a, b, closure);
}


PRInt32 nsNavHistoryContainerResultNode::SortComparison_DateAddedLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = ComparePRTime(a->mDateAdded, b->mDateAdded);
  if (value == 0) {
    value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                      NS_ConvertUTF8toUTF16(b->mTitle));
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
PRInt32 nsNavHistoryContainerResultNode::SortComparison_DateAddedGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_DateAddedLess(a, b, closure);
}


PRInt32 nsNavHistoryContainerResultNode::SortComparison_LastModifiedLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = ComparePRTime(a->mLastModified, b->mLastModified);
  if (value == 0) {
    value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                      NS_ConvertUTF8toUTF16(b->mTitle));
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
PRInt32 nsNavHistoryContainerResultNode::SortComparison_LastModifiedGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_LastModifiedLess(a, b, closure);
}






PRInt32 nsNavHistoryContainerResultNode::SortComparison_URILess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value;
  if (a->IsURI() && b->IsURI()) {
    
    value = a->mURI.Compare(b->mURI.get());
  } else {
    
    value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                      NS_ConvertUTF8toUTF16(b->mTitle));
  }

  if (value == 0) {
    value = ComparePRTime(a->mTime, b->mTime);
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
PRInt32 nsNavHistoryContainerResultNode::SortComparison_URIGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_URILess(a, b, closure);
}


PRInt32 nsNavHistoryContainerResultNode::SortComparison_KeywordLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = 0;
  if (a->mItemId != -1 || b->mItemId != -1) {
    
    nsAutoString keywordA, keywordB;
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, 0);

    nsresult rv;
    if (a->mItemId != -1) {
      rv = bookmarks->GetKeywordForBookmark(a->mItemId, keywordA);
      NS_ENSURE_SUCCESS(rv, 0);
    }
    if (b->mItemId != -1) {
      rv = bookmarks->GetKeywordForBookmark(b->mItemId, keywordB);
      NS_ENSURE_SUCCESS(rv, 0);
    }

    value = SortComparison_StringLess(keywordA, keywordB);
  }

  
  if (value == 0)
    value = SortComparison_TitleLess(a, b, closure);

  return value;
}

PRInt32 nsNavHistoryContainerResultNode::SortComparison_KeywordGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_KeywordLess(a, b, closure);
}

PRInt32 nsNavHistoryContainerResultNode::SortComparison_AnnotationLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  nsCAutoString annoName(static_cast<char*>(closure));
  NS_ENSURE_TRUE(!annoName.IsEmpty(), 0);
  
  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, 0);

  PRBool a_itemAnno = PR_FALSE;
  PRBool b_itemAnno = PR_FALSE;

  
  nsCOMPtr<nsIURI> a_uri, b_uri;
  if (a->mItemId != -1) {
    a_itemAnno = PR_TRUE;
  } else {
    nsCAutoString spec;
    if (NS_SUCCEEDED(a->GetUri(spec)))
      NS_NewURI(getter_AddRefs(a_uri), spec);
    NS_ENSURE_TRUE(a_uri, 0);
  }

  if (b->mItemId != -1) {
    b_itemAnno = PR_TRUE;
  } else {
    nsCAutoString spec;
    if (NS_SUCCEEDED(b->GetUri(spec)))
      NS_NewURI(getter_AddRefs(b_uri), spec);
    NS_ENSURE_TRUE(b_uri, 0);
  }

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, 0);

  PRBool a_hasAnno, b_hasAnno;
  if (a_itemAnno) {
    NS_ENSURE_SUCCESS(annosvc->ItemHasAnnotation(a->mItemId, annoName,
                                                 &a_hasAnno), 0);
  } else {
    NS_ENSURE_SUCCESS(annosvc->PageHasAnnotation(a_uri, annoName,
                                                 &a_hasAnno), 0);
  }
  if (b_itemAnno) {
    NS_ENSURE_SUCCESS(annosvc->ItemHasAnnotation(b->mItemId, annoName,
                                                 &b_hasAnno), 0);
  } else {
    NS_ENSURE_SUCCESS(annosvc->PageHasAnnotation(b_uri, annoName,
                                                 &b_hasAnno), 0);    
  }

  PRInt32 value = 0;
  if (a_hasAnno || b_hasAnno) {
    PRUint16 annoType;
    if (a_hasAnno) {
      if (a_itemAnno) {
        NS_ENSURE_SUCCESS(annosvc->GetItemAnnotationType(a->mItemId,
                                                         annoName,
                                                         &annoType), 0);
      } else {
        NS_ENSURE_SUCCESS(annosvc->GetPageAnnotationType(a_uri, annoName,
                                                         &annoType), 0);
      }
    }
    if (b_hasAnno) {
      PRUint16 b_type;
      if (b_itemAnno) {
        NS_ENSURE_SUCCESS(annosvc->GetItemAnnotationType(b->mItemId,
                                                         annoName,
                                                         &b_type), 0);
      } else {
        NS_ENSURE_SUCCESS(annosvc->GetPageAnnotationType(b_uri, annoName,
                                                         &b_type), 0);
      }
      
      
      if (a_hasAnno && b_type != annoType)
        return 0;
      annoType = b_type;
    }

#define GET_ANNOTATIONS_VALUES(METHOD_ITEM, METHOD_PAGE, A_VAL, B_VAL)        \
        if (a_hasAnno) {                                                      \
          if (a_itemAnno) {                                                   \
            NS_ENSURE_SUCCESS(annosvc->METHOD_ITEM(a->mItemId, annoName,      \
                                                   A_VAL), 0);                \
          } else {                                                            \
            NS_ENSURE_SUCCESS(annosvc->METHOD_PAGE(a_uri, annoName,           \
                                                   A_VAL), 0);                \
          }                                                                   \
        }                                                                     \
        if (b_hasAnno) {                                                      \
          if (b_itemAnno) {                                                   \
            NS_ENSURE_SUCCESS(annosvc->METHOD_ITEM(b->mItemId, annoName,      \
                                                   B_VAL), 0);                \
          } else {                                                            \
            NS_ENSURE_SUCCESS(annosvc->METHOD_PAGE(b_uri, annoName,           \
                                                   B_VAL), 0);                \
          }                                                                   \
        }

    
    if (annoType != nsIAnnotationService::TYPE_BINARY) {
      if (annoType == nsIAnnotationService::TYPE_STRING) {
        nsAutoString a_val, b_val;
        GET_ANNOTATIONS_VALUES(GetItemAnnotationString,
                               GetPageAnnotationString, a_val, b_val);
        value = SortComparison_StringLess(a_val, b_val);
      }
      else if (annoType == nsIAnnotationService::TYPE_INT32) {
        PRInt32 a_val = 0, b_val = 0;
        GET_ANNOTATIONS_VALUES(GetItemAnnotationInt32,
                               GetPageAnnotationInt32, &a_val, &b_val);
        value = (a_val < b_val) ? -1 : (a_val > b_val) ? 1 : 0;
      }
      else if (annoType == nsIAnnotationService::TYPE_INT64) {
        PRInt64 a_val = 0, b_val = 0;
        GET_ANNOTATIONS_VALUES(GetItemAnnotationInt64,
                               GetPageAnnotationInt64, &a_val, &b_val);
        value = (a_val < b_val) ? -1 : (a_val > b_val) ? 1 : 0;
      }
      else if (annoType == nsIAnnotationService::TYPE_DOUBLE) {
        double a_val = 0, b_val = 0;
        GET_ANNOTATIONS_VALUES(GetItemAnnotationDouble,
                               GetPageAnnotationDouble, &a_val, &b_val);
        value = (a_val < b_val) ? -1 : (a_val > b_val) ? 1 : 0;
      }
    }
  }

  
  
  
  if (value == 0)
    return SortComparison_TitleLess(a, b, nsnull);

  return value;
}
PRInt32 nsNavHistoryContainerResultNode::SortComparison_AnnotationGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_AnnotationLess(a, b, closure);
}




PRInt32 nsNavHistoryContainerResultNode::SortComparison_VisitCountLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = CompareIntegers(a->mAccessCount, b->mAccessCount);
  if (value == 0) {
    value = ComparePRTime(a->mTime, b->mTime);
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
PRInt32 nsNavHistoryContainerResultNode::SortComparison_VisitCountGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_VisitCountLess(a, b, closure);
}


PRInt32 nsNavHistoryContainerResultNode::SortComparison_TagsLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = 0;
  nsAutoString aTags, bTags;

  nsresult rv = a->GetTags(aTags);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = b->GetTags(bTags);
  NS_ENSURE_SUCCESS(rv, 0);

  value = SortComparison_StringLess(aTags, bTags);

  
  if (value == 0)
    value = SortComparison_TitleLess(a, b, closure);

  return value;
}

PRInt32 nsNavHistoryContainerResultNode::SortComparison_TagsGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_TagsLess(a, b, closure);
}




PRInt32
nsNavHistoryContainerResultNode::SortComparison_FrecencyLess(
  nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure
)
{
  PRInt32 value = CompareIntegers(a->mFrecency, b->mFrecency);
  if (value == 0) {
    value = ComparePRTime(a->mTime, b->mTime);
    if (value == 0) {
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
    }
  }
  return value;
}
PRInt32
nsNavHistoryContainerResultNode::SortComparison_FrecencyGreater(
  nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure
)
{
  return -nsNavHistoryContainerResultNode::SortComparison_FrecencyLess(a, b, closure);
}







nsNavHistoryResultNode*
nsNavHistoryContainerResultNode::FindChildURI(const nsACString& aSpec,
    PRUint32* aNodeIndex)
{
  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->IsURI()) {
      if (aSpec.Equals(mChildren[i]->mURI)) {
        *aNodeIndex = i;
        return mChildren[i];
      }
    }
  }
  return nsnull;
}









nsNavHistoryContainerResultNode*
nsNavHistoryContainerResultNode::FindChildContainerByName(
    const nsACString& aTitle, PRUint32* aNodeIndex)
{
  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->IsContainer()) {
      nsNavHistoryContainerResultNode* container =
          mChildren[i]->GetAsContainer();
      if (container->mTitle.Equals(aTitle)) {
        *aNodeIndex = i;
        return container;
      }
    }
  }
  return nsnull;
}












nsresult
nsNavHistoryContainerResultNode::InsertChildAt(nsNavHistoryResultNode* aNode,
                                               PRInt32 aIndex,
                                               PRBool aIsTemporary)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  aNode->mParent = this;
  aNode->mIndentLevel = mIndentLevel + 1;
  if (!aIsTemporary && aNode->IsContainer()) {
    
    nsNavHistoryContainerResultNode* container = aNode->GetAsContainer();
    container->mResult = result;
    container->FillStats();
  }

  if (!mChildren.InsertObjectAt(aNode, aIndex))
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (!aIsTemporary) {
    mAccessCount += aNode->mAccessCount;
    if (mTime < aNode->mTime)
      mTime = aNode->mTime;
    if (!mParent || mParent->AreChildrenVisible()) {
      NOTIFY_RESULT_OBSERVERS(result,
                              NodeHistoryDetailsChanged(TO_ICONTAINER(this),
                                                        mTime,
                                                        mAccessCount));
    }

    nsresult rv = ReverseUpdateStats(aNode->mAccessCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  if (AreChildrenVisible())
    NOTIFY_RESULT_OBSERVERS(result, NodeInserted(this, aNode, aIndex));

  return NS_OK;
}






nsresult
nsNavHistoryContainerResultNode::InsertSortedChild(
    nsNavHistoryResultNode* aNode, 
    PRBool aIsTemporary, PRBool aIgnoreDuplicates)
{

  if (mChildren.Count() == 0)
    return InsertChildAt(aNode, 0, aIsTemporary);

  SortComparator comparator = GetSortingComparator(GetSortType());
  if (comparator) {
    
    
    
    
    
    
    if (!aIsTemporary && aNode->IsContainer()) {
      
      nsNavHistoryContainerResultNode* container = aNode->GetAsContainer();
      container->mResult = mResult;
      container->FillStats();
    }

    nsCAutoString sortingAnnotation;
    GetSortingAnnotation(sortingAnnotation);
    PRBool itemExists;
    PRUint32 position = FindInsertionPoint(aNode, comparator, 
                                           sortingAnnotation.get(), 
                                           &itemExists);
    if (aIgnoreDuplicates && itemExists)
      return NS_OK;

    return InsertChildAt(aNode, position, aIsTemporary);
  }
  return InsertChildAt(aNode, mChildren.Count(), aIsTemporary);
}








PRBool
nsNavHistoryContainerResultNode::EnsureItemPosition(PRUint32 aIndex) {
  NS_ASSERTION(aIndex >= 0 && aIndex < (PRUint32)mChildren.Count(), "Invalid index");
  if (aIndex < 0 || aIndex >= (PRUint32)mChildren.Count())
    return PR_FALSE;

  SortComparator comparator = GetSortingComparator(GetSortType());
  if (!comparator)
    return PR_FALSE;

  nsCAutoString sortAnno;
  GetSortingAnnotation(sortAnno);
  if (!DoesChildNeedResorting(aIndex, comparator, sortAnno.get()))
    return PR_FALSE;

  nsRefPtr<nsNavHistoryResultNode> node(mChildren[aIndex]);
  mChildren.RemoveObjectAt(aIndex);

  PRUint32 newIndex = FindInsertionPoint(
                          node, comparator,sortAnno.get(), nsnull);
  mChildren.InsertObjectAt(node.get(), newIndex);

  if (AreChildrenVisible()) {
    nsNavHistoryResult* result = GetResult();
    NOTIFY_RESULT_OBSERVERS(result,
                            NodeMoved(node, this, aIndex, this, newIndex));
  }

  return PR_TRUE;
}














void
nsNavHistoryContainerResultNode::MergeResults(
    nsCOMArray<nsNavHistoryResultNode>* aAddition)
{
  
  
  
  for (PRUint32 i = 0; i < PRUint32(aAddition->Count()); ++i) {
    nsNavHistoryResultNode* curAddition = (*aAddition)[i];
    if (curAddition->IsContainer()) {
      PRUint32 containerIndex;
      nsNavHistoryContainerResultNode* container =
        FindChildContainerByName(curAddition->mTitle, &containerIndex);
      if (container) {
        
        container->MergeResults(&curAddition->GetAsContainer()->mChildren);
      } else {
        
        InsertSortedChild(curAddition);
      }
    } else {
      if (curAddition->IsVisit()) {
        
        InsertSortedChild(curAddition);
      } else {
        
        PRUint32 oldIndex;
        nsNavHistoryResultNode* oldNode =
          FindChildURI(curAddition->mURI, &oldIndex);
        if (oldNode) {
          
          
          
          
          if (mParent)
            ReplaceChildURIAt(oldIndex, curAddition);
          else {
            RemoveChildAt(oldIndex, PR_TRUE);
            InsertSortedChild(curAddition, PR_TRUE);
          }
        }
        else
          InsertSortedChild(curAddition);
      }
    }
  }
}










nsresult
nsNavHistoryContainerResultNode::ReplaceChildURIAt(PRUint32 aIndex,
    nsNavHistoryResultNode* aNode)
{
  NS_ASSERTION(aIndex < PRUint32(mChildren.Count()),
               "Invalid index for replacement");
  NS_ASSERTION(mChildren[aIndex]->IsURI(),
               "Can not use ReplaceChildAt for a node of another type");
  NS_ASSERTION(mChildren[aIndex]->mURI.Equals(aNode->mURI),
               "We must replace a URI with an updated one of the same");

  aNode->mParent = this;
  aNode->mIndentLevel = mIndentLevel + 1;

  
  PRUint32 accessCountChange = aNode->mAccessCount - mChildren[aIndex]->mAccessCount;
  if (accessCountChange != 0 || mChildren[aIndex]->mTime != aNode->mTime) {
    NS_ASSERTION(aNode->mAccessCount >= mChildren[aIndex]->mAccessCount,
                 "Replacing a node with one back in time or some nonmatching node");

    mAccessCount += accessCountChange;
    if (mTime < aNode->mTime)
      mTime = aNode->mTime;
    nsresult rv = ReverseUpdateStats(accessCountChange);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  nsRefPtr<nsNavHistoryResultNode> oldItem = mChildren[aIndex];
  if (!mChildren.ReplaceObjectAt(aNode, aIndex))
    return NS_ERROR_FAILURE;

  if (AreChildrenVisible()) {
    nsNavHistoryResult* result = GetResult();
    NOTIFY_RESULT_OBSERVERS(result,
                            NodeReplaced(this, oldItem, aNode, aIndex));
  }

  mChildren[aIndex]->OnRemoving();
  return NS_OK;
}












nsresult
nsNavHistoryContainerResultNode::RemoveChildAt(PRInt32 aIndex,
                                               PRBool aIsTemporary)
{
  NS_ASSERTION(aIndex >= 0 && aIndex < mChildren.Count(), "Invalid index");

  
  nsRefPtr<nsNavHistoryResultNode> oldNode = mChildren[aIndex];

  
  PRUint32 oldAccessCount = 0;
  if (!aIsTemporary) {
    oldAccessCount = mAccessCount;
    mAccessCount -= mChildren[aIndex]->mAccessCount;
    NS_ASSERTION(mAccessCount >= 0, "Invalid access count while updating!");
  }

  
  mChildren.RemoveObjectAt(aIndex);
  if (AreChildrenVisible()) {
    nsNavHistoryResult* result = GetResult();
    NOTIFY_RESULT_OBSERVERS(result,
                            NodeRemoved(this, oldNode, aIndex));
  }

  if (!aIsTemporary) {
    nsresult rv = ReverseUpdateStats(mAccessCount - oldAccessCount);
    NS_ENSURE_SUCCESS(rv, rv);
    oldNode->OnRemoving();
  }
  return NS_OK;
}










void
nsNavHistoryContainerResultNode::RecursiveFindURIs(PRBool aOnlyOne,
    nsNavHistoryContainerResultNode* aContainer, const nsCString& aSpec,
    nsCOMArray<nsNavHistoryResultNode>* aMatches)
{
  for (PRInt32 child = 0; child < aContainer->mChildren.Count(); ++child) {
    PRUint32 type;
    aContainer->mChildren[child]->GetType(&type);
    if (nsNavHistoryResultNode::IsTypeURI(type)) {
      
      nsNavHistoryResultNode* uriNode = aContainer->mChildren[child];
      if (uriNode->mURI.Equals(aSpec)) {
        
        aMatches->AppendObject(uriNode);
        if (aOnlyOne)
          return;
      }
    }
  }
}









nsresult
nsNavHistoryContainerResultNode::UpdateURIs(PRBool aRecursive, PRBool aOnlyOne,
    PRBool aUpdateSort, const nsCString& aSpec,
    nsresult (*aCallback)(nsNavHistoryResultNode*,void*, nsNavHistoryResult*), void* aClosure)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  
  
  nsCOMArray<nsNavHistoryResultNode> matches;

  if (aRecursive) {
    RecursiveFindURIs(aOnlyOne, this, aSpec, &matches);
  } else if (aOnlyOne) {
    PRUint32 nodeIndex;
    nsNavHistoryResultNode* node = FindChildURI(aSpec, &nodeIndex);
    if (node)
      matches.AppendObject(node);
  } else {
    NS_NOTREACHED("UpdateURIs does not handle nonrecursive updates of multiple items.");
    
    
    
    return NS_ERROR_FAILURE;
  }
  if (matches.Count() == 0)
    return NS_OK;

  
  
  
  
  for (PRInt32 i = 0; i < matches.Count(); ++i)
  {
    nsNavHistoryResultNode* node = matches[i];
    nsNavHistoryContainerResultNode* parent = node->mParent;
    if (!parent) {
      NS_NOTREACHED("All URI nodes being updated must have parents");
      continue;
    }

    PRUint32 oldAccessCount = node->mAccessCount;
    PRTime oldTime = node->mTime;
    aCallback(node, aClosure, result);

    if (oldAccessCount != node->mAccessCount || oldTime != node->mTime) {
      parent->mAccessCount += node->mAccessCount - oldAccessCount;
      if (node->mTime > parent->mTime)
        parent->mTime = node->mTime;
      if (parent->AreChildrenVisible()) {
          NOTIFY_RESULT_OBSERVERS(result,
                                  NodeHistoryDetailsChanged(
                                    TO_ICONTAINER(parent),
                                    parent->mTime,
                                    parent->mAccessCount));
      }
      nsresult rv = parent->ReverseUpdateStats(node->mAccessCount - oldAccessCount);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (aUpdateSort) {
      PRInt32 childIndex = parent->FindChild(node);
      NS_ASSERTION(childIndex >= 0, "Could not find child we just got a reference to");
      if (childIndex >= 0)
        parent->EnsureItemPosition(childIndex);
    }
  }

  return NS_OK;
}








static nsresult setTitleCallback(
    nsNavHistoryResultNode* aNode, void* aClosure,
    nsNavHistoryResult* aResult)
{
  const nsACString* newTitle = reinterpret_cast<nsACString*>(aClosure);
  aNode->mTitle = *newTitle;

  if (aResult && (!aNode->mParent || aNode->mParent->AreChildrenVisible()))
    NOTIFY_RESULT_OBSERVERS(aResult, NodeTitleChanged(aNode, *newTitle));

  return NS_OK;
}
nsresult
nsNavHistoryContainerResultNode::ChangeTitles(nsIURI* aURI,
                                              const nsACString& aNewTitle,
                                              PRBool aRecursive,
                                              PRBool aOnlyOne)
{
  
  nsCAutoString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  PRUint16 sortType = GetSortType();
  PRBool updateSorting =
    (sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING ||
     sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING);

  rv = UpdateURIs(aRecursive, aOnlyOne, updateSorting, uriString,
                 setTitleCallback,
                 const_cast<void*>(reinterpret_cast<const void*>(&aNewTitle)));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetHasChildren(PRBool *aHasChildren)
{
  *aHasChildren = (mChildren.Count() > 0);
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChildCount(PRUint32* aChildCount)
{
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;
  *aChildCount = mChildren.Count();
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChild(PRUint32 aIndex,
                                          nsINavHistoryResultNode** _retval)
{
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;
  if (aIndex >= PRUint32(mChildren.Count()))
    return NS_ERROR_INVALID_ARG;
  NS_ADDREF(*_retval = mChildren[aIndex]);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChildIndex(nsINavHistoryResultNode* aNode,
                                               PRUint32* _retval)
{
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;

  PRInt32 nodeIndex = FindChild(static_cast<nsNavHistoryResultNode*>(aNode));
  if (nodeIndex == -1)
    return NS_ERROR_INVALID_ARG;

  *_retval = nodeIndex;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::FindNodeByDetails(const nsACString& aURIString,
                                                   PRTime aTime,
                                                   PRInt64 aItemId,
                                                   PRBool aRecursive,
                                                   nsINavHistoryResultNode** _retval) {
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;

  *_retval = nsnull;
  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->mURI.Equals(aURIString) &&
        mChildren[i]->mTime == aTime &&
        mChildren[i]->mItemId == aItemId) {
      *_retval = mChildren[i];
      break;
    }

    if (aRecursive && mChildren[i]->IsContainer()) {
      nsNavHistoryContainerResultNode* asContainer =
        mChildren[i]->GetAsContainer();
      if (asContainer->mExpanded) {
        nsresult rv = asContainer->FindNodeByDetails(aURIString, aTime,
                                                     aItemId,
                                                     aRecursive,
                                                     _retval);
                                                      
        if (NS_SUCCEEDED(rv) && _retval)
          break;
      }
    }
  }
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChildrenReadOnly(PRBool *aChildrenReadOnly)
{
  *aChildrenReadOnly = mChildrenReadOnly;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetDynamicContainerType(
    nsACString& aDynamicContainerType)
{
  aDynamicContainerType = mDynamicContainerType;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendURINode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, nsINavHistoryResultNode** _retval)
{
  *_retval = nsnull;
  if (!IsDynamicContainer())
    return NS_ERROR_INVALID_ARG; 

  
  
  if ((mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
      (mParent && mParent->mOptions->ExcludeItems()))
    return NS_OK;

  nsRefPtr<nsNavHistoryResultNode> result =
      new nsNavHistoryResultNode(aURI, aTitle, aAccessCount, aTime, aIconURI);
  NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = InsertChildAt(result, mChildren.Count());
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = result);
  return NS_OK;
}


#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendVisitNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, PRInt64 aSession,
    nsINavHistoryVisitResultNode** _retval)
{
  *_retval = nsnull;
  if (!IsDynamicContainer())
    return NS_ERROR_INVALID_ARG; 

  nsRefPtr<nsNavHistoryVisitResultNode> result =
      new nsNavHistoryVisitResultNode(aURI, aTitle, aAccessCount, aTime,
                                      aIconURI, aSession);
  NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

  
  if (! mChildren.AppendObject(result))
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval = result);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendFullVisitNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, PRInt64 aSession,
    PRInt64 aVisitId, PRInt64 aReferringVisitId, PRInt32 aTransitionType,
    nsINavHistoryFullVisitResultNode** _retval)
{
  *_retval = nsnull;
  if (!IsDynamicContainer())
    return NS_ERROR_INVALID_ARG; 

  nsRefPtr<nsNavHistoryFullVisitResultNode> result =
      new nsNavHistoryFullVisitResultNode(aURI, aTitle, aAccessCount, aTime,
                                          aIconURI, aSession, aVisitId,
                                          aReferringVisitId, aTransitionType);
  NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

  
  if (! mChildren.AppendObject(result))
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval = result);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendContainerNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    PRUint32 aContainerType, const nsACString& aDynamicContainerType,
    nsINavHistoryContainerResultNode** _retval)
{
  *_retval = nsnull;
  if (!IsDynamicContainer())
    return NS_ERROR_INVALID_ARG; 
  if (! IsTypeContainer(aContainerType) || IsTypeFolder(aContainerType) ||
      IsTypeQuery(aContainerType))
    return NS_ERROR_INVALID_ARG; 
  if (aContainerType == nsNavHistoryResultNode::RESULT_TYPE_DYNAMIC_CONTAINER &&
      aRemoteContainerType.IsEmpty())
    return NS_ERROR_INVALID_ARG; 
  if (aContainerType != nsNavHistoryResultNode::RESULT_TYPE_DYNAMIC_CONTAINER &&
      ! aDynamicContainerType.IsEmpty())
    return NS_ERROR_INVALID_ARG; 

  nsRefPtr<nsNavHistoryContainerResultNode> result =
      new nsNavHistoryContainerResultNode(EmptyCString(), aTitle, aIconURI,
                                          aContainerType, PR_TRUE,
                                          aDynamicContainerType);
  NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

  
  if (! mChildren.AppendObject(result))
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval = result);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendQueryNode(
    const nsACString& aQueryURI, const nsACString& aTitle,
    const nsACString& aIconURI, nsINavHistoryQueryResultNode** _retval)
{
  *_retval = nsnull;
  if (!IsDynamicContainer())
    return NS_ERROR_INVALID_ARG; 

  nsRefPtr<nsNavHistoryQueryResultNode> result =
      new nsNavHistoryQueryResultNode(aQueryURI, aTitle, aIconURI);
  NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

  
  if (! mChildren.AppendObject(result))
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval = result);
  return NS_OK;
}
#endif


NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendFolderNode(
    PRInt64 aFolderId, nsINavHistoryContainerResultNode** _retval)
{
  *_retval = nsnull;
  if (!IsDynamicContainer())
    return NS_ERROR_INVALID_ARG; 

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  
  nsRefPtr<nsNavHistoryResultNode> result;
  nsresult rv = bookmarks->ResultNodeForContainer(aFolderId,
                                                  GetGeneratingOptions(),
                                                  getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = InsertChildAt(result, mChildren.Count());
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = result->GetAsContainer());
  return NS_OK;
}


#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::ClearContents()
{
  if (!IsDynamicContainer())
    return NS_ERROR_INVALID_ARG; 

  
  
  
  
  mChildren.Clear();

  PRUint32 oldAccessCount = mAccessCount;
  mAccessCount = 0;
  mTime = 0;
  rv = ReverseUpdateStats(-PRInt32(oldAccessCount));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}
#endif



















NS_IMPL_ISUPPORTS_INHERITED1(nsNavHistoryQueryResultNode,
                             nsNavHistoryContainerResultNode,
                             nsINavHistoryQueryResultNode)

nsNavHistoryQueryResultNode::nsNavHistoryQueryResultNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    const nsACString& aQueryURI) :
  nsNavHistoryContainerResultNode(aQueryURI, aTitle, aIconURI,
                                  nsNavHistoryResultNode::RESULT_TYPE_QUERY,
                                  PR_TRUE, EmptyCString(), nsnull),
  mLiveUpdate(QUERYUPDATE_COMPLEX_WITH_BOOKMARKS),
  mHasSearchTerms(PR_FALSE),
  mContentsValid(PR_FALSE)
{
}

nsNavHistoryQueryResultNode::nsNavHistoryQueryResultNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, aIconURI,
                                  nsNavHistoryResultNode::RESULT_TYPE_QUERY,
                                  PR_TRUE, EmptyCString(), aOptions),
  mQueries(aQueries),
  mContentsValid(PR_FALSE)
{
  NS_ASSERTION(aQueries.Count() > 0, "Must have at least one query");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ASSERTION(history, "History service missing");
  if (history) {
    mLiveUpdate = history->GetUpdateRequirements(mQueries, mOptions,
                                                 &mHasSearchTerms);
  }
}

nsNavHistoryQueryResultNode::nsNavHistoryQueryResultNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    PRTime aTime,
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, aTime, aIconURI,
                                  nsNavHistoryResultNode::RESULT_TYPE_QUERY,
                                  PR_TRUE, EmptyCString(), aOptions),
  mQueries(aQueries),
  mContentsValid(PR_FALSE)
{
  NS_ASSERTION(aQueries.Count() > 0, "Must have at least one query");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ASSERTION(history, "History service missing");
  if (history) {
    mLiveUpdate = history->GetUpdateRequirements(mQueries, mOptions,
                                                 &mHasSearchTerms);
  }
}

nsNavHistoryQueryResultNode::~nsNavHistoryQueryResultNode() {
  
  
  if (mResult && mResult->mAllBookmarksObservers.IndexOf(this) !=
                   mResult->mAllBookmarksObservers.NoIndex)
    mResult->RemoveAllBookmarksObserver(this);
  if (mResult && mResult->mHistoryObservers.IndexOf(this) !=
                   mResult->mHistoryObservers.NoIndex)
    mResult->RemoveHistoryObserver(this);
}







PRBool
nsNavHistoryQueryResultNode::CanExpand()
{
  if (IsContainersQuery())
    return PR_TRUE;

  
  if ((mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
      Options()->ExcludeItems())
    return PR_FALSE;

  
  nsNavHistoryQueryOptions* options = GetGeneratingOptions();
  if (options) {
    if (options->ExcludeItems())
      return PR_FALSE;
    if (options->ExpandQueries())
      return PR_TRUE;
  }

  if (mResult && mResult->mRootNode == this)
    return PR_TRUE;

  return PR_FALSE;
}






PRBool
nsNavHistoryQueryResultNode::IsContainersQuery()
{
  PRUint16 resultType = Options()->ResultType();
  return resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
         resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY ||
         resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY ||
         resultType == nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY;
}








void
nsNavHistoryQueryResultNode::OnRemoving()
{
  nsNavHistoryResultNode::OnRemoving();
  ClearChildren(PR_TRUE);
}














nsresult
nsNavHistoryQueryResultNode::OpenContainer()
{
  NS_ASSERTION(!mExpanded, "Container must be closed to open it");
  mExpanded = PR_TRUE;

  nsresult rv;

  if (!CanExpand())
    return NS_OK;
  if (!mContentsValid) {
    rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = NotifyOnStateChange(STATE_CLOSED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetHasChildren(PRBool* aHasChildren)
{
  if (!CanExpand()) {
    *aHasChildren = PR_FALSE;
    return NS_OK;
  }

  PRUint16 resultType = mOptions->ResultType();
  
  if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    mozIStorageConnection *dbConn = history->GetStorageConnection();

    nsCOMPtr<mozIStorageStatement> stmt;
    nsresult rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id FROM moz_bookmarks WHERE parent = :tags_folder LIMIT 1"
    ), getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("tags_folder"),
                               history->GetTagsFolder());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->ExecuteStep(aHasChildren);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  
  if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
      resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY ||
      resultType == nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    return history->GetHasHistoryEntries(aHasChildren);
  }

  
  
  
  

  if (mContentsValid) {
    *aHasChildren = (mChildren.Count() > 0);
    return NS_OK;
  }
  *aHasChildren = PR_TRUE;
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetUri(nsACString& aURI)
{
  nsresult rv = VerifyQueriesSerialized();
  NS_ENSURE_SUCCESS(rv, rv);
  aURI = mURI;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetFolderItemId(PRInt64* aItemId)
{
  *aItemId = mItemId;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetQueries(PRUint32* queryCount,
                                        nsINavHistoryQuery*** queries)
{
  nsresult rv = VerifyQueriesParsed();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mQueries.Count() > 0, "Must have >= 1 query");

  *queries = static_cast<nsINavHistoryQuery**>
                        (nsMemory::Alloc(mQueries.Count() * sizeof(nsINavHistoryQuery*)));
  NS_ENSURE_TRUE(*queries, NS_ERROR_OUT_OF_MEMORY);

  for (PRInt32 i = 0; i < mQueries.Count(); ++i)
    NS_ADDREF((*queries)[i] = mQueries[i]);
  *queryCount = mQueries.Count();
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetQueryOptions(
                                      nsINavHistoryQueryOptions** aQueryOptions)
{
  *aQueryOptions = Options();
  NS_ADDREF(*aQueryOptions);
  return NS_OK;
}




nsNavHistoryQueryOptions*
nsNavHistoryQueryResultNode::Options()
{
  nsresult rv = VerifyQueriesParsed();
  if (NS_FAILED(rv))
    return nsnull;
  NS_ASSERTION(mOptions, "Options invalid, cannot generate from URI");
  return mOptions;
}


nsresult
nsNavHistoryQueryResultNode::VerifyQueriesParsed()
{
  if (mQueries.Count() > 0) {
    NS_ASSERTION(mOptions, "If a result has queries, it also needs options");
    return NS_OK;
  }
  NS_ASSERTION(!mURI.IsEmpty(),
               "Query nodes must have either a URI or query/options");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = history->QueryStringToQueryArray(mURI, &mQueries,
                                                 getter_AddRefs(mOptions));
  NS_ENSURE_SUCCESS(rv, rv);

  mLiveUpdate = history->GetUpdateRequirements(mQueries, mOptions,
                                               &mHasSearchTerms);
  return NS_OK;
}


nsresult
nsNavHistoryQueryResultNode::VerifyQueriesSerialized()
{
  if (!mURI.IsEmpty()) {
    return NS_OK;
  }
  NS_ASSERTION(mQueries.Count() > 0 && mOptions,
               "Query nodes must have either a URI or query/options");

  nsTArray<nsINavHistoryQuery*> flatQueries;
  flatQueries.SetCapacity(mQueries.Count());
  for (PRInt32 i = 0; i < mQueries.Count(); ++i)
    flatQueries.AppendElement(static_cast<nsINavHistoryQuery*>
                                         (mQueries.ObjectAt(i)));

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = history->QueriesToQueryString(flatQueries.Elements(),
                                              flatQueries.Length(),
                                              mOptions, mURI);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_STATE(!mURI.IsEmpty());
  return NS_OK;
}


nsresult
nsNavHistoryQueryResultNode::FillChildren()
{
  NS_ASSERTION(!mContentsValid,
               "Don't call FillChildren when contents are valid");
  NS_ASSERTION(mChildren.Count() == 0,
               "We are trying to fill children when there already are some");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = VerifyQueriesParsed();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = history->GetQueryResults(this, mQueries, mOptions, &mChildren);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  FillStats();

  PRUint16 sortType = GetSortType();

  if (mResult->mNeedsToApplySortingMode) {
    
    
    mResult->SetSortingMode(mResult->mSortingMode);
  }
  else if (mOptions->QueryType() != nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY ||
           sortType != nsINavHistoryQueryOptions::SORT_BY_NONE) {
    
    
    
    
    SortComparator comparator = GetSortingComparator(GetSortType());
    if (comparator) {
      nsCAutoString sortingAnnotation;
      GetSortingAnnotation(sortingAnnotation);
      
      
      
      
      
      
      
      
      
      
      
      if (IsContainersQuery() &&
          sortType == mOptions->SortingMode() &&
          (sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING ||
           sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING))
        nsNavHistoryContainerResultNode::RecursiveSort(sortingAnnotation.get(), comparator);
      else
        RecursiveSort(sortingAnnotation.get(), comparator);
    }
  }

  
  
  
  if (!mParent && mOptions->MaxResults()) {
    while ((PRUint32)mChildren.Count() > mOptions->MaxResults())
      mChildren.RemoveObjectAt(mChildren.Count() - 1);
  }

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  if (mOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY ||
      mOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_UNIFIED) {
    
    
    
    if (!mParent || mParent->mOptions->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY) {
      
      result->AddHistoryObserver(this);
    }
  }

  if (mOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS ||
      mOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_UNIFIED ||
      mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS ||
      mHasSearchTerms) {
    
    result->AddAllBookmarksObserver(this);
  }

  mContentsValid = PR_TRUE;
  return NS_OK;
}












void
nsNavHistoryQueryResultNode::ClearChildren(PRBool aUnregister)
{
  for (PRInt32 i = 0; i < mChildren.Count(); ++i)
    mChildren[i]->OnRemoving();
  mChildren.Clear();

  if (aUnregister && mContentsValid) {
    nsNavHistoryResult* result = GetResult();
    if (result) {
      result->RemoveHistoryObserver(this);
      result->RemoveAllBookmarksObserver(this);
    }
  }
  mContentsValid = PR_FALSE;
}






nsresult
nsNavHistoryQueryResultNode::Refresh()
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  if (result->mBatchInProgress) {
    result->requestRefresh(this);
    return NS_OK;
  }

  
  
  
  if (mIndentLevel > -1 && !mParent)
    return NS_OK;

  
  
  
  
  if (!mExpanded ||
      (mParent && mParent->IsQuery() &&
       mParent->GetAsQuery()->IsContainersQuery())) {
    
    ClearChildren(PR_TRUE);
    return NS_OK; 
  }

  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    ClearChildren(PR_TRUE);
  else
    ClearChildren(PR_FALSE);

  
  
  (void)FillChildren();

  NOTIFY_RESULT_OBSERVERS(result, InvalidateContainer(TO_CONTAINER(this)));
  return NS_OK;
}























PRUint16
nsNavHistoryQueryResultNode::GetSortType()
{
  if (mParent)
    return mOptions->SortingMode();
  if (mResult)
    return mResult->mSortingMode;

  NS_NOTREACHED("We should always have a result");
  return nsINavHistoryQueryOptions::SORT_BY_NONE;
}


void
nsNavHistoryQueryResultNode::GetSortingAnnotation(nsACString& aAnnotation) {
  if (mParent) {
    
    mOptions->GetSortingAnnotation(aAnnotation);
  }
  else if (mResult) {
    aAnnotation.Assign(mResult->mSortingAnnotation);
  }
  else
    NS_NOTREACHED("We should always have a result");
}

void
nsNavHistoryQueryResultNode::RecursiveSort(
    const char* aData, SortComparator aComparator)
{
  void* data = const_cast<void*>(static_cast<const void*>(aData));

  if (!IsContainersQuery())
    mChildren.Sort(aComparator, data);

  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->IsContainer())
      mChildren[i]->GetAsContainer()->RecursiveSort(aData, aComparator);
  }
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnBeginUpdateBatch()
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnEndUpdateBatch()
{
  
  
  
  if (mChildren.Count() == 0) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}








NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnVisit(nsIURI* aURI, PRInt64 aVisitId,
                                     PRTime aTime, PRInt64 aSessionId,
                                     PRInt64 aReferringId,
                                     PRUint32 aTransitionType,
                                     PRUint32* aAdded)
{
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  nsRefPtr<nsNavHistoryResultNode> addition;
  switch(mLiveUpdate) {

    case QUERYUPDATE_HOST: {
      
      
      NS_ASSERTION(mQueries.Count() == 1, 
          "Host updated queries can have only one object");
      nsCOMPtr<nsNavHistoryQuery> queryHost = 
          do_QueryInterface(mQueries[0], &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasDomain;
      queryHost->GetHasDomain(&hasDomain);
      if (!hasDomain)
        return NS_OK;

      nsCAutoString host;
      if (NS_FAILED(aURI->GetAsciiHost(host)))
        return NS_OK;

      if (!queryHost->Domain().Equals(host))
        return NS_OK;

    } 
      
    case QUERYUPDATE_TIME: {
      
      
      NS_ASSERTION(mQueries.Count() == 1, 
          "Time updated queries can have only one object");
      nsCOMPtr<nsNavHistoryQuery> query = 
          do_QueryInterface(mQueries[0], &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasIt;
      query->GetHasBeginTime(&hasIt);
      if (hasIt) {
        PRTime beginTime = history->NormalizeTime(query->BeginTimeReference(),
                                                  query->BeginTime());
        if (aTime < beginTime)
          return NS_OK; 
      }
      query->GetHasEndTime(&hasIt);
      if (hasIt) {
        PRTime endTime = history->NormalizeTime(query->EndTimeReference(),
                                                query->EndTime());
        if (aTime > endTime)
          return NS_OK; 
      }
      
      
    }
    case QUERYUPDATE_SIMPLE: {
      
      
      rv = history->VisitIdToResultNode(aVisitId, mOptions,
                                        getter_AddRefs(addition));
      if (NS_FAILED(rv) || !addition ||
          !history->EvaluateQueryForNode(mQueries, mOptions, addition))
        return NS_OK; 
      break;
    }
    case QUERYUPDATE_COMPLEX:
    case QUERYUPDATE_COMPLEX_WITH_BOOKMARKS:
      
      return Refresh();
    default:
      NS_NOTREACHED("Invalid value for mLiveUpdate");
      return Refresh();
  }

  
  
  
  
  
  
  
  
  
  
  

  nsCOMArray<nsNavHistoryResultNode> mergerNode;

  if (!mergerNode.AppendObject(addition))
    return NS_ERROR_OUT_OF_MEMORY;

  MergeResults(&mergerNode);

  if (aAdded)
    ++(*aAdded);

  return NS_OK;
}











NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnTitleChanged(nsIURI* aURI,
                                            const nsAString& aPageTitle)
{
  if (!mExpanded) {
    
    
    
    
    ClearChildren(PR_TRUE);
    return NS_OK; 
  }

  
  NS_ConvertUTF16toUTF8 newTitle(aPageTitle);

  PRBool onlyOneEntry = (mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_URI ||
                         mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS
                         );

  
  if (mHasSearchTerms) {
    
    nsCOMArray<nsNavHistoryResultNode> matches;
    nsCAutoString spec;
    nsresult rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
    RecursiveFindURIs(onlyOneEntry, this, spec, &matches);
    if (matches.Count() == 0) {
      
      
      nsRefPtr<nsNavHistoryResultNode> node;
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      rv = history->URIToResultNode(aURI, mOptions, getter_AddRefs(node));
      NS_ENSURE_SUCCESS(rv, rv);
      if (history->EvaluateQueryForNode(mQueries, mOptions, node)) {
        rv = InsertSortedChild(node, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    for (PRInt32 i = 0; i < matches.Count(); ++i) {
      
      
      
      nsNavHistoryResultNode* node = matches[i];
      
      node->mTitle = newTitle;

      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      if (!history->EvaluateQueryForNode(mQueries, mOptions, node)) {
        nsNavHistoryContainerResultNode* parent = node->mParent;
        
        NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);
        PRInt32 childIndex = parent->FindChild(node);
        NS_ASSERTION(childIndex >= 0, "Child not found in parent");
        parent->RemoveChildAt(childIndex);
      }
    }
  }

  return ChangeTitles(aURI, newTitle, PR_TRUE, onlyOneEntry);
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnBeforeDeleteURI(nsIURI *aURI)
{
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnDeleteURI(nsIURI *aURI)
{
  if (IsContainersQuery()) {
    
    
    
    
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  PRBool onlyOneEntry = (mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_URI ||
                         mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS);
  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsNavHistoryResultNode> matches;
  RecursiveFindURIs(onlyOneEntry, this, spec, &matches);
  for (PRInt32 i = 0; i < matches.Count(); ++i) {
    nsNavHistoryResultNode* node = matches[i];
    nsNavHistoryContainerResultNode* parent = node->mParent;
    
    NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

    PRInt32 childIndex = parent->FindChild(node);
    NS_ASSERTION(childIndex >= 0, "Child not found in parent");
    parent->RemoveChildAt(childIndex);
    if (parent->mChildren.Count() == 0 && parent->IsQuery() &&
        parent->mIndentLevel > -1) {
      
      
      
      matches.AppendObject(parent);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnClearHistory()
{
  nsresult rv = Refresh();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


static nsresult setFaviconCallback(
   nsNavHistoryResultNode* aNode, void* aClosure,
   nsNavHistoryResult* aResult)
{
  const nsCString* newFavicon = static_cast<nsCString*>(aClosure);
  aNode->mFaviconURI = *newFavicon;

  if (aResult && (!aNode->mParent || aNode->mParent->AreChildrenVisible()))
    NOTIFY_RESULT_OBSERVERS(aResult, NodeIconChanged(aNode));

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnPageChanged(nsIURI *aURI, PRUint32 aWhat,
                                           const nsAString &aValue)
{
  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (aWhat) {
    case nsINavHistoryObserver::ATTRIBUTE_FAVICON: {
      NS_ConvertUTF16toUTF8 newFavicon(aValue);
      PRBool onlyOneEntry = (mOptions->ResultType() ==
                             nsINavHistoryQueryOptions::RESULTS_AS_URI ||
                             mOptions->ResultType() ==
                             nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS);
      rv = UpdateURIs(PR_TRUE, onlyOneEntry, PR_FALSE, spec, setFaviconCallback,
                      &newFavicon);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    default:
      NS_WARNING("Unknown page changed notification");
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnDeleteVisits(nsIURI* aURI, PRTime aVisitTime)
{
  NS_PRECONDITION(mOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY,
                  "Bookmarks queries should not get a OnDeleteVisits notification");
  if (aVisitTime == 0) {
    
    
    
    nsresult rv = OnDeleteURI(aURI);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsNavHistoryQueryResultNode::NotifyIfTagsChanged(nsIURI* aURI)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool onlyOneEntry = (mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_URI ||
                         mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS
                         );

  
  nsRefPtr<nsNavHistoryResultNode> node;
  nsNavHistory* history = nsNavHistory::GetHistoryService();

  nsCOMArray<nsNavHistoryResultNode> matches;
  RecursiveFindURIs(onlyOneEntry, this, spec, &matches);

  if (matches.Count() == 0 && mHasSearchTerms && !mRemovingURI) {
    
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->URIToResultNode(aURI, mOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
    if (history->EvaluateQueryForNode(mQueries, mOptions, node)) {
      rv = InsertSortedChild(node, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  for (PRInt32 i = 0; i < matches.Count(); ++i) {
    nsNavHistoryResultNode* node = matches[i];
    
    node->mTags.SetIsVoid(PR_TRUE);
    nsAutoString tags;
    rv = node->GetTags(tags);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    if (mHasSearchTerms &&
        !history->EvaluateQueryForNode(mQueries, mOptions, node)) {
      nsNavHistoryContainerResultNode* parent = node->mParent;
      
      NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);
      PRInt32 childIndex = parent->FindChild(node);
      NS_ASSERTION(childIndex >= 0, "Child not found in parent");
      parent->RemoveChildAt(childIndex);
    }
    else {
      NOTIFY_RESULT_OBSERVERS(result, NodeTagsChanged(node));
    }
  }

  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemAdded(PRInt64 aItemId,
                                         PRInt64 aParentId,
                                         PRInt32 aIndex,
                                         PRUint16 aItemType,
                                         nsIURI* aURI)
{
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK &&
      mLiveUpdate != QUERYUPDATE_SIMPLE &&  mLiveUpdate != QUERYUPDATE_TIME) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnBeforeItemRemoved(PRInt64 aItemId,
                                                 PRUint16 aItemType)
{
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK &&
      (mLiveUpdate == QUERYUPDATE_SIMPLE ||  mLiveUpdate == QUERYUPDATE_TIME)) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
    nsresult rv = bookmarks->GetBookmarkURI(aItemId, getter_AddRefs(mRemovingURI));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemRemoved(PRInt64 aItemId,
                                           PRInt64 aParentId,
                                           PRInt32 aIndex,
                                           PRUint16 aItemType)
{
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK &&
      mLiveUpdate != QUERYUPDATE_SIMPLE && mLiveUpdate != QUERYUPDATE_TIME) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemChanged(PRInt64 aItemId,
                                           const nsACString& aProperty,
                                           PRBool aIsAnnotationProperty,
                                           const nsACString& aNewValue,
                                           PRTime aLastModified,
                                           PRUint16 aItemType)
{
  
  
  
  

  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS) {
    switch (aItemType) {
      case nsINavBookmarksService::TYPE_SEPARATOR:
        
        return NS_OK;
      case nsINavBookmarksService::TYPE_FOLDER:
        
        
        
        if (mOptions->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY)
          return NS_OK;
      default:
        (void)Refresh();
    }
  }
  else {
    
    
    NS_WARN_IF_FALSE(mResult && (mResult->mIsAllBookmarksObserver || mResult->mIsBookmarkFolderObserver),
                     "history observers should not get OnItemChanged, but should get the corresponding history notifications instead");

    
    
    if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK &&
        aProperty.EqualsLiteral("tags")) {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      nsCOMPtr<nsIURI> uri;
      nsresult rv = bookmarks->GetBookmarkURI(aItemId, getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = NotifyIfTagsChanged(uri);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return nsNavHistoryResultNode::OnItemChanged(aItemId, aProperty,
                                               aIsAnnotationProperty,
                                               aNewValue,
                                               aLastModified,
                                               aItemType);
}

NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemVisited(PRInt64 aItemId,
                                           PRInt64 aVisitId, PRTime aTime)
{
  
  
  if (mLiveUpdate != QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    NS_WARN_IF_FALSE(mResult && (mResult->mIsAllBookmarksObserver || mResult->mIsBookmarkFolderObserver),
                     "history observers should not get OnItemVisited, but should get OnVisit instead");
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemMoved(PRInt64 aFolder,
                                         PRInt64 aOldParent, PRInt32 aOldIndex,
                                         PRInt64 aNewParent, PRInt32 aNewIndex,
                                         PRUint16 aItemType)
{
  
  
  
  
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS &&
      aItemType != nsINavBookmarksService::TYPE_SEPARATOR &&
      aOldParent != aNewParent) {
    return Refresh();
  }
  return NS_OK;
}



























NS_IMPL_ISUPPORTS_INHERITED1(nsNavHistoryFolderResultNode,
                             nsNavHistoryContainerResultNode,
                             nsINavHistoryQueryResultNode)

nsNavHistoryFolderResultNode::nsNavHistoryFolderResultNode(
    const nsACString& aTitle, nsNavHistoryQueryOptions* aOptions,
    PRInt64 aFolderId, const nsACString& aDynamicContainerType) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, EmptyCString(),
                                  nsNavHistoryResultNode::RESULT_TYPE_FOLDER,
                                  PR_FALSE, aDynamicContainerType, aOptions),
  mContentsValid(PR_FALSE),
  mQueryItemId(-1),
  mIsRegisteredFolderObserver(PR_FALSE)
{
  mItemId = aFolderId;
}

nsNavHistoryFolderResultNode::~nsNavHistoryFolderResultNode()
{
  if (mIsRegisteredFolderObserver && mResult)
    mResult->RemoveBookmarkFolderObserver(this, mItemId);
}








void
nsNavHistoryFolderResultNode::OnRemoving()
{
  nsNavHistoryResultNode::OnRemoving();
  ClearChildren(PR_TRUE);
}


nsresult
nsNavHistoryFolderResultNode::OpenContainer()
{
  NS_ASSERTION(!mExpanded, "Container must be expanded to close it");
  nsresult rv;

  if (!mContentsValid) {
    rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
    if (IsDynamicContainer()) {
      
      nsCOMPtr<nsIDynamicContainer> svc = do_GetService(mDynamicContainerType.get(), &rv);
      if (NS_SUCCEEDED(rv)) {
        svc->OnContainerNodeOpening(TO_CONTAINER(this), mOptions);
      } else {
        NS_WARNING("Unable to get dynamic container for ");
        NS_WARNING(mDynamicContainerType.get());
      }
    }
  }
  mExpanded = PR_TRUE;

  rv = NotifyOnStateChange(STATE_CLOSED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





nsresult
nsNavHistoryFolderResultNode::OpenContainerAsync()
{
  NS_ASSERTION(!mExpanded, "Container already expanded when opening it");

  
  
  
  if (mContentsValid)
    return OpenContainer();

  nsresult rv = FillChildrenAsync();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NotifyOnStateChange(STATE_CLOSED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetHasChildren(PRBool* aHasChildren)
{
  if (!mContentsValid) {
    nsresult rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  *aHasChildren = (mChildren.Count() > 0);
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetItemId(PRInt64* aItemId)
{
  *aItemId = mQueryItemId == -1 ? mItemId : mQueryItemId;
  return NS_OK;
}











NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetChildrenReadOnly(PRBool *aChildrenReadOnly)
{
  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_UNEXPECTED);
  return bookmarks->GetFolderReadonly(mItemId, aChildrenReadOnly);
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetFolderItemId(PRInt64* aItemId)
{
  *aItemId = mItemId;
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetUri(nsACString& aURI)
{
  if (!mURI.IsEmpty()) {
    aURI = mURI;
    return NS_OK;
  }

  PRUint32 queryCount;
  nsINavHistoryQuery** queries;
  nsresult rv = GetQueries(&queryCount, &queries);
  NS_ENSURE_SUCCESS(rv, rv);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  rv = history->QueriesToQueryString(queries, queryCount, mOptions, aURI);
  for (PRUint32 queryIndex = 0; queryIndex < queryCount; ++queryIndex) {
    NS_RELEASE(queries[queryIndex]);
  }
  nsMemory::Free(queries);
  return rv;
}





NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetQueries(PRUint32* queryCount,
                                         nsINavHistoryQuery*** queries)
{
  
  nsCOMPtr<nsINavHistoryQuery> query;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = history->GetNewQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = query->SetFolders(&mItemId, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  
  *queries = static_cast<nsINavHistoryQuery**>
                        (nsMemory::Alloc(sizeof(nsINavHistoryQuery*)));
  if (!*queries)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF((*queries)[0] = query);
  *queryCount = 1;
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetQueryOptions(
                                      nsINavHistoryQueryOptions** aQueryOptions)
{
  NS_ASSERTION(mOptions, "Options invalid");

  *aQueryOptions = mOptions;
  NS_ADDREF(*aQueryOptions);
  return NS_OK;
}


nsresult
nsNavHistoryFolderResultNode::FillChildren()
{
  NS_ASSERTION(!mContentsValid,
               "Don't call FillChildren when contents are valid");
  NS_ASSERTION(mChildren.Count() == 0,
               "We are trying to fill children when there already are some");

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = bookmarks->QueryFolderChildren(mItemId, mOptions, &mChildren);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  

  return OnChildrenFilled();
}






nsresult
nsNavHistoryFolderResultNode::OnChildrenFilled()
{
  
  
  FillStats();

  if (mResult->mNeedsToApplySortingMode) {
    
    
    mResult->SetSortingMode(mResult->mSortingMode);
  }
  else {
    
    
    SortComparator comparator = GetSortingComparator(GetSortType());
    if (comparator) {
      nsCAutoString sortingAnnotation;
      GetSortingAnnotation(sortingAnnotation);
      RecursiveSort(sortingAnnotation.get(), comparator);
    }
  }

  
  
  
  if (!mParent && mOptions->MaxResults()) {
    while ((PRUint32)mChildren.Count() > mOptions->MaxResults())
      mChildren.RemoveObjectAt(mChildren.Count() - 1);
  }

  
  EnsureRegisteredAsFolderObserver();

  mContentsValid = PR_TRUE;
  return NS_OK;
}






void
nsNavHistoryFolderResultNode::EnsureRegisteredAsFolderObserver()
{
  if (!mIsRegisteredFolderObserver && mResult) {
    mResult->AddBookmarkFolderObserver(this, mItemId);
    mIsRegisteredFolderObserver = PR_TRUE;
  }
}








nsresult
nsNavHistoryFolderResultNode::FillChildrenAsync()
{
  NS_ASSERTION(!mContentsValid, "FillChildrenAsync when contents are valid");
  NS_ASSERTION(mChildren.Count() == 0, "FillChildrenAsync when children exist");

  
  
  mAsyncBookmarkIndex = -1;

  nsNavBookmarks* bmSvc = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bmSvc, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv =
    bmSvc->QueryFolderChildrenAsync(this, mItemId,
                                    getter_AddRefs(mAsyncPendingStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  EnsureRegisteredAsFolderObserver();

  return NS_OK;
}









NS_IMETHODIMP
nsNavHistoryFolderResultNode::HandleResult(mozIStorageResultSet* aResultSet)
{
  NS_ENSURE_ARG_POINTER(aResultSet);

  nsNavBookmarks* bmSvc = nsNavBookmarks::GetBookmarksService();
  if (!bmSvc) {
    CancelAsyncOpen(PR_FALSE);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsCOMPtr<mozIStorageRow> row;
  while (NS_SUCCEEDED(aResultSet->GetNextRow(getter_AddRefs(row))) && row) {
    nsresult rv = bmSvc->ProcessFolderNodeRow(row, mOptions, &mChildren,
                                              mAsyncBookmarkIndex);
    if (NS_FAILED(rv)) {
      CancelAsyncOpen(PR_FALSE);
      return rv;
    }
  }

  return NS_OK;
}









NS_IMETHODIMP
nsNavHistoryFolderResultNode::HandleCompletion(PRUint16 aReason)
{
  if (aReason == mozIStorageStatementCallback::REASON_FINISHED &&
      mAsyncCanceledState == NOT_CANCELED) {
    

    nsresult rv = OnChildrenFilled();
    NS_ENSURE_SUCCESS(rv, rv);

    if (IsDynamicContainer()) {
      
      
      nsCOMPtr<nsIDynamicContainer> svc =
        do_GetService(mDynamicContainerType.get(), &rv);
      if (NS_SUCCEEDED(rv)) {
        svc->OnContainerNodeOpening(
          static_cast<nsNavHistoryContainerResultNode*>(this), mOptions);
      }
      else {
        NS_WARNING("Unable to get dynamic container for ");
        NS_WARNING(mDynamicContainerType.get());
      }
    }

    mExpanded = PR_TRUE;
    mAsyncPendingStmt = nsnull;

    
    rv = NotifyOnStateChange(STATE_LOADING);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  else if (mAsyncCanceledState == CANCELED_RESTART_NEEDED) {
    
    mAsyncCanceledState = NOT_CANCELED;
    ClearChildren(PR_FALSE);
    FillChildrenAsync();
  }

  else {
    
    
    mAsyncCanceledState = NOT_CANCELED;
    ClearChildren(PR_TRUE);
    CloseContainer();
  }

  return NS_OK;
}


void
nsNavHistoryFolderResultNode::ClearChildren(PRBool unregister)
{
  for (PRInt32 i = 0; i < mChildren.Count(); ++i)
    mChildren[i]->OnRemoving();
  mChildren.Clear();

  PRBool needsUnregister = unregister && (mContentsValid || mAsyncPendingStmt);
  if (needsUnregister && mResult && mIsRegisteredFolderObserver) {
    mResult->RemoveBookmarkFolderObserver(this, mItemId);
    mIsRegisteredFolderObserver = PR_FALSE;
  }
  mContentsValid = PR_FALSE;
}






nsresult
nsNavHistoryFolderResultNode::Refresh()
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  if (result->mBatchInProgress) {
    result->requestRefresh(this);
    return NS_OK;
  }

  ClearChildren(PR_TRUE);

  if (!mExpanded) {
    
    return NS_OK;
  }

  
  
  
  
  (void)FillChildren();

  NOTIFY_RESULT_OBSERVERS(result, InvalidateContainer(TO_CONTAINER(this)));
  return NS_OK;
}







PRBool
nsNavHistoryFolderResultNode::StartIncrementalUpdate()
{
  
  
  nsCAutoString parentAnnotationToExclude;
  nsresult rv = mOptions->GetExcludeItemIfParentHasAnnotation(parentAnnotationToExclude);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  if (!mOptions->ExcludeItems() && 
      !mOptions->ExcludeQueries() && 
      !mOptions->ExcludeReadOnlyFolders() && 
      parentAnnotationToExclude.IsEmpty()) {

    
    if (mExpanded || AreChildrenVisible())
      return PR_TRUE;

    nsNavHistoryResult* result = GetResult();
    NS_ENSURE_STATE(result);

    
    
    if (mParent)
      return result->mObservers.Length() > 0;
  }

  
  (void)Refresh();
  return PR_FALSE;
}







void
nsNavHistoryFolderResultNode::ReindexRange(PRInt32 aStartIndex,
                                           PRInt32 aEndIndex,
                                           PRInt32 aDelta)
{
  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    nsNavHistoryResultNode* node = mChildren[i];
    if (node->mBookmarkIndex >= aStartIndex &&
        node->mBookmarkIndex <= aEndIndex)
      node->mBookmarkIndex += aDelta;
  }
}








nsNavHistoryResultNode*
nsNavHistoryFolderResultNode::FindChildById(PRInt64 aItemId,
    PRUint32* aNodeIndex)
{
  for (PRInt32 i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->mItemId == aItemId ||
        (mChildren[i]->IsFolder() &&
         mChildren[i]->GetAsFolder()->mQueryItemId == aItemId)) {
      *aNodeIndex = i;
      return mChildren[i];
    }
  }
  return nsnull;
}





#define RESTART_AND_RETURN_IF_ASYNC_PENDING() \
  if (mAsyncPendingStmt) { \
    CancelAsyncOpen(PR_TRUE); \
    return NS_OK; \
  }


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnBeginUpdateBatch()
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnEndUpdateBatch()
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemAdded(PRInt64 aItemId,
                                          PRInt64 aParentFolder,
                                          PRInt32 aIndex,
                                          PRUint16 aItemType,
                                          nsIURI* aURI)
{
  NS_ASSERTION(aParentFolder == mItemId, "Got wrong bookmark update");

  PRBool excludeItems = (mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
                        (mParent && mParent->mOptions->ExcludeItems()) ||
                        mOptions->ExcludeItems();

  
  
  if (aIndex < 0) {
    NS_NOTREACHED("Invalid index for item adding: <0");
    aIndex = 0;
  }
  else if (aIndex > mChildren.Count()) {
    if (!excludeItems) {
      
      NS_NOTREACHED("Invalid index for item adding: greater than count");
    }
    aIndex = mChildren.Count();
  }

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  
  
  PRBool isQuery = PR_FALSE;
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK) {
    NS_ASSERTION(aURI, "Got a null URI when we are a bookmark?!");
    nsCAutoString itemURISpec;
    rv = aURI->GetSpec(itemURISpec);
    NS_ENSURE_SUCCESS(rv, rv);
    isQuery = IsQueryURI(itemURISpec);
  }

  if (aItemType != nsINavBookmarksService::TYPE_FOLDER &&
      !isQuery && excludeItems) {
    
    
    ReindexRange(aIndex, PR_INT32_MAX, 1);
    return NS_OK;
  }

  if (!StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex, PR_INT32_MAX, 1);

  nsRefPtr<nsNavHistoryResultNode> node;
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->BookmarkIdToResultNode(aItemId, mOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aItemType == nsINavBookmarksService::TYPE_FOLDER ||
           aItemType == nsINavBookmarksService::TYPE_DYNAMIC_CONTAINER) {
    rv = bookmarks->ResultNodeForContainer(aItemId, mOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aItemType == nsINavBookmarksService::TYPE_SEPARATOR) {
    node = new nsNavHistorySeparatorResultNode();
    NS_ENSURE_TRUE(node, NS_ERROR_OUT_OF_MEMORY);
    node->mItemId = aItemId;
  }
  node->mBookmarkIndex = aIndex;

  if (aItemType == nsINavBookmarksService::TYPE_SEPARATOR ||
      GetSortType() == nsINavHistoryQueryOptions::SORT_BY_NONE) {
    
    return InsertChildAt(node, aIndex);
  }
  
  return InsertSortedChild(node, PR_FALSE);
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnBeforeItemRemoved(PRInt64 aItemId,
                                                  PRUint16 aItemType)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemRemoved(PRInt64 aItemId,
                                            PRInt64 aParentFolder,
                                            PRInt32 aIndex,
                                            PRUint16 aItemType)
{
  
  
  
  if (mItemId == aItemId)
    return NS_OK;

  NS_ASSERTION(aParentFolder == mItemId, "Got wrong bookmark update");

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  PRBool excludeItems = (mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
                        (mParent && mParent->mOptions->ExcludeItems()) ||
                        mOptions->ExcludeItems();

  
  
  
  PRUint32 index;
  nsNavHistoryResultNode* node = FindChildById(aItemId, &index);
  if (!node) {
    if (excludeItems)
      return NS_OK;

    NS_NOTREACHED("Removing item we don't have");
    return NS_ERROR_FAILURE;
  }

  if ((node->IsURI() || node->IsSeparator()) && excludeItems) {
    
    
    ReindexRange(aIndex, PR_INT32_MAX, -1);
    return NS_OK;
  }

  if (!StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex + 1, PR_INT32_MAX, -1);

  return RemoveChildAt(index);
}


NS_IMETHODIMP
nsNavHistoryResultNode::OnItemChanged(PRInt64 aItemId,
                                      const nsACString& aProperty,
                                      PRBool aIsAnnotationProperty,
                                      const nsACString& aNewValue,
                                      PRTime aLastModified,
                                      PRUint16 aItemType)
{
  if (aItemId != mItemId)
    return NS_OK;

  mLastModified = aLastModified;

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  PRBool shouldNotify = !mParent || mParent->AreChildrenVisible();

  if (aIsAnnotationProperty) {
    if (shouldNotify)
      NOTIFY_RESULT_OBSERVERS(result, NodeAnnotationChanged(this, aProperty));
  }
  else if (aProperty.EqualsLiteral("title")) {
    
    mTitle = aNewValue;
    if (shouldNotify)
      NOTIFY_RESULT_OBSERVERS(result, NodeTitleChanged(this, mTitle));
  }
  else if (aProperty.EqualsLiteral("uri")) {
    
    mTags.SetIsVoid(PR_TRUE);
    mURI = aNewValue;
    if (shouldNotify)
      NOTIFY_RESULT_OBSERVERS(result, NodeURIChanged(this, mURI));
  }
  else if (aProperty.EqualsLiteral("favicon")) {
    mFaviconURI = aNewValue;
    if (shouldNotify)
      NOTIFY_RESULT_OBSERVERS(result, NodeIconChanged(this));
  }
  else if (aProperty.EqualsLiteral("cleartime")) {
    mTime = 0;
    if (shouldNotify) {
      NOTIFY_RESULT_OBSERVERS(result,
                              NodeHistoryDetailsChanged(this, 0, mAccessCount));
    }
  }
  else if (aProperty.EqualsLiteral("tags")) {
    mTags.SetIsVoid(PR_TRUE);
    if (shouldNotify)
      NOTIFY_RESULT_OBSERVERS(result, NodeTagsChanged(this));
  }
  else if (aProperty.EqualsLiteral("dateAdded")) {
    
    
    mDateAdded = aLastModified;
    if (shouldNotify)
      NOTIFY_RESULT_OBSERVERS(result, NodeDateAddedChanged(this, mDateAdded));
  }
  else if (aProperty.EqualsLiteral("lastModified")) {
    if (shouldNotify) {
      NOTIFY_RESULT_OBSERVERS(result,
                              NodeLastModifiedChanged(this, aLastModified));
    }
  }
  else if (aProperty.EqualsLiteral("keyword")) {
    if (shouldNotify)
      NOTIFY_RESULT_OBSERVERS(result, NodeKeywordChanged(this, aNewValue));
  }
  else
    NS_NOTREACHED("Unknown bookmark property changing.");

  if (!mParent)
    return NS_OK;

  
  
  
  PRInt32 ourIndex = mParent->FindChild(this);
  mParent->EnsureItemPosition(ourIndex);

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemChanged(PRInt64 aItemId,
                                            const nsACString& aProperty,
                                            PRBool aIsAnnotationProperty,
                                            const nsACString& aNewValue,
                                            PRTime aLastModified,
                                            PRUint16 aItemType) {
  
  if (mQueryItemId != -1) {
    PRBool isTitleChange = aProperty.EqualsLiteral("title");
    if ((mQueryItemId == aItemId && !isTitleChange) ||
        (mQueryItemId != aItemId && isTitleChange)) {
      return NS_OK;
    }
  }

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  return nsNavHistoryResultNode::OnItemChanged(aItemId, aProperty,
                                               aIsAnnotationProperty,
                                               aNewValue,
                                               aLastModified,
                                               aItemType);
}




NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemVisited(PRInt64 aItemId,
                                            PRInt64 aVisitId, PRTime aTime)
{
  PRBool excludeItems = (mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
                        (mParent && mParent->mOptions->ExcludeItems()) ||
                        mOptions->ExcludeItems();
  if (excludeItems)
    return NS_OK; 

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  if (!StartIncrementalUpdate())
    return NS_OK;

  PRUint32 nodeIndex;
  nsNavHistoryResultNode* node = FindChildById(aItemId, &nodeIndex);
  if (!node)
    return NS_ERROR_FAILURE;

  
  node->mTime = aTime;
  ++node->mAccessCount;

  
  PRInt32 oldAccessCount = mAccessCount;
  ++mAccessCount;
  if (aTime > mTime)
    mTime = aTime;
  nsresult rv = ReverseUpdateStats(mAccessCount - oldAccessCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (AreChildrenVisible()) {
    
    nsNavHistoryResult* result = GetResult();
    NOTIFY_RESULT_OBSERVERS(result,
                            NodeHistoryDetailsChanged(node, mTime, mAccessCount));
  }

  
  PRUint32 sortType = GetSortType();
  if (sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING) {
    PRInt32 childIndex = FindChild(node);
    NS_ASSERTION(childIndex >= 0, "Could not find child we just got a reference to");
    if (childIndex >= 0) {
      EnsureItemPosition(childIndex);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemMoved(PRInt64 aItemId, PRInt64 aOldParent,
                                          PRInt32 aOldIndex, PRInt64 aNewParent,
                                          PRInt32 aNewIndex, PRUint16 aItemType)
{
  NS_ASSERTION(aOldParent == mItemId || aNewParent == mItemId,
               "Got a bookmark message that doesn't belong to us");

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  if (!StartIncrementalUpdate())
    return NS_OK; 

  if (aOldParent == aNewParent) {
    
    

    
    ReindexRange(aOldIndex + 1, PR_INT32_MAX, -1);
    ReindexRange(aNewIndex, PR_INT32_MAX, 1);

    PRUint32 index;
    nsNavHistoryResultNode* node = FindChildById(aItemId, &index);
    if (!node) {
      NS_NOTREACHED("Can't find folder that is moving!");
      return NS_ERROR_FAILURE;
    }
    NS_ASSERTION(index >= 0 && index < PRUint32(mChildren.Count()),
                 "Invalid index!");
    node->mBookmarkIndex = aNewIndex;

    
    EnsureItemPosition(index);
    return NS_OK;
  } else {
    
    if (aOldParent == mItemId)
      OnItemRemoved(aItemId, aOldParent, aOldIndex, aItemType);
    if (aNewParent == mItemId) {
      nsCOMPtr<nsIURI> itemURI;
      if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK) {
        nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
        NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

        nsresult rv =
          bookmarks->GetBookmarkURI(aItemId, getter_AddRefs(itemURI));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      OnItemAdded(aItemId, aNewParent, aNewIndex, aItemType, itemURI);
    }
  }
  return NS_OK;
}





nsNavHistorySeparatorResultNode::nsNavHistorySeparatorResultNode()
  : nsNavHistoryResultNode(EmptyCString(), EmptyCString(),
                           0, 0, EmptyCString())
{
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsNavHistoryResult)

static PLDHashOperator
RemoveBookmarkFolderObserversCallback(nsTrimInt64HashKey::KeyType aKey,
                                      nsNavHistoryResult::FolderObserverList*& aData,
                                      void* userArg)
{
  delete aData;
  return PL_DHASH_REMOVE;
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsNavHistoryResult)
  tmp->StopObserving();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRootNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mObservers)
  tmp->mBookmarkFolderObservers.Enumerate(&RemoveBookmarkFolderObserversCallback, nsnull);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mAllBookmarksObservers)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mHistoryObservers)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

static PLDHashOperator
TraverseBookmarkFolderObservers(nsTrimInt64HashKey::KeyType aKey,
                                nsNavHistoryResult::FolderObserverList* &aData,
                                void *aClosure)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  for (PRUint32 i = 0; i < aData->Length(); ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                       "mBookmarkFolderObservers value[i]");
    nsNavHistoryResultNode* node = aData->ElementAt(i);
    cb->NoteXPCOMChild(node);
  }
  return PL_DHASH_NEXT;
}

static void
traverseResultObservers(nsMaybeWeakPtrArray<nsINavHistoryResultObserver> aObservers,
                        void *aClosure)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  for (PRUint32 i = 0; i < aObservers.Length(); ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mResultObservers value[i]");
    const nsCOMPtr<nsINavHistoryResultObserver> &obs = aObservers.ElementAt(i);
    cb->NoteXPCOMChild(obs);
  }
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsNavHistoryResult)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mRootNode, nsINavHistoryContainerResultNode)
  traverseResultObservers(tmp->mObservers, &cb);
  tmp->mBookmarkFolderObservers.Enumerate(&TraverseBookmarkFolderObservers, &cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_MEMBER(mAllBookmarksObservers, nsNavHistoryQueryResultNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_MEMBER(mHistoryObservers, nsNavHistoryQueryResultNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNavHistoryResult)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNavHistoryResult)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNavHistoryResult)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryResult)
  NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsNavHistoryResult)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryResult)
  NS_INTERFACE_MAP_ENTRY(nsINavBookmarkObserver)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

nsNavHistoryResult::nsNavHistoryResult(nsNavHistoryContainerResultNode* aRoot)
: mRootNode(aRoot)
, mNeedsToApplySortingMode(PR_FALSE)
, mIsHistoryObserver(PR_FALSE)
, mIsBookmarkFolderObserver(PR_FALSE)
, mIsAllBookmarksObserver(PR_FALSE)
, mBatchInProgress(PR_FALSE)
, mSuppressNotifications(PR_FALSE)
{
  mRootNode->mResult = this;
}

nsNavHistoryResult::~nsNavHistoryResult()
{
  
  mBookmarkFolderObservers.Enumerate(&RemoveBookmarkFolderObserversCallback, nsnull);
}

void
nsNavHistoryResult::StopObserving()
{
  if (mIsBookmarkFolderObserver || mIsAllBookmarksObserver) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    if (bookmarks) {
      bookmarks->RemoveObserver(this);
      mIsBookmarkFolderObserver = PR_FALSE;
      mIsAllBookmarksObserver = PR_FALSE;
    }
  }
  if (mIsHistoryObserver) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    if (history) {
      history->RemoveObserver(this);
      mIsHistoryObserver = PR_FALSE;
    }
  }
}





nsresult
nsNavHistoryResult::Init(nsINavHistoryQuery** aQueries,
                         PRUint32 aQueryCount,
                         nsNavHistoryQueryOptions *aOptions)
{
  nsresult rv;
  NS_ASSERTION(aOptions, "Must have valid options");
  NS_ASSERTION(aQueries && aQueryCount > 0, "Must have >1 query in result");

  
  
  
  for (PRUint32 i = 0; i < aQueryCount; ++i) {
    nsCOMPtr<nsINavHistoryQuery> queryClone;
    rv = aQueries[i]->Clone(getter_AddRefs(queryClone));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!mQueries.AppendObject(queryClone))
      return NS_ERROR_OUT_OF_MEMORY;
  }
  rv = aOptions->Clone(getter_AddRefs(mOptions));
  NS_ENSURE_SUCCESS(rv, rv);
  mSortingMode = aOptions->SortingMode();
  rv = aOptions->GetSortingAnnotation(mSortingAnnotation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mBookmarkFolderObservers.Init(128))
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ASSERTION(mRootNode->mIndentLevel == -1,
               "Root node's indent level initialized wrong");
  mRootNode->FillStats();

  return NS_OK;
}





nsresult 
nsNavHistoryResult::NewHistoryResult(nsINavHistoryQuery** aQueries,
                                     PRUint32 aQueryCount,
                                     nsNavHistoryQueryOptions* aOptions,
                                     nsNavHistoryContainerResultNode* aRoot,
                                     bool aBatchInProgress,
                                     nsNavHistoryResult** result)
{
  *result = new nsNavHistoryResult(aRoot);
  if (!*result)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result); 
  
  (*result)->mBatchInProgress = aBatchInProgress;
  nsresult rv = (*result)->Init(aQueries, aQueryCount, aOptions);
  if (NS_FAILED(rv)) {
    NS_RELEASE(*result);
    *result = nsnull;
    return rv;
  }

  return NS_OK;
}


void
nsNavHistoryResult::AddHistoryObserver(nsNavHistoryQueryResultNode* aNode)
{
  if (!mIsHistoryObserver) {
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ASSERTION(history, "Can't create history service");
      history->AddObserver(this, PR_TRUE);
      mIsHistoryObserver = PR_TRUE;
  }
  
  
  
  if (mHistoryObservers.IndexOf(aNode) == mHistoryObservers.NoIndex) {
    mHistoryObservers.AppendElement(aNode);
  }
}


void
nsNavHistoryResult::AddAllBookmarksObserver(nsNavHistoryQueryResultNode* aNode)
{
  if (!mIsAllBookmarksObserver && !mIsBookmarkFolderObserver) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    if (!bookmarks) {
      NS_NOTREACHED("Can't create bookmark service");
      return;
    }
    bookmarks->AddObserver(this, PR_TRUE);
    mIsAllBookmarksObserver = PR_TRUE;
  }
  
  
  
  if (mAllBookmarksObservers.IndexOf(aNode) == mAllBookmarksObservers.NoIndex) {
    mAllBookmarksObservers.AppendElement(aNode);
  }
}


void
nsNavHistoryResult::AddBookmarkFolderObserver(nsNavHistoryFolderResultNode* aNode,
                                              PRInt64 aFolder)
{
  if (!mIsBookmarkFolderObserver && !mIsAllBookmarksObserver) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    if (!bookmarks) {
      NS_NOTREACHED("Can't create bookmark service");
      return;
    }
    bookmarks->AddObserver(this, PR_TRUE);
    mIsBookmarkFolderObserver = PR_TRUE;
  }
  
  
  
  FolderObserverList* list = BookmarkFolderObserversForId(aFolder, PR_TRUE);
  if (list->IndexOf(aNode) == list->NoIndex) {
    list->AppendElement(aNode);
  }
}


void
nsNavHistoryResult::RemoveHistoryObserver(nsNavHistoryQueryResultNode* aNode)
{
  mHistoryObservers.RemoveElement(aNode);
}


void
nsNavHistoryResult::RemoveAllBookmarksObserver(nsNavHistoryQueryResultNode* aNode)
{
  mAllBookmarksObservers.RemoveElement(aNode);
}


void
nsNavHistoryResult::RemoveBookmarkFolderObserver(nsNavHistoryFolderResultNode* aNode,
                                                 PRInt64 aFolder)
{
  FolderObserverList* list = BookmarkFolderObserversForId(aFolder, PR_FALSE);
  if (!list)
    return; 
  list->RemoveElement(aNode);
}


nsNavHistoryResult::FolderObserverList*
nsNavHistoryResult::BookmarkFolderObserversForId(PRInt64 aFolderId, PRBool aCreate)
{
  FolderObserverList* list;
  if (mBookmarkFolderObservers.Get(aFolderId, &list))
    return list;
  if (!aCreate)
    return nsnull;

  
  list = new FolderObserverList;
  mBookmarkFolderObservers.Put(aFolderId, list);
  return list;
}


NS_IMETHODIMP
nsNavHistoryResult::GetSortingMode(PRUint16* aSortingMode)
{
  *aSortingMode = mSortingMode;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::SetSortingMode(PRUint16 aSortingMode)
{
  NS_ENSURE_STATE(mRootNode);

  if (aSortingMode > nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING)
    return NS_ERROR_INVALID_ARG;

  
  NS_ASSERTION(mOptions, "Options should always be present for a root query");

  mSortingMode = aSortingMode;

  if (!mRootNode->mExpanded) {
    
    mNeedsToApplySortingMode = PR_TRUE;
    return NS_OK;
  }

  
  nsNavHistoryContainerResultNode::SortComparator comparator =
      nsNavHistoryContainerResultNode::GetSortingComparator(aSortingMode);
  if (comparator) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    mRootNode->RecursiveSort(mSortingAnnotation.get(), comparator);
  }

  NOTIFY_RESULT_OBSERVERS(this, SortingChanged(aSortingMode));
  NOTIFY_RESULT_OBSERVERS(this, InvalidateContainer(mRootNode));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::GetSortingAnnotation(nsACString& _result) {
  _result.Assign(mSortingAnnotation);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::SetSortingAnnotation(const nsACString& aSortingAnnotation) {
  mSortingAnnotation.Assign(aSortingAnnotation);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::AddObserver(nsINavHistoryResultObserver* aObserver,
                                PRBool aOwnsWeak)
{
  NS_ENSURE_ARG(aObserver);
  nsresult rv = mObservers.AppendWeakElement(aObserver, aOwnsWeak);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aObserver->SetResult(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (mBatchInProgress) {
    NOTIFY_RESULT_OBSERVERS(this, Batching(PR_TRUE));
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::RemoveObserver(nsINavHistoryResultObserver* aObserver)
{
  NS_ENSURE_ARG(aObserver);
  return mObservers.RemoveWeakElement(aObserver);
}


NS_IMETHODIMP
nsNavHistoryResult::GetSuppressNotifications(PRBool* _retval)
{
  *_retval = mSuppressNotifications;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::SetSuppressNotifications(PRBool aSuppressNotifications)
{
  mSuppressNotifications = aSuppressNotifications;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::GetRoot(nsINavHistoryContainerResultNode** aRoot)
{
  if (!mRootNode) {
    NS_NOTREACHED("Root is null");
    *aRoot = nsnull;
    return NS_ERROR_FAILURE;
  }
  return mRootNode->QueryInterface(NS_GET_IID(nsINavHistoryContainerResultNode),
                                   reinterpret_cast<void**>(aRoot));
}


void
nsNavHistoryResult::requestRefresh(nsNavHistoryContainerResultNode* aContainer)
{
  
  if (mRefreshParticipants.IndexOf(aContainer) == mRefreshParticipants.NoIndex)
    mRefreshParticipants.AppendElement(aContainer);
}






#define ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(_folderId, _functionCall) \
  PR_BEGIN_MACRO \
    FolderObserverList* _fol = BookmarkFolderObserversForId(_folderId, PR_FALSE); \
    if (_fol) { \
      FolderObserverList _listCopy(*_fol); \
      for (PRUint32 _fol_i = 0; _fol_i < _listCopy.Length(); ++_fol_i) { \
        if (_listCopy[_fol_i]) \
          _listCopy[_fol_i]->_functionCall; \
      } \
    } \
  PR_END_MACRO
#define ENUMERATE_LIST_OBSERVERS(_listType, _functionCall, _observersList, _conditionCall) \
  PR_BEGIN_MACRO \
    _listType _listCopy(_observersList); \
    for (PRUint32 _obs_i = 0; _obs_i < _listCopy.Length(); ++_obs_i) { \
      if (_listCopy[_obs_i] && _listCopy[_obs_i]->_conditionCall) \
        _listCopy[_obs_i]->_functionCall; \
    } \
  PR_END_MACRO
#define ENUMERATE_QUERY_OBSERVERS(_functionCall, _observersList, _conditionCall) \
  ENUMERATE_LIST_OBSERVERS(QueryObserverList, _functionCall, _observersList, _conditionCall)
#define ENUMERATE_ALL_BOOKMARKS_OBSERVERS(_functionCall) \
  ENUMERATE_QUERY_OBSERVERS(_functionCall, mAllBookmarksObservers, IsQuery())
#define ENUMERATE_HISTORY_OBSERVERS(_functionCall) \
  ENUMERATE_QUERY_OBSERVERS(_functionCall, mHistoryObservers, IsQuery())

#define NOTIFY_REFRESH_PARTICIPANTS() \
  PR_BEGIN_MACRO \
  ENUMERATE_LIST_OBSERVERS(ContainerObserverList, Refresh(), mRefreshParticipants, IsContainer()); \
  mRefreshParticipants.Clear(); \
  PR_END_MACRO

NS_IMETHODIMP
nsNavHistoryResult::OnBeginUpdateBatch()
{
  
  
  if (!mBatchInProgress) {
    mBatchInProgress = PR_TRUE;
    ENUMERATE_HISTORY_OBSERVERS(OnBeginUpdateBatch());
    ENUMERATE_ALL_BOOKMARKS_OBSERVERS(OnBeginUpdateBatch());

    NOTIFY_RESULT_OBSERVERS(this, Batching(PR_TRUE));
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnEndUpdateBatch()
{
  
  
  
  
  
  if (mBatchInProgress) {
    ENUMERATE_HISTORY_OBSERVERS(OnEndUpdateBatch());
    ENUMERATE_ALL_BOOKMARKS_OBSERVERS(OnEndUpdateBatch());

    
    
    
    mBatchInProgress = PR_FALSE;
    NOTIFY_REFRESH_PARTICIPANTS();
    NOTIFY_RESULT_OBSERVERS(this, Batching(PR_FALSE));
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemAdded(PRInt64 aItemId,
                                PRInt64 aParentId,
                                PRInt32 aIndex,
                                PRUint16 aItemType,
                                nsIURI* aURI)
{
  ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aParentId,
    OnItemAdded(aItemId, aParentId, aIndex, aItemType, aURI)
  );
  ENUMERATE_HISTORY_OBSERVERS(
    OnItemAdded(aItemId, aParentId, aIndex, aItemType, aURI)
  );
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
    OnItemAdded(aItemId, aParentId, aIndex, aItemType, aURI)
  );
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnBeforeItemRemoved(PRInt64 aItemId, PRUint16 aItemType)
{
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
    OnBeforeItemRemoved(aItemId, aItemType);
  );
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemRemoved(PRInt64 aItemId,
                                  PRInt64 aParentId, PRInt32 aIndex,
                                  PRUint16 aItemType)
{
  ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aParentId,
      OnItemRemoved(aItemId, aParentId, aIndex, aItemType));
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
      OnItemRemoved(aItemId, aParentId, aIndex, aItemType));
  ENUMERATE_HISTORY_OBSERVERS(
      OnItemRemoved(aItemId, aParentId, aIndex, aItemType));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemChanged(PRInt64 aItemId,
                                  const nsACString &aProperty,
                                  PRBool aIsAnnotationProperty,
                                  const nsACString &aNewValue,
                                  PRTime aLastModified,
                                  PRUint16 aItemType)
{
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
    OnItemChanged(aItemId, aProperty, aIsAnnotationProperty, aNewValue,
                  aLastModified, aItemType));

  
  
  

  nsNavBookmarks* bookmarkService = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarkService, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt64 folderId;
  nsresult rv = bookmarkService->GetFolderIdForItem(aItemId, &folderId);
  NS_ENSURE_SUCCESS(rv, rv);

  FolderObserverList* list = BookmarkFolderObserversForId(folderId, PR_FALSE);
  if (!list)
    return NS_OK;

  for (PRUint32 i = 0; i < list->Length(); ++i) {
    nsRefPtr<nsNavHistoryFolderResultNode> folder = list->ElementAt(i);
    if (folder) {
      PRUint32 nodeIndex;
      nsRefPtr<nsNavHistoryResultNode> node =
        folder->FindChildById(aItemId, &nodeIndex);
      
      PRBool excludeItems = (mRootNode->mOptions->ExcludeItems()) ||
                             folder->mOptions->ExcludeItems();
      if (node &&
          (!excludeItems || !(node->IsURI() || node->IsSeparator())) &&
          folder->StartIncrementalUpdate()) {
        node->OnItemChanged(aItemId, aProperty, aIsAnnotationProperty,
                            aNewValue, aLastModified, aItemType);
      }
    }
  }

  
  
  
  
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemVisited(PRInt64 aItemId, PRInt64 aVisitId,
                                  PRTime aVisitTime)
{
  nsresult rv;
  nsNavBookmarks* bookmarkService = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarkService, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt64 folderId;
  rv = bookmarkService->GetFolderIdForItem(aItemId, &folderId);
  NS_ENSURE_SUCCESS(rv, rv);
  ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(folderId,
      OnItemVisited(aItemId, aVisitId, aVisitTime));
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
      OnItemVisited(aItemId, aVisitId, aVisitTime));
  
  
  
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryResult::OnItemMoved(PRInt64 aItemId,
                                PRInt64 aOldParent, PRInt32 aOldIndex,
                                PRInt64 aNewParent, PRInt32 aNewIndex,
                                PRUint16 aItemType)
{
  { 
    ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aOldParent,
        OnItemMoved(aItemId, aOldParent, aOldIndex, aNewParent, aNewIndex,
                    aItemType));
  }
  if (aNewParent != aOldParent) {
    ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aNewParent,
        OnItemMoved(aItemId, aOldParent, aOldIndex, aNewParent, aNewIndex,
                    aItemType));
  }
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(OnItemMoved(aItemId, aOldParent, aOldIndex,
                                                aNewParent, aNewIndex,
                                                aItemType));
  ENUMERATE_HISTORY_OBSERVERS(OnItemMoved(aItemId, aOldParent, aOldIndex,
                                          aNewParent, aNewIndex, aItemType));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnVisit(nsIURI* aURI, PRInt64 aVisitId, PRTime aTime,
                            PRInt64 aSessionId, PRInt64 aReferringId,
                            PRUint32 aTransitionType, PRUint32* aAdded)
{
  PRUint32 added = 0;

  ENUMERATE_HISTORY_OBSERVERS(OnVisit(aURI, aVisitId, aTime, aSessionId,
                                      aReferringId, aTransitionType, &added));

  if (!mRootNode->mExpanded)
    return NS_OK;

  
  
  
  PRBool todayIsMissing = PR_FALSE;
  PRUint32 resultType = mRootNode->mOptions->ResultType();
  if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
      resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY) {
    PRUint32 childCount;
    nsresult rv = mRootNode->GetChildCount(&childCount);
    NS_ENSURE_SUCCESS(rv, rv);
    if (childCount) {
      nsCOMPtr<nsINavHistoryResultNode> firstChild;
      rv = mRootNode->GetChild(0, getter_AddRefs(firstChild));
      NS_ENSURE_SUCCESS(rv, rv);
      nsCAutoString title;
      rv = firstChild->GetTitle(title);
      NS_ENSURE_SUCCESS(rv, rv);
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, 0);
      nsCAutoString todayLabel;
      history->GetStringFromName(
        NS_LITERAL_STRING("finduri-AgeInDays-is-0").get(), todayLabel);
      todayIsMissing = !todayLabel.Equals(title);
    }
  }

  if (!added || todayIsMissing) {
    
    
    PRUint32 resultType = mRootNode->mOptions->ResultType();
    if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
        resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY ||
        resultType == nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY)
      (void)mRootNode->GetAsQuery()->Refresh();
    else {
      
      
      
      
      ENUMERATE_QUERY_OBSERVERS(Refresh(), mHistoryObservers, IsContainersQuery());
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnTitleChanged(nsIURI* aURI, const nsAString& aPageTitle)
{
  ENUMERATE_HISTORY_OBSERVERS(OnTitleChanged(aURI, aPageTitle));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnBeforeDeleteURI(nsIURI *aURI)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnDeleteURI(nsIURI *aURI)
{
  ENUMERATE_HISTORY_OBSERVERS(OnDeleteURI(aURI));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnClearHistory()
{
  ENUMERATE_HISTORY_OBSERVERS(OnClearHistory());
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnPageChanged(nsIURI *aURI,
                                  PRUint32 aWhat, const nsAString &aValue)
{
  ENUMERATE_HISTORY_OBSERVERS(OnPageChanged(aURI, aWhat, aValue));
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistoryResult::OnDeleteVisits(nsIURI* aURI, PRTime aVisitTime)
{
  ENUMERATE_HISTORY_OBSERVERS(OnDeleteVisits(aURI, aVisitTime));
  return NS_OK;
}
