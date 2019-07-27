




#include <stdio.h>
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsFaviconService.h"
#include "nsITaggingService.h"
#include "nsAnnotationService.h"
#include "Helpers.h"
#include "mozilla/DebugOnly.h"
#include "nsDebug.h"
#include "nsNetUtil.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "prtime.h"
#include "prprf.h"
#include "nsQueryObject.h"

#include "nsCycleCollectionParticipant.h"


#undef CompareString

#define TO_ICONTAINER(_node)                                                  \
    static_cast<nsINavHistoryContainerResultNode*>(_node)                      

#define TO_CONTAINER(_node)                                                   \
    static_cast<nsNavHistoryContainerResultNode*>(_node)

#define NOTIFY_RESULT_OBSERVERS_RET(_result, _method, _ret)                   \
  PR_BEGIN_MACRO                                                              \
  NS_ENSURE_TRUE(_result, _ret);                                              \
  if (!_result->mSuppressNotifications) {                                     \
    ENUMERATE_WEAKARRAY(_result->mObservers, nsINavHistoryResultObserver,     \
                        _method)                                              \
  }                                                                           \
  PR_END_MACRO

#define NOTIFY_RESULT_OBSERVERS(_result, _method)                             \
  NOTIFY_RESULT_OBSERVERS_RET(_result, _method, NS_ERROR_UNEXPECTED)






#define NS_INTERFACE_MAP_STATIC_AMBIGUOUS(_class) \
  if (aIID.Equals(NS_GET_IID(_class))) { \
    NS_ADDREF(this); \
    *aInstancePtr = this; \
    return NS_OK; \
  } else



#define MAX_BATCH_CHANGES_BEFORE_REFRESH 5


inline int32_t ComparePRTime(PRTime a, PRTime b)
{
  if (a < b)
    return -1;
  else if (a > b)
    return 1;
  return 0;
}
inline int32_t CompareIntegers(uint32_t a, uint32_t b)
{
  return a - b;
}

using namespace mozilla;
using namespace mozilla::places;

NS_IMPL_CYCLE_COLLECTION(nsNavHistoryResultNode, mParent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNavHistoryResultNode)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryResultNode)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryResultNode)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNavHistoryResultNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNavHistoryResultNode)

nsNavHistoryResultNode::nsNavHistoryResultNode(
    const nsACString& aURI, const nsACString& aTitle, uint32_t aAccessCount,
    PRTime aTime, const nsACString& aIconURI) :
  mParent(nullptr),
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
  mFrecency(0),
  mHidden(false),
  mTransitionType(0)
{
  mTags.SetIsVoid(true);
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
  *aResult = nullptr;
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
      mTags.SetIsVoid(true);
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

  
  nsRefPtr<Database> DB = Database::GetDatabase();
  NS_ENSURE_STATE(DB);
  nsCOMPtr<mozIStorageStatement> stmt = DB->GetStatement(
    "/* do not warn (bug 487594) */ "
    "SELECT GROUP_CONCAT(tag_title, ', ') "
    "FROM ( "
      "SELECT t.title AS tag_title "
      "FROM moz_bookmarks b "
      "JOIN moz_bookmarks t ON t.id = +b.parent "
      "WHERE b.fk = (SELECT id FROM moz_places WHERE url = :page_url) "
        "AND t.parent = :tags_folder "
      "ORDER BY t.title COLLATE NOCASE ASC "
    ") "
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("tags_folder"),
                                      history->GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasTags = false;
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

NS_IMETHODIMP
nsNavHistoryResultNode::GetPageGuid(nsACString& aPageGuid) {
  aPageGuid = mPageGuid;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResultNode::GetBookmarkGuid(nsACString& aBookmarkGuid) {
  aBookmarkGuid = mBookmarkGuid;
  return NS_OK;
}


void
nsNavHistoryResultNode::OnRemoving()
{
  mParent = nullptr;
}










nsNavHistoryResult*
nsNavHistoryResultNode::GetResult()
{
  nsNavHistoryResultNode* node = this;
  do {
    if (node->IsContainer()) {
      nsNavHistoryContainerResultNode* container = TO_CONTAINER(node);
      return container->mResult;
    }
    node = node->mParent;
  } while (node);
  MOZ_ASSERT(false, "No container node found in hierarchy!");
  return nullptr;
}











nsNavHistoryQueryOptions*
nsNavHistoryResultNode::GetGeneratingOptions()
{
  if (!mParent) {
    
    
    
    
    if (IsContainer())
      return GetAsContainer()->mOptions;

    NS_NOTREACHED("Can't find a generating node for this container, perhaps FillStats has not been called on this tree yet?");
    return nullptr;
  }

  
  
  
  nsNavHistoryContainerResultNode* cur = mParent;
  while (cur) {
    if (cur->IsContainer())
      return cur->GetAsContainer()->mOptions;
    cur = cur->mParent;
  }

  
  NS_NOTREACHED("Can't find a generating node for this container, the tree seemes corrupted.");
  return nullptr;
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode,
                                   mResult,
                                   mChildren)

NS_IMPL_ADDREF_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)
NS_IMPL_RELEASE_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsNavHistoryContainerResultNode)
  NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsNavHistoryContainerResultNode)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryContainerResultNode)
NS_INTERFACE_MAP_END_INHERITING(nsNavHistoryResultNode)

nsNavHistoryContainerResultNode::nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    const nsACString& aIconURI, uint32_t aContainerType,
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryResultNode(aURI, aTitle, 0, 0, aIconURI),
  mResult(nullptr),
  mContainerType(aContainerType),
  mExpanded(false),
  mOptions(aOptions),
  mAsyncCanceledState(NOT_CANCELED)
{
}

nsNavHistoryContainerResultNode::nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    PRTime aTime,
    const nsACString& aIconURI, uint32_t aContainerType,
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryResultNode(aURI, aTitle, 0, aTime, aIconURI),
  mResult(nullptr),
  mContainerType(aContainerType),
  mExpanded(false),
  mOptions(aOptions),
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
  for (int32_t i = 0; i < mChildren.Count(); ++i)
    mChildren[i]->OnRemoving();
  mChildren.Clear();
  mResult = nullptr;
}


bool
nsNavHistoryContainerResultNode::AreChildrenVisible()
{
  nsNavHistoryResult* result = GetResult();
  if (!result) {
    NS_NOTREACHED("Invalid result");
    return false;
  }

  if (!mExpanded)
    return false;

  
  nsNavHistoryContainerResultNode* ancestor = mParent;
  while (ancestor) {
    if (!ancestor->mExpanded)
      return false;

    ancestor = ancestor->mParent;
  }

  return true;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetContainerOpen(bool *aContainerOpen)
{
  *aContainerOpen = mExpanded;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::SetContainerOpen(bool aContainerOpen)
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
      CancelAsyncOpen(false);
  }

  return NS_OK;
}










nsresult
nsNavHistoryContainerResultNode::NotifyOnStateChange(uint16_t aOldState)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  nsresult rv;
  uint16_t currState;
  rv = GetState(&currState);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NOTIFY_RESULT_OBSERVERS(result,
                          ContainerStateChanged(this, aOldState, currState));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetState(uint16_t* _state)
{
  NS_ENSURE_ARG_POINTER(_state);

  *_state = mExpanded ? (uint16_t)STATE_OPENED
                      : mAsyncPendingStmt ? (uint16_t)STATE_LOADING
                                          : (uint16_t)STATE_CLOSED;

  return NS_OK;
}






nsresult
nsNavHistoryContainerResultNode::OpenContainer()
{
  NS_ASSERTION(!mExpanded, "Container must not be expanded to open it");
  mExpanded = true;

  nsresult rv = NotifyOnStateChange(STATE_CLOSED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








nsresult
nsNavHistoryContainerResultNode::CloseContainer(bool aSuppressNotifications)
{
  NS_ASSERTION((mExpanded && !mAsyncPendingStmt) ||
               (!mExpanded && mAsyncPendingStmt),
               "Container must be expanded or loading to close it");

  nsresult rv;
  uint16_t oldState;
  rv = GetState(&oldState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mExpanded) {
    
    for (int32_t i = 0; i < mChildren.Count(); ++i) {
      if (mChildren[i]->IsContainer() &&
          mChildren[i]->GetAsContainer()->mExpanded)
        mChildren[i]->GetAsContainer()->CloseContainer(true);
    }

    mExpanded = false;
  }

  
  
  mAsyncPendingStmt = nullptr;

  if (!aSuppressNotifications) {
    rv = NotifyOnStateChange(oldState);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  if (result->mRootNode == this) {
    result->StopObserving();
    
    
    
    if (this->IsQuery())
      this->GetAsQuery()->ClearChildren(true);
    else if (this->IsFolder())
      this->GetAsFolder()->ClearChildren(true);
  }

  return NS_OK;
}





nsresult
nsNavHistoryContainerResultNode::OpenContainerAsync()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}











void
nsNavHistoryContainerResultNode::CancelAsyncOpen(bool aRestart)
{
  NS_ASSERTION(mAsyncPendingStmt, "Async execution canceled but not pending");

  mAsyncCanceledState = aRestart ? CANCELED_RESTART_NEEDED : CANCELED;

  
  
  
  (void)mAsyncPendingStmt->Cancel();
}











void
nsNavHistoryContainerResultNode::FillStats()
{
  uint32_t accessCount = 0;
  PRTime newTime = 0;

  for (int32_t i = 0; i < mChildren.Count(); ++i) {
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
nsNavHistoryContainerResultNode::ReverseUpdateStats(int32_t aAccessCountChange)
{
  if (mParent) {
    nsNavHistoryResult* result = GetResult();
    bool shouldNotify = result && mParent->mParent &&
                          mParent->mParent->AreChildrenVisible();

    mParent->mAccessCount += aAccessCountChange;
    bool timeChanged = false;
    if (mTime > mParent->mTime) {
      timeChanged = true;
      mParent->mTime = mTime;
    }

    if (shouldNotify) {
      NOTIFY_RESULT_OBSERVERS(result,
                              NodeHistoryDetailsChanged(TO_ICONTAINER(mParent),
                                                        mParent->mTime,
                                                        mParent->mAccessCount));
    }

    
    
    uint16_t sortMode = mParent->GetSortType();
    bool sortingByVisitCount =
      sortMode == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING ||
      sortMode == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING;
    bool sortingByTime =
      sortMode == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING ||
      sortMode == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING;

    if ((sortingByVisitCount && aAccessCountChange != 0) ||
        (sortingByTime && timeChanged)) {
      int32_t ourIndex = mParent->FindChild(this);
      NS_ASSERTION(ourIndex >= 0, "Could not find self in parent");
      if (ourIndex >= 0)
        EnsureItemPosition(static_cast<uint32_t>(ourIndex));
    }

    nsresult rv = mParent->ReverseUpdateStats(aAccessCountChange);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






uint16_t
nsNavHistoryContainerResultNode::GetSortType()
{
  if (mParent)
    return mParent->GetSortType();
  if (mResult)
    return mResult->mSortingMode;

  
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
}





nsNavHistoryContainerResultNode::SortComparator
nsNavHistoryContainerResultNode::GetSortingComparator(uint16_t aSortType)
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
      return nullptr;
  }
}









void
nsNavHistoryContainerResultNode::RecursiveSort(
    const char* aData, SortComparator aComparator)
{
  void* data = const_cast<void*>(static_cast<const void*>(aData));

  mChildren.Sort(aComparator, data);
  for (int32_t i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->IsContainer())
      mChildren[i]->GetAsContainer()->RecursiveSort(aData, aComparator);
  }
}






uint32_t
nsNavHistoryContainerResultNode::FindInsertionPoint(
    nsNavHistoryResultNode* aNode, SortComparator aComparator,
    const char* aData, bool* aItemExists)
{
  if (aItemExists)
    (*aItemExists) = false;

  if (mChildren.Count() == 0)
    return 0;

  void* data = const_cast<void*>(static_cast<const void*>(aData));

  
  
  int32_t res;
  res = aComparator(aNode, mChildren[0], data);
  if (res <= 0) {
    if (aItemExists && res == 0)
      (*aItemExists) = true;
    return 0;
  }
  res = aComparator(aNode, mChildren[mChildren.Count() - 1], data);
  if (res >= 0) {
    if (aItemExists && res == 0)
      (*aItemExists) = true;
    return mChildren.Count();
  }

  uint32_t beginRange = 0; 
  uint32_t endRange = mChildren.Count(); 
  while (1) {
    if (beginRange == endRange)
      return endRange;
    uint32_t center = beginRange + (endRange - beginRange) / 2;
    int32_t res = aComparator(aNode, mChildren[center], data);
    if (res <= 0) {
      endRange = center; 
      if (aItemExists && res == 0)
        (*aItemExists) = true;
    }
    else {
      beginRange = center + 1; 
    }
  }
}









bool
nsNavHistoryContainerResultNode::DoesChildNeedResorting(uint32_t aIndex,
    SortComparator aComparator, const char* aData)
{
  NS_ASSERTION(aIndex < uint32_t(mChildren.Count()),
               "Input index out of range");
  if (mChildren.Count() == 1)
    return false;

  void* data = const_cast<void*>(static_cast<const void*>(aData));

  if (aIndex > 0) {
    
    if (aComparator(mChildren[aIndex - 1], mChildren[aIndex], data) > 0)
      return true;
  }
  if (aIndex < uint32_t(mChildren.Count()) - 1) {
    
    if (aComparator(mChildren[aIndex], mChildren[aIndex + 1], data) > 0)
      return true;
  }
  return false;
}



int32_t nsNavHistoryContainerResultNode::SortComparison_StringLess(
    const nsAString& a, const nsAString& b) {

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, 0);
  nsICollation* collation = history->GetCollation();
  NS_ENSURE_TRUE(collation, 0);

  int32_t res = 0;
  collation->CompareString(nsICollation::kCollationCaseInSensitive, a, b, &res);
  return res;
}







int32_t nsNavHistoryContainerResultNode::SortComparison_Bookmark(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return a->mBookmarkIndex - b->mBookmarkIndex;
}









int32_t nsNavHistoryContainerResultNode::SortComparison_TitleLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  uint32_t aType;
  a->GetType(&aType);

  int32_t value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
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
int32_t nsNavHistoryContainerResultNode::SortComparison_TitleGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_TitleLess(a, b, closure);
}





int32_t nsNavHistoryContainerResultNode::SortComparison_DateLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  int32_t value = ComparePRTime(a->mTime, b->mTime);
  if (value == 0) {
    value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                      NS_ConvertUTF8toUTF16(b->mTitle));
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
int32_t nsNavHistoryContainerResultNode::SortComparison_DateGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_DateLess(a, b, closure);
}


int32_t nsNavHistoryContainerResultNode::SortComparison_DateAddedLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  int32_t value = ComparePRTime(a->mDateAdded, b->mDateAdded);
  if (value == 0) {
    value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                      NS_ConvertUTF8toUTF16(b->mTitle));
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
int32_t nsNavHistoryContainerResultNode::SortComparison_DateAddedGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_DateAddedLess(a, b, closure);
}


int32_t nsNavHistoryContainerResultNode::SortComparison_LastModifiedLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  int32_t value = ComparePRTime(a->mLastModified, b->mLastModified);
  if (value == 0) {
    value = SortComparison_StringLess(NS_ConvertUTF8toUTF16(a->mTitle),
                                      NS_ConvertUTF8toUTF16(b->mTitle));
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
int32_t nsNavHistoryContainerResultNode::SortComparison_LastModifiedGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_LastModifiedLess(a, b, closure);
}






int32_t nsNavHistoryContainerResultNode::SortComparison_URILess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  int32_t value;
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
int32_t nsNavHistoryContainerResultNode::SortComparison_URIGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_URILess(a, b, closure);
}


int32_t nsNavHistoryContainerResultNode::SortComparison_KeywordLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  int32_t value = 0;
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

int32_t nsNavHistoryContainerResultNode::SortComparison_KeywordGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_KeywordLess(a, b, closure);
}

int32_t nsNavHistoryContainerResultNode::SortComparison_AnnotationLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  nsAutoCString annoName(static_cast<char*>(closure));
  NS_ENSURE_TRUE(!annoName.IsEmpty(), 0);

  bool a_itemAnno = false;
  bool b_itemAnno = false;

  
  nsCOMPtr<nsIURI> a_uri, b_uri;
  if (a->mItemId != -1) {
    a_itemAnno = true;
  } else {
    nsAutoCString spec;
    if (NS_SUCCEEDED(a->GetUri(spec)))
      NS_NewURI(getter_AddRefs(a_uri), spec);
    NS_ENSURE_TRUE(a_uri, 0);
  }

  if (b->mItemId != -1) {
    b_itemAnno = true;
  } else {
    nsAutoCString spec;
    if (NS_SUCCEEDED(b->GetUri(spec)))
      NS_NewURI(getter_AddRefs(b_uri), spec);
    NS_ENSURE_TRUE(b_uri, 0);
  }

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, 0);

  bool a_hasAnno, b_hasAnno;
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

  int32_t value = 0;
  if (a_hasAnno || b_hasAnno) {
    uint16_t annoType;
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
      uint16_t b_type;
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

    if (annoType == nsIAnnotationService::TYPE_STRING) {
      nsAutoString a_val, b_val;
      GET_ANNOTATIONS_VALUES(GetItemAnnotationString,
                             GetPageAnnotationString, a_val, b_val);
      value = SortComparison_StringLess(a_val, b_val);
    }
    else if (annoType == nsIAnnotationService::TYPE_INT32) {
      int32_t a_val = 0, b_val = 0;
      GET_ANNOTATIONS_VALUES(GetItemAnnotationInt32,
                             GetPageAnnotationInt32, &a_val, &b_val);
      value = (a_val < b_val) ? -1 : (a_val > b_val) ? 1 : 0;
    }
    else if (annoType == nsIAnnotationService::TYPE_INT64) {
      int64_t a_val = 0, b_val = 0;
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

  
  
  
  if (value == 0)
    return SortComparison_TitleLess(a, b, nullptr);

  return value;
}
int32_t nsNavHistoryContainerResultNode::SortComparison_AnnotationGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_AnnotationLess(a, b, closure);
}




int32_t nsNavHistoryContainerResultNode::SortComparison_VisitCountLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  int32_t value = CompareIntegers(a->mAccessCount, b->mAccessCount);
  if (value == 0) {
    value = ComparePRTime(a->mTime, b->mTime);
    if (value == 0)
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
  }
  return value;
}
int32_t nsNavHistoryContainerResultNode::SortComparison_VisitCountGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -nsNavHistoryContainerResultNode::SortComparison_VisitCountLess(a, b, closure);
}


int32_t nsNavHistoryContainerResultNode::SortComparison_TagsLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  int32_t value = 0;
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

int32_t nsNavHistoryContainerResultNode::SortComparison_TagsGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_TagsLess(a, b, closure);
}




int32_t
nsNavHistoryContainerResultNode::SortComparison_FrecencyLess(
  nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure
)
{
  int32_t value = CompareIntegers(a->mFrecency, b->mFrecency);
  if (value == 0) {
    value = ComparePRTime(a->mTime, b->mTime);
    if (value == 0) {
      value = nsNavHistoryContainerResultNode::SortComparison_Bookmark(a, b, closure);
    }
  }
  return value;
}
int32_t
nsNavHistoryContainerResultNode::SortComparison_FrecencyGreater(
  nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure
)
{
  return -nsNavHistoryContainerResultNode::SortComparison_FrecencyLess(a, b, closure);
}







nsNavHistoryResultNode*
nsNavHistoryContainerResultNode::FindChildURI(const nsACString& aSpec,
    uint32_t* aNodeIndex)
{
  for (int32_t i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->IsURI()) {
      if (aSpec.Equals(mChildren[i]->mURI)) {
        *aNodeIndex = i;
        return mChildren[i];
      }
    }
  }
  return nullptr;
}






nsresult
nsNavHistoryContainerResultNode::InsertChildAt(nsNavHistoryResultNode* aNode,
                                               int32_t aIndex)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  aNode->mParent = this;
  aNode->mIndentLevel = mIndentLevel + 1;
  if (aNode->IsContainer()) {
    
    nsNavHistoryContainerResultNode* container = aNode->GetAsContainer();
    container->mResult = result;
    container->FillStats();
  }

  if (!mChildren.InsertObjectAt(aNode, aIndex))
    return NS_ERROR_OUT_OF_MEMORY;

  
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

  
  
  
  if (AreChildrenVisible())
    NOTIFY_RESULT_OBSERVERS(result, NodeInserted(this, aNode, aIndex));

  return NS_OK;
}






nsresult
nsNavHistoryContainerResultNode::InsertSortedChild(
    nsNavHistoryResultNode* aNode,
    bool aIgnoreDuplicates)
{

  if (mChildren.Count() == 0)
    return InsertChildAt(aNode, 0);

  SortComparator comparator = GetSortingComparator(GetSortType());
  if (comparator) {
    
    
    
    
    
    
    if (aNode->IsContainer()) {
      
      nsNavHistoryContainerResultNode* container = aNode->GetAsContainer();
      container->mResult = mResult;
      container->FillStats();
    }

    nsAutoCString sortingAnnotation;
    GetSortingAnnotation(sortingAnnotation);
    bool itemExists;
    uint32_t position = FindInsertionPoint(aNode, comparator, 
                                           sortingAnnotation.get(), 
                                           &itemExists);
    if (aIgnoreDuplicates && itemExists)
      return NS_OK;

    return InsertChildAt(aNode, position);
  }
  return InsertChildAt(aNode, mChildren.Count());
}








bool
nsNavHistoryContainerResultNode::EnsureItemPosition(uint32_t aIndex) {
  NS_ASSERTION(aIndex < (uint32_t)mChildren.Count(), "Invalid index");
  if (aIndex >= (uint32_t)mChildren.Count())
    return false;

  SortComparator comparator = GetSortingComparator(GetSortType());
  if (!comparator)
    return false;

  nsAutoCString sortAnno;
  GetSortingAnnotation(sortAnno);
  if (!DoesChildNeedResorting(aIndex, comparator, sortAnno.get()))
    return false;

  nsRefPtr<nsNavHistoryResultNode> node(mChildren[aIndex]);
  mChildren.RemoveObjectAt(aIndex);

  uint32_t newIndex = FindInsertionPoint(
                          node, comparator,sortAnno.get(), nullptr);
  mChildren.InsertObjectAt(node.get(), newIndex);

  if (AreChildrenVisible()) {
    nsNavHistoryResult* result = GetResult();
    NOTIFY_RESULT_OBSERVERS_RET(result,
                                NodeMoved(node, this, aIndex, this, newIndex),
                                false);
  }

  return true;
}






nsresult
nsNavHistoryContainerResultNode::RemoveChildAt(int32_t aIndex)
{
  NS_ASSERTION(aIndex >= 0 && aIndex < mChildren.Count(), "Invalid index");

  
  nsRefPtr<nsNavHistoryResultNode> oldNode = mChildren[aIndex];

  
  
  
  
  uint32_t oldAccessCount = mAccessCount;
  mAccessCount -= mChildren[aIndex]->mAccessCount;

  
  mChildren.RemoveObjectAt(aIndex);
  if (AreChildrenVisible()) {
    nsNavHistoryResult* result = GetResult();
    NOTIFY_RESULT_OBSERVERS(result,
                            NodeRemoved(this, oldNode, aIndex));
  }

  nsresult rv = ReverseUpdateStats(mAccessCount - oldAccessCount);
  NS_ENSURE_SUCCESS(rv, rv);
  oldNode->OnRemoving();
  return NS_OK;
}










void
nsNavHistoryContainerResultNode::RecursiveFindURIs(bool aOnlyOne,
    nsNavHistoryContainerResultNode* aContainer, const nsCString& aSpec,
    nsCOMArray<nsNavHistoryResultNode>* aMatches)
{
  for (int32_t child = 0; child < aContainer->mChildren.Count(); ++child) {
    uint32_t type;
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









bool
nsNavHistoryContainerResultNode::UpdateURIs(bool aRecursive, bool aOnlyOne,
    bool aUpdateSort, const nsCString& aSpec,
    nsresult (*aCallback)(nsNavHistoryResultNode*, const void*, const nsNavHistoryResult*),
    const void* aClosure)
{
  const nsNavHistoryResult* result = GetResult();
  if (!result) {
    MOZ_ASSERT(false, "Should have a result");
    return false;
  }

  
  
  nsCOMArray<nsNavHistoryResultNode> matches;

  if (aRecursive) {
    RecursiveFindURIs(aOnlyOne, this, aSpec, &matches);
  } else if (aOnlyOne) {
    uint32_t nodeIndex;
    nsNavHistoryResultNode* node = FindChildURI(aSpec, &nodeIndex);
    if (node)
      matches.AppendObject(node);
  } else {
    MOZ_ASSERT(false,
               "UpdateURIs does not handle nonrecursive updates of multiple items.");
    
    
    
    return false;
  }

  if (matches.Count() == 0)
    return false;

  
  
  
  
  for (int32_t i = 0; i < matches.Count(); ++i)
  {
    nsNavHistoryResultNode* node = matches[i];
    nsNavHistoryContainerResultNode* parent = node->mParent;
    if (!parent) {
      MOZ_ASSERT(false, "All URI nodes being updated must have parents");
      continue;
    }

    uint32_t oldAccessCount = node->mAccessCount;
    PRTime oldTime = node->mTime;
    aCallback(node, aClosure, result);

    if (oldAccessCount != node->mAccessCount || oldTime != node->mTime) {
      parent->mAccessCount += node->mAccessCount - oldAccessCount;
      if (node->mTime > parent->mTime)
        parent->mTime = node->mTime;
      if (parent->AreChildrenVisible()) {
        NOTIFY_RESULT_OBSERVERS_RET(result,
                                    NodeHistoryDetailsChanged(
                                      TO_ICONTAINER(parent),
                                      parent->mTime,
                                      parent->mAccessCount),
                                    true);
      }
      DebugOnly<nsresult> rv = parent->ReverseUpdateStats(node->mAccessCount - oldAccessCount);
      MOZ_ASSERT(NS_SUCCEEDED(rv), "should be able to ReverseUpdateStats");
    }

    if (aUpdateSort) {
      int32_t childIndex = parent->FindChild(node);
      MOZ_ASSERT(childIndex >= 0, "Could not find child we just got a reference to");
      if (childIndex >= 0)
        parent->EnsureItemPosition(childIndex);
    }
  }

  return true;
}








static nsresult setTitleCallback(nsNavHistoryResultNode* aNode,
                                 const void* aClosure,
                                 const nsNavHistoryResult* aResult)
{
  const nsACString* newTitle = static_cast<const nsACString*>(aClosure);
  aNode->mTitle = *newTitle;

  if (aResult && (!aNode->mParent || aNode->mParent->AreChildrenVisible()))
    NOTIFY_RESULT_OBSERVERS(aResult, NodeTitleChanged(aNode, *newTitle));

  return NS_OK;
}
nsresult
nsNavHistoryContainerResultNode::ChangeTitles(nsIURI* aURI,
                                              const nsACString& aNewTitle,
                                              bool aRecursive,
                                              bool aOnlyOne)
{
  
  nsAutoCString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  uint16_t sortType = GetSortType();
  bool updateSorting =
    (sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING ||
     sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING);

  UpdateURIs(aRecursive, aOnlyOne, updateSorting, uriString,
             setTitleCallback,
             static_cast<const void*>(&aNewTitle));

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetHasChildren(bool *aHasChildren)
{
  *aHasChildren = (mChildren.Count() > 0);
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChildCount(uint32_t* aChildCount)
{
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;
  *aChildCount = mChildren.Count();
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChild(uint32_t aIndex,
                                          nsINavHistoryResultNode** _retval)
{
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;
  if (aIndex >= uint32_t(mChildren.Count()))
    return NS_ERROR_INVALID_ARG;
  NS_ADDREF(*_retval = mChildren[aIndex]);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChildIndex(nsINavHistoryResultNode* aNode,
                                               uint32_t* _retval)
{
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;

  int32_t nodeIndex = FindChild(static_cast<nsNavHistoryResultNode*>(aNode));
  if (nodeIndex == -1)
    return NS_ERROR_INVALID_ARG;

  *_retval = nodeIndex;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryContainerResultNode::FindNodeByDetails(const nsACString& aURIString,
                                                   PRTime aTime,
                                                   int64_t aItemId,
                                                   bool aRecursive,
                                                   nsINavHistoryResultNode** _retval) {
  if (!mExpanded)
    return NS_ERROR_NOT_AVAILABLE;

  *_retval = nullptr;
  for (int32_t i = 0; i < mChildren.Count(); ++i) {
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



















NS_IMPL_ISUPPORTS_INHERITED(nsNavHistoryQueryResultNode,
                            nsNavHistoryContainerResultNode,
                            nsINavHistoryQueryResultNode)

nsNavHistoryQueryResultNode::nsNavHistoryQueryResultNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    const nsACString& aQueryURI) :
  nsNavHistoryContainerResultNode(aQueryURI, aTitle, aIconURI,
                                  nsNavHistoryResultNode::RESULT_TYPE_QUERY,
                                  nullptr),
  mLiveUpdate(QUERYUPDATE_COMPLEX_WITH_BOOKMARKS),
  mHasSearchTerms(false),
  mContentsValid(false),
  mBatchChanges(0)
{
}

nsNavHistoryQueryResultNode::nsNavHistoryQueryResultNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, aIconURI,
                                  nsNavHistoryResultNode::RESULT_TYPE_QUERY,
                                  aOptions),
  mQueries(aQueries),
  mContentsValid(false),
  mBatchChanges(0),
  mTransitions(mQueries[0]->Transitions())
{
  NS_ASSERTION(aQueries.Count() > 0, "Must have at least one query");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ASSERTION(history, "History service missing");
  if (history) {
    mLiveUpdate = history->GetUpdateRequirements(mQueries, mOptions,
                                                 &mHasSearchTerms);
  }

  
  for (int32_t i = 1; i < mQueries.Count(); ++i) {
    const nsTArray<uint32_t>& queryTransitions = mQueries[i]->Transitions();
    for (uint32_t j = 0; j < mTransitions.Length() ; ++j) {
      uint32_t transition = mTransitions.SafeElementAt(j, 0);
      if (transition && !queryTransitions.Contains(transition))
        mTransitions.RemoveElement(transition);
    }
  }
}

nsNavHistoryQueryResultNode::nsNavHistoryQueryResultNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    PRTime aTime,
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, aTime, aIconURI,
                                  nsNavHistoryResultNode::RESULT_TYPE_QUERY,
                                  aOptions),
  mQueries(aQueries),
  mContentsValid(false),
  mBatchChanges(0),
  mTransitions(mQueries[0]->Transitions())
{
  NS_ASSERTION(aQueries.Count() > 0, "Must have at least one query");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ASSERTION(history, "History service missing");
  if (history) {
    mLiveUpdate = history->GetUpdateRequirements(mQueries, mOptions,
                                                 &mHasSearchTerms);
  }

  
  for (int32_t i = 1; i < mQueries.Count(); ++i) {
    const nsTArray<uint32_t>& queryTransitions = mQueries[i]->Transitions();
    for (uint32_t j = 0; j < mTransitions.Length() ; ++j) {
      uint32_t transition = mTransitions.SafeElementAt(j, 0);
      if (transition && !queryTransitions.Contains(transition))
        mTransitions.RemoveElement(transition);
    }
  }
}

nsNavHistoryQueryResultNode::~nsNavHistoryQueryResultNode() {
  
  
  if (mResult && mResult->mAllBookmarksObservers.Contains(this))
    mResult->RemoveAllBookmarksObserver(this);
  if (mResult && mResult->mHistoryObservers.Contains(this))
    mResult->RemoveHistoryObserver(this);
}







bool
nsNavHistoryQueryResultNode::CanExpand()
{
  if (IsContainersQuery())
    return true;

  
  if ((mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
      Options()->ExcludeItems())
    return false;

  
  nsNavHistoryQueryOptions* options = GetGeneratingOptions();
  if (options) {
    if (options->ExcludeItems())
      return false;
    if (options->ExpandQueries())
      return true;
  }

  if (mResult && mResult->mRootNode == this)
    return true;

  return false;
}






bool
nsNavHistoryQueryResultNode::IsContainersQuery()
{
  uint16_t resultType = Options()->ResultType();
  return resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
         resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY ||
         resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY ||
         resultType == nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY;
}








void
nsNavHistoryQueryResultNode::OnRemoving()
{
  nsNavHistoryResultNode::OnRemoving();
  ClearChildren(true);
  mResult = nullptr;
}














nsresult
nsNavHistoryQueryResultNode::OpenContainer()
{
  NS_ASSERTION(!mExpanded, "Container must be closed to open it");
  mExpanded = true;

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
nsNavHistoryQueryResultNode::GetHasChildren(bool* aHasChildren)
{
  *aHasChildren = false;

  if (!CanExpand()) {
    return NS_OK;
  }

  uint16_t resultType = mOptions->ResultType();

  
  if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
    *aHasChildren = true;
    return NS_OK;
  }

  
  if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY) {
    nsCOMPtr<nsITaggingService> tagging =
      do_GetService(NS_TAGGINGSERVICE_CONTRACTID);
    if (tagging) {
      bool hasTags;
      *aHasChildren = NS_SUCCEEDED(tagging->GetHasTags(&hasTags)) && hasTags;
    }
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
  *aHasChildren = true;
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
nsNavHistoryQueryResultNode::GetFolderItemId(int64_t* aItemId)
{
  *aItemId = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetTargetFolderGuid(nsACString& aGuid) {
  aGuid = EmptyCString();
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetQueries(uint32_t* queryCount,
                                        nsINavHistoryQuery*** queries)
{
  nsresult rv = VerifyQueriesParsed();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mQueries.Count() > 0, "Must have >= 1 query");

  *queries = static_cast<nsINavHistoryQuery**>
                        (moz_xmalloc(mQueries.Count() * sizeof(nsINavHistoryQuery*)));
  NS_ENSURE_TRUE(*queries, NS_ERROR_OUT_OF_MEMORY);

  for (int32_t i = 0; i < mQueries.Count(); ++i)
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
    return nullptr;
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
  for (int32_t i = 0; i < mQueries.Count(); ++i)
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

  uint16_t sortType = GetSortType();

  if (mResult && mResult->mNeedsToApplySortingMode) {
    
    
    mResult->SetSortingMode(mResult->mSortingMode);
  }
  else if (mOptions->QueryType() != nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY ||
           sortType != nsINavHistoryQueryOptions::SORT_BY_NONE) {
    
    
    
    
    SortComparator comparator = GetSortingComparator(GetSortType());
    if (comparator) {
      nsAutoCString sortingAnnotation;
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
    while ((uint32_t)mChildren.Count() > mOptions->MaxResults())
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

  mContentsValid = true;
  return NS_OK;
}












void
nsNavHistoryQueryResultNode::ClearChildren(bool aUnregister)
{
  for (int32_t i = 0; i < mChildren.Count(); ++i)
    mChildren[i]->OnRemoving();
  mChildren.Clear();

  if (aUnregister && mContentsValid) {
    nsNavHistoryResult* result = GetResult();
    if (result) {
      result->RemoveHistoryObserver(this);
      result->RemoveAllBookmarksObserver(this);
    }
  }
  mContentsValid = false;
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
    
    ClearChildren(true);
    return NS_OK; 
  }

  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    ClearChildren(true);
  else
    ClearChildren(false);

  
  
  (void)FillChildren();

  NOTIFY_RESULT_OBSERVERS(result, InvalidateContainer(TO_CONTAINER(this)));
  return NS_OK;
}























uint16_t
nsNavHistoryQueryResultNode::GetSortType()
{
  if (mParent)
    return mOptions->SortingMode();
  if (mResult)
    return mResult->mSortingMode;

  
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
}

void
nsNavHistoryQueryResultNode::RecursiveSort(
    const char* aData, SortComparator aComparator)
{
  void* data = const_cast<void*>(static_cast<const void*>(aData));

  if (!IsContainersQuery())
    mChildren.Sort(aComparator, data);

  for (int32_t i = 0; i < mChildren.Count(); ++i) {
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

  mBatchChanges = 0;
  return NS_OK;
}

static nsresult setHistoryDetailsCallback(nsNavHistoryResultNode* aNode,
                                          const void* aClosure,
                                          const nsNavHistoryResult* aResult)
{
  const nsNavHistoryResultNode* updatedNode =
    static_cast<const nsNavHistoryResultNode*>(aClosure);

  aNode->mAccessCount = updatedNode->mAccessCount;
  aNode->mTime = updatedNode->mTime;
  aNode->mFrecency = updatedNode->mFrecency;
  aNode->mHidden = updatedNode->mHidden;

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnVisit(nsIURI* aURI, int64_t aVisitId,
                                     PRTime aTime, int64_t aSessionId,
                                     int64_t aReferringId,
                                     uint32_t aTransitionType,
                                     const nsACString& aGUID,
                                     bool aHidden,
                                     uint32_t* aAdded)
{
  if (aHidden && !mOptions->IncludeHidden())
    return NS_OK;

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  if (result->mBatchInProgress &&
      ++mBatchChanges > MAX_BATCH_CHANGES_BEFORE_REFRESH) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  switch(mLiveUpdate) {
    case QUERYUPDATE_HOST: {
      
      
      MOZ_ASSERT(mQueries.Count() == 1,
                 "Host updated queries can have only one object");
      nsRefPtr<nsNavHistoryQuery> query = do_QueryObject(mQueries[0]);

      bool hasDomain;
      query->GetHasDomain(&hasDomain);
      if (!hasDomain)
        return NS_OK;

      nsAutoCString host;
      if (NS_FAILED(aURI->GetAsciiHost(host)))
        return NS_OK;

      if (!query->Domain().Equals(host))
        return NS_OK;

      
      
    }

    case QUERYUPDATE_TIME: {
      
      
      MOZ_ASSERT(mQueries.Count() == 1,
                 "Time updated queries can have only one object");
      nsRefPtr<nsNavHistoryQuery> query = do_QueryObject(mQueries[0]);

      bool hasIt;
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
      
      
      if (mTransitions.Length() > 0 && !mTransitions.Contains(aTransitionType))
        return NS_OK;

      
      
      nsRefPtr<nsNavHistoryResultNode> addition;
      nsresult rv = history->VisitIdToResultNode(aVisitId, mOptions,
                                                 getter_AddRefs(addition));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(addition);
      addition->mTransitionType = aTransitionType;
      if (!history->EvaluateQueryForNode(mQueries, mOptions, addition))
        return NS_OK; 

      if (mOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_VISIT) {
        
        
        rv = InsertSortedChild(addition);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        uint16_t sortType = GetSortType();
        bool updateSorting =
          sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING ||
          sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING ||
          sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING ||
          sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING ||
          sortType == nsINavHistoryQueryOptions::SORT_BY_FRECENCY_ASCENDING ||
          sortType == nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING;

        if (!UpdateURIs(false, true, updateSorting, addition->mURI,
                        setHistoryDetailsCallback,
                        const_cast<void*>(static_cast<void*>(addition.get())))) {
          
          rv = InsertSortedChild(addition);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      if (aAdded)
        ++(*aAdded);

      break;
    }

    case QUERYUPDATE_COMPLEX:
    case QUERYUPDATE_COMPLEX_WITH_BOOKMARKS:
      
      return Refresh();

    default:
      MOZ_ASSERT(false, "Invalid value for mLiveUpdate");
      return Refresh();
  }

  return NS_OK;
}











NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnTitleChanged(nsIURI* aURI,
                                            const nsAString& aPageTitle,
                                            const nsACString& aGUID)
{
  if (!mExpanded) {
    
    
    
    
    ClearChildren(true);
    return NS_OK; 
  }

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  if (result->mBatchInProgress &&
      ++mBatchChanges > MAX_BATCH_CHANGES_BEFORE_REFRESH) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  
  NS_ConvertUTF16toUTF8 newTitle(aPageTitle);

  bool onlyOneEntry =
    mOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_URI ||
    mOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS;

  
  if (mHasSearchTerms) {
    
    nsCOMArray<nsNavHistoryResultNode> matches;
    nsAutoCString spec;
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
        rv = InsertSortedChild(node);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    for (int32_t i = 0; i < matches.Count(); ++i) {
      
      
      
      nsNavHistoryResultNode* node = matches[i];
      
      node->mTitle = newTitle;

      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      if (!history->EvaluateQueryForNode(mQueries, mOptions, node)) {
        nsNavHistoryContainerResultNode* parent = node->mParent;
        
        NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);
        int32_t childIndex = parent->FindChild(node);
        NS_ASSERTION(childIndex >= 0, "Child not found in parent");
        parent->RemoveChildAt(childIndex);
      }
    }
  }

  return ChangeTitles(aURI, newTitle, true, onlyOneEntry);
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnFrecencyChanged(nsIURI* aURI,
                                               int32_t aNewFrecency,
                                               const nsACString& aGUID,
                                               bool aHidden,
                                               PRTime aLastVisitDate)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnManyFrecenciesChanged()
{
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnDeleteURI(nsIURI* aURI,
                                         const nsACString& aGUID,
                                         uint16_t aReason)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  if (result->mBatchInProgress &&
      ++mBatchChanges > MAX_BATCH_CHANGES_BEFORE_REFRESH) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  if (IsContainersQuery()) {
    
    
    
    
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  bool onlyOneEntry = (mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_URI ||
                         mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS);
  nsAutoCString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsNavHistoryResultNode> matches;
  RecursiveFindURIs(onlyOneEntry, this, spec, &matches);
  for (int32_t i = 0; i < matches.Count(); ++i) {
    nsNavHistoryResultNode* node = matches[i];
    nsNavHistoryContainerResultNode* parent = node->mParent;
    
    NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

    int32_t childIndex = parent->FindChild(node);
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


static nsresult setFaviconCallback(nsNavHistoryResultNode* aNode,
                                   const void* aClosure,
                                   const nsNavHistoryResult* aResult)
{
  const nsCString* newFavicon = static_cast<const nsCString*>(aClosure);
  aNode->mFaviconURI = *newFavicon;

  if (aResult && (!aNode->mParent || aNode->mParent->AreChildrenVisible()))
    NOTIFY_RESULT_OBSERVERS(aResult, NodeIconChanged(aNode));

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnPageChanged(nsIURI* aURI,
                                           uint32_t aChangedAttribute,
                                           const nsAString& aNewValue,
                                           const nsACString& aGUID)
{
  nsAutoCString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (aChangedAttribute) {
    case nsINavHistoryObserver::ATTRIBUTE_FAVICON: {
      NS_ConvertUTF16toUTF8 newFavicon(aNewValue);
      bool onlyOneEntry = (mOptions->ResultType() ==
                             nsINavHistoryQueryOptions::RESULTS_AS_URI ||
                             mOptions->ResultType() ==
                             nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS);
      UpdateURIs(true, onlyOneEntry, false, spec, setFaviconCallback,
                 &newFavicon);
      break;
    }
    default:
      NS_WARNING("Unknown page changed notification");
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnDeleteVisits(nsIURI* aURI,
                                            PRTime aVisitTime,
                                            const nsACString& aGUID,
                                            uint16_t aReason,
                                            uint32_t aTransitionType)
{
  NS_PRECONDITION(mOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY,
                  "Bookmarks queries should not get a OnDeleteVisits notification");
  if (aVisitTime == 0) {
    
    
    
    nsresult rv = OnDeleteURI(aURI, aGUID, aReason);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (aTransitionType > 0) {
    
    
    
    if (mTransitions.Length() > 0 && mTransitions.Contains(aTransitionType)) {
      nsresult rv = OnDeleteURI(aURI, aGUID, aReason);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsNavHistoryQueryResultNode::NotifyIfTagsChanged(nsIURI* aURI)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);
  nsAutoCString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  bool onlyOneEntry = (mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_URI ||
                         mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS
                         );

  
  nsRefPtr<nsNavHistoryResultNode> node;
  nsNavHistory* history = nsNavHistory::GetHistoryService();

  nsCOMArray<nsNavHistoryResultNode> matches;
  RecursiveFindURIs(onlyOneEntry, this, spec, &matches);

  if (matches.Count() == 0 && mHasSearchTerms) {
    
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->URIToResultNode(aURI, mOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
    if (history->EvaluateQueryForNode(mQueries, mOptions, node)) {
      rv = InsertSortedChild(node);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  for (int32_t i = 0; i < matches.Count(); ++i) {
    nsNavHistoryResultNode* node = matches[i];
    
    node->mTags.SetIsVoid(true);
    nsAutoString tags;
    rv = node->GetTags(tags);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    if (mHasSearchTerms &&
        !history->EvaluateQueryForNode(mQueries, mOptions, node)) {
      nsNavHistoryContainerResultNode* parent = node->mParent;
      
      NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);
      int32_t childIndex = parent->FindChild(node);
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
nsNavHistoryQueryResultNode::OnItemAdded(int64_t aItemId,
                                         int64_t aParentId,
                                         int32_t aIndex,
                                         uint16_t aItemType,
                                         nsIURI* aURI,
                                         const nsACString& aTitle,
                                         PRTime aDateAdded,
                                         const nsACString& aGUID,
                                         const nsACString& aParentGUID)
{
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK &&
      mLiveUpdate != QUERYUPDATE_SIMPLE &&  mLiveUpdate != QUERYUPDATE_TIME) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemRemoved(int64_t aItemId,
                                           int64_t aParentId,
                                           int32_t aIndex,
                                           uint16_t aItemType,
                                           nsIURI* aURI,
                                           const nsACString& aGUID,
                                           const nsACString& aParentGUID)
{
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK &&
      mLiveUpdate != QUERYUPDATE_SIMPLE && mLiveUpdate != QUERYUPDATE_TIME) {
    nsresult rv = Refresh();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemChanged(int64_t aItemId,
                                           const nsACString& aProperty,
                                           bool aIsAnnotationProperty,
                                           const nsACString& aNewValue,
                                           PRTime aLastModified,
                                           uint16_t aItemType,
                                           int64_t aParentId,
                                           const nsACString& aGUID,
                                           const nsACString& aParentGUID)
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
                                               aNewValue, aLastModified,
                                               aItemType, aParentId, aGUID,
                                               aParentGUID);
}

NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemVisited(int64_t aItemId,
                                           int64_t aVisitId,
                                           PRTime aTime,
                                           uint32_t aTransitionType,
                                           nsIURI* aURI,
                                           int64_t aParentId,
                                           const nsACString& aGUID,
                                           const nsACString& aParentGUID)
{
  
  
  if (mLiveUpdate != QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    NS_WARN_IF_FALSE(mResult && (mResult->mIsAllBookmarksObserver || mResult->mIsBookmarkFolderObserver),
                     "history observers should not get OnItemVisited, but should get OnVisit instead");
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemMoved(int64_t aFolder,
                                         int64_t aOldParent,
                                         int32_t aOldIndex,
                                         int64_t aNewParent,
                                         int32_t aNewIndex,
                                         uint16_t aItemType,
                                         const nsACString& aGUID,
                                         const nsACString& aOldParentGUID,
                                         const nsACString& aNewParentGUID)
{
  
  
  
  
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS &&
      aItemType != nsINavBookmarksService::TYPE_SEPARATOR &&
      aOldParent != aNewParent) {
    return Refresh();
  }
  return NS_OK;
}



























NS_IMPL_ISUPPORTS_INHERITED(nsNavHistoryFolderResultNode,
                            nsNavHistoryContainerResultNode,
                            nsINavHistoryQueryResultNode)

nsNavHistoryFolderResultNode::nsNavHistoryFolderResultNode(
    const nsACString& aTitle, nsNavHistoryQueryOptions* aOptions,
    int64_t aFolderId) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, EmptyCString(),
                                  nsNavHistoryResultNode::RESULT_TYPE_FOLDER,
                                  aOptions),
  mContentsValid(false),
  mTargetFolderItemId(aFolderId),
  mIsRegisteredFolderObserver(false)
{
  mItemId = aFolderId;
}

nsNavHistoryFolderResultNode::~nsNavHistoryFolderResultNode()
{
  if (mIsRegisteredFolderObserver && mResult)
    mResult->RemoveBookmarkFolderObserver(this, mTargetFolderItemId);
}








void
nsNavHistoryFolderResultNode::OnRemoving()
{
  nsNavHistoryResultNode::OnRemoving();
  ClearChildren(true);
  mResult = nullptr;
}


nsresult
nsNavHistoryFolderResultNode::OpenContainer()
{
  NS_ASSERTION(!mExpanded, "Container must be expanded to close it");
  nsresult rv;

  if (!mContentsValid) {
    rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  mExpanded = true;

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
nsNavHistoryFolderResultNode::GetHasChildren(bool* aHasChildren)
{
  if (!mContentsValid) {
    nsresult rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  *aHasChildren = (mChildren.Count() > 0);
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetFolderItemId(int64_t* aItemId)
{
  *aItemId = mTargetFolderItemId;
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetTargetFolderGuid(nsACString& aGuid) {
  aGuid = mTargetFolderGuid;
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetUri(nsACString& aURI)
{
  if (!mURI.IsEmpty()) {
    aURI = mURI;
    return NS_OK;
  }

  uint32_t queryCount;
  nsINavHistoryQuery** queries;
  nsresult rv = GetQueries(&queryCount, &queries);
  NS_ENSURE_SUCCESS(rv, rv);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  rv = history->QueriesToQueryString(queries, queryCount, mOptions, aURI);
  for (uint32_t queryIndex = 0; queryIndex < queryCount; ++queryIndex) {
    NS_RELEASE(queries[queryIndex]);
  }
  free(queries);
  return rv;
}





NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetQueries(uint32_t* queryCount,
                                         nsINavHistoryQuery*** queries)
{
  
  nsCOMPtr<nsINavHistoryQuery> query;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = history->GetNewQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = query->SetFolders(&mTargetFolderItemId, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  
  *queries = static_cast<nsINavHistoryQuery**>
                        (moz_xmalloc(sizeof(nsINavHistoryQuery*)));
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

  
  nsresult rv = bookmarks->QueryFolderChildren(mTargetFolderItemId, mOptions, &mChildren);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  

  return OnChildrenFilled();
}






nsresult
nsNavHistoryFolderResultNode::OnChildrenFilled()
{
  
  
  FillStats();

  if (mResult && mResult->mNeedsToApplySortingMode) {
    
    
    mResult->SetSortingMode(mResult->mSortingMode);
  }
  else {
    
    
    SortComparator comparator = GetSortingComparator(GetSortType());
    if (comparator) {
      nsAutoCString sortingAnnotation;
      GetSortingAnnotation(sortingAnnotation);
      RecursiveSort(sortingAnnotation.get(), comparator);
    }
  }

  
  
  
  if (!mParent && mOptions->MaxResults()) {
    while ((uint32_t)mChildren.Count() > mOptions->MaxResults())
      mChildren.RemoveObjectAt(mChildren.Count() - 1);
  }

  
  EnsureRegisteredAsFolderObserver();

  mContentsValid = true;
  return NS_OK;
}






void
nsNavHistoryFolderResultNode::EnsureRegisteredAsFolderObserver()
{
  if (!mIsRegisteredFolderObserver && mResult) {
    mResult->AddBookmarkFolderObserver(this, mTargetFolderItemId);
    mIsRegisteredFolderObserver = true;
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
    bmSvc->QueryFolderChildrenAsync(this, mTargetFolderItemId,
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
    CancelAsyncOpen(false);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsCOMPtr<mozIStorageRow> row;
  while (NS_SUCCEEDED(aResultSet->GetNextRow(getter_AddRefs(row))) && row) {
    nsresult rv = bmSvc->ProcessFolderNodeRow(row, mOptions, &mChildren,
                                              mAsyncBookmarkIndex);
    if (NS_FAILED(rv)) {
      CancelAsyncOpen(false);
      return rv;
    }
  }

  return NS_OK;
}









NS_IMETHODIMP
nsNavHistoryFolderResultNode::HandleCompletion(uint16_t aReason)
{
  if (aReason == mozIStorageStatementCallback::REASON_FINISHED &&
      mAsyncCanceledState == NOT_CANCELED) {
    

    nsresult rv = OnChildrenFilled();
    NS_ENSURE_SUCCESS(rv, rv);

    mExpanded = true;
    mAsyncPendingStmt = nullptr;

    
    rv = NotifyOnStateChange(STATE_LOADING);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  else if (mAsyncCanceledState == CANCELED_RESTART_NEEDED) {
    
    mAsyncCanceledState = NOT_CANCELED;
    ClearChildren(false);
    FillChildrenAsync();
  }

  else {
    
    
    mAsyncCanceledState = NOT_CANCELED;
    ClearChildren(true);
    CloseContainer();
  }

  return NS_OK;
}


void
nsNavHistoryFolderResultNode::ClearChildren(bool unregister)
{
  for (int32_t i = 0; i < mChildren.Count(); ++i)
    mChildren[i]->OnRemoving();
  mChildren.Clear();

  bool needsUnregister = unregister && (mContentsValid || mAsyncPendingStmt);
  if (needsUnregister && mResult && mIsRegisteredFolderObserver) {
    mResult->RemoveBookmarkFolderObserver(this, mTargetFolderItemId);
    mIsRegisteredFolderObserver = false;
  }
  mContentsValid = false;
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

  ClearChildren(true);

  if (!mExpanded) {
    
    return NS_OK;
  }

  
  
  
  
  (void)FillChildren();

  NOTIFY_RESULT_OBSERVERS(result, InvalidateContainer(TO_CONTAINER(this)));
  return NS_OK;
}







bool
nsNavHistoryFolderResultNode::StartIncrementalUpdate()
{
  
  

  if (!mOptions->ExcludeItems() &&
      !mOptions->ExcludeQueries() &&
      !mOptions->ExcludeReadOnlyFolders()) {
    
    if (mExpanded || AreChildrenVisible())
      return true;

    nsNavHistoryResult* result = GetResult();
    NS_ENSURE_TRUE(result, false);

    
    
    if (mParent)
      return result->mObservers.Length() > 0;
  }

  
  (void)Refresh();
  return false;
}







void
nsNavHistoryFolderResultNode::ReindexRange(int32_t aStartIndex,
                                           int32_t aEndIndex,
                                           int32_t aDelta)
{
  for (int32_t i = 0; i < mChildren.Count(); ++i) {
    nsNavHistoryResultNode* node = mChildren[i];
    if (node->mBookmarkIndex >= aStartIndex &&
        node->mBookmarkIndex <= aEndIndex)
      node->mBookmarkIndex += aDelta;
  }
}








nsNavHistoryResultNode*
nsNavHistoryFolderResultNode::FindChildById(int64_t aItemId,
    uint32_t* aNodeIndex)
{
  for (int32_t i = 0; i < mChildren.Count(); ++i) {
    if (mChildren[i]->mItemId == aItemId ||
        (mChildren[i]->IsFolder() &&
         mChildren[i]->GetAsFolder()->mTargetFolderItemId == aItemId)) {
      *aNodeIndex = i;
      return mChildren[i];
    }
  }
  return nullptr;
}





#define RESTART_AND_RETURN_IF_ASYNC_PENDING() \
  if (mAsyncPendingStmt) { \
    CancelAsyncOpen(true); \
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
nsNavHistoryFolderResultNode::OnItemAdded(int64_t aItemId,
                                          int64_t aParentFolder,
                                          int32_t aIndex,
                                          uint16_t aItemType,
                                          nsIURI* aURI,
                                          const nsACString& aTitle,
                                          PRTime aDateAdded,
                                          const nsACString& aGUID,
                                          const nsACString& aParentGUID)
{
  MOZ_ASSERT(aParentFolder == mTargetFolderItemId, "Got wrong bookmark update");

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  {
    uint32_t index;
    nsNavHistoryResultNode* node = FindChildById(aItemId, &index);
    
    
    
    
    
    if (node)
      return NS_OK;
  }

  bool excludeItems = (mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
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

  nsresult rv;

  
  
  bool isQuery = false;
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK) {
    NS_ASSERTION(aURI, "Got a null URI when we are a bookmark?!");
    nsAutoCString itemURISpec;
    rv = aURI->GetSpec(itemURISpec);
    NS_ENSURE_SUCCESS(rv, rv);
    isQuery = IsQueryURI(itemURISpec);
  }

  if (aItemType != nsINavBookmarksService::TYPE_FOLDER &&
      !isQuery && excludeItems) {
    
    
    ReindexRange(aIndex, INT32_MAX, 1);
    return NS_OK;
  }

  if (!StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex, INT32_MAX, 1);

  nsRefPtr<nsNavHistoryResultNode> node;
  if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->BookmarkIdToResultNode(aItemId, mOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aItemType == nsINavBookmarksService::TYPE_FOLDER) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
    rv = bookmarks->ResultNodeForContainer(aItemId, mOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aItemType == nsINavBookmarksService::TYPE_SEPARATOR) {
    node = new nsNavHistorySeparatorResultNode();
    NS_ENSURE_TRUE(node, NS_ERROR_OUT_OF_MEMORY);
    node->mItemId = aItemId;
    node->mBookmarkGuid = aGUID;
    node->mDateAdded = aDateAdded;
    node->mLastModified = aDateAdded;
  }

  node->mBookmarkIndex = aIndex;

  if (aItemType == nsINavBookmarksService::TYPE_SEPARATOR ||
      GetSortType() == nsINavHistoryQueryOptions::SORT_BY_NONE) {
    
    return InsertChildAt(node, aIndex);
  }

  
  return InsertSortedChild(node);
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemRemoved(int64_t aItemId,
                                            int64_t aParentFolder,
                                            int32_t aIndex,
                                            uint16_t aItemType,
                                            nsIURI* aURI,
                                            const nsACString& aGUID,
                                            const nsACString& aParentGUID)
{
  
  
  MOZ_ASSERT(mItemId == mTargetFolderItemId || aItemId != mItemId);

  
  if (mTargetFolderItemId == aItemId)
    return NS_OK;

  MOZ_ASSERT(aParentFolder == mTargetFolderItemId, "Got wrong bookmark update");

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  
  
  
  uint32_t index;
  nsNavHistoryResultNode* node = FindChildById(aItemId, &index);
    
    
    
    
    
  if (!node) {
    return NS_OK;
  }

  bool excludeItems = (mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
                        (mParent && mParent->mOptions->ExcludeItems()) ||
                        mOptions->ExcludeItems();
  if ((node->IsURI() || node->IsSeparator()) && excludeItems) {
    
    
    ReindexRange(aIndex, INT32_MAX, -1);
    return NS_OK;
  }

  if (!StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex + 1, INT32_MAX, -1);

  return RemoveChildAt(index);
}


NS_IMETHODIMP
nsNavHistoryResultNode::OnItemChanged(int64_t aItemId,
                                      const nsACString& aProperty,
                                      bool aIsAnnotationProperty,
                                      const nsACString& aNewValue,
                                      PRTime aLastModified,
                                      uint16_t aItemType,
                                      int64_t aParentId,
                                      const nsACString& aGUID,
                                      const nsACString& aParentGUID)
{
  if (aItemId != mItemId)
    return NS_OK;

  mLastModified = aLastModified;

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_STATE(result);

  bool shouldNotify = !mParent || mParent->AreChildrenVisible();

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
    
    mTags.SetIsVoid(true);
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
    mTags.SetIsVoid(true);
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

  
  
  
  int32_t ourIndex = mParent->FindChild(this);
  NS_ASSERTION(ourIndex >= 0, "Could not find self in parent");
  if (ourIndex >= 0)
    mParent->EnsureItemPosition(ourIndex);

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemChanged(int64_t aItemId,
                                            const nsACString& aProperty,
                                            bool aIsAnnotationProperty,
                                            const nsACString& aNewValue,
                                            PRTime aLastModified,
                                            uint16_t aItemType,
                                            int64_t aParentId,
                                            const nsACString& aGUID,
                                            const nsACString&aParentGUID)
{
  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  return nsNavHistoryResultNode::OnItemChanged(aItemId, aProperty,
                                               aIsAnnotationProperty,
                                               aNewValue, aLastModified,
                                               aItemType, aParentId, aGUID,
                                               aParentGUID);
}




NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemVisited(int64_t aItemId,
                                            int64_t aVisitId,
                                            PRTime aTime,
                                            uint32_t aTransitionType,
                                            nsIURI* aURI,
                                            int64_t aParentId,
                                            const nsACString& aGUID,
                                            const nsACString& aParentGUID)
{
  bool excludeItems = (mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
                        (mParent && mParent->mOptions->ExcludeItems()) ||
                        mOptions->ExcludeItems();
  if (excludeItems)
    return NS_OK; 

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  if (!StartIncrementalUpdate())
    return NS_OK;

  uint32_t nodeIndex;
  nsNavHistoryResultNode* node = FindChildById(aItemId, &nodeIndex);
  if (!node)
    return NS_ERROR_FAILURE;

  
  node->mTime = aTime;
  ++node->mAccessCount;

  
  int32_t oldAccessCount = mAccessCount;
  ++mAccessCount;
  if (aTime > mTime)
    mTime = aTime;
  nsresult rv = ReverseUpdateStats(mAccessCount - oldAccessCount);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_OK);
  nsRefPtr<nsNavHistoryResultNode> visitNode;
  rv = history->VisitIdToResultNode(aVisitId, mOptions,
                                    getter_AddRefs(visitNode));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_STATE(visitNode);
  node->mFrecency = visitNode->mFrecency;

  if (AreChildrenVisible()) {
    
    nsNavHistoryResult* result = GetResult();
    NOTIFY_RESULT_OBSERVERS(result,
                            NodeHistoryDetailsChanged(node, mTime, mAccessCount));
  }

  
  uint32_t sortType = GetSortType();
  if (sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_FRECENCY_ASCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING) {
    int32_t childIndex = FindChild(node);
    NS_ASSERTION(childIndex >= 0, "Could not find child we just got a reference to");
    if (childIndex >= 0) {
      EnsureItemPosition(childIndex);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemMoved(int64_t aItemId,
                                          int64_t aOldParent,
                                          int32_t aOldIndex,
                                          int64_t aNewParent,
                                          int32_t aNewIndex,
                                          uint16_t aItemType,
                                          const nsACString& aGUID,
                                          const nsACString& aOldParentGUID,
                                          const nsACString& aNewParentGUID)
{
  NS_ASSERTION(aOldParent == mTargetFolderItemId || aNewParent == mTargetFolderItemId,
               "Got a bookmark message that doesn't belong to us");

  RESTART_AND_RETURN_IF_ASYNC_PENDING();

  uint32_t index;
  nsNavHistoryResultNode* node = FindChildById(aItemId, &index);
  
  
  
  
  
  if (node && aNewParent == mTargetFolderItemId && index == static_cast<uint32_t>(aNewIndex))
    return NS_OK;
  if (!node && aOldParent == mTargetFolderItemId)
    return NS_OK;

  bool excludeItems = (mResult && mResult->mRootNode->mOptions->ExcludeItems()) ||
                      (mParent && mParent->mOptions->ExcludeItems()) ||
                      mOptions->ExcludeItems();
  if (node && excludeItems && (node->IsURI() || node->IsSeparator())) {
    
    return NS_OK;
  }

  if (!StartIncrementalUpdate())
    return NS_OK; 

  if (aOldParent == aNewParent) {
    
    

    
    ReindexRange(aOldIndex + 1, INT32_MAX, -1);
    ReindexRange(aNewIndex, INT32_MAX, 1);

    MOZ_ASSERT(node, "Can't find folder that is moving!");
    if (!node) {
      return NS_ERROR_FAILURE;
    }
    MOZ_ASSERT(index < uint32_t(mChildren.Count()), "Invalid index!");
    node->mBookmarkIndex = aNewIndex;

    
    EnsureItemPosition(index);
    return NS_OK;
  } else {
    
    nsCOMPtr<nsIURI> itemURI;
    nsAutoCString itemTitle;
    if (aItemType == nsINavBookmarksService::TYPE_BOOKMARK) {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      nsresult rv = bookmarks->GetBookmarkURI(aItemId, getter_AddRefs(itemURI));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = bookmarks->GetItemTitle(aItemId, itemTitle);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (aOldParent == mTargetFolderItemId) {
      OnItemRemoved(aItemId, aOldParent, aOldIndex, aItemType, itemURI,
                    aGUID, aOldParentGUID);
    }
    if (aNewParent == mTargetFolderItemId) {
      OnItemAdded(aItemId, aNewParent, aNewIndex, aItemType, itemURI, itemTitle,
                  RoundedPRNow(), 
                  aGUID, aNewParentGUID);
    }
  }
  return NS_OK;
}





nsNavHistorySeparatorResultNode::nsNavHistorySeparatorResultNode()
  : nsNavHistoryResultNode(EmptyCString(), EmptyCString(),
                           0, 0, EmptyCString())
{
}


static PLDHashOperator
RemoveBookmarkFolderObserversCallback(nsTrimInt64HashKey::KeyType aKey,
                                      nsNavHistoryResult::FolderObserverList*& aData,
                                      void* userArg)
{
  delete aData;
  return PL_DHASH_REMOVE;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsNavHistoryResult)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsNavHistoryResult)
  tmp->StopObserving();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRootNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mObservers)
  tmp->mBookmarkFolderObservers.Enumerate(&RemoveBookmarkFolderObserversCallback, nullptr);
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mAllBookmarksObservers)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mHistoryObservers)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

static PLDHashOperator
TraverseBookmarkFolderObservers(nsTrimInt64HashKey::KeyType aKey,
                                nsNavHistoryResult::FolderObserverList* &aData,
                                void *aClosure)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  for (uint32_t i = 0; i < aData->Length(); ++i) {
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
  for (uint32_t i = 0; i < aObservers.Length(); ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mResultObservers value[i]");
    const nsCOMPtr<nsINavHistoryResultObserver> &obs = aObservers.ElementAt(i);
    cb->NoteXPCOMChild(obs);
  }
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsNavHistoryResult)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRootNode)
  traverseResultObservers(tmp->mObservers, &cb);
  tmp->mBookmarkFolderObservers.Enumerate(&TraverseBookmarkFolderObservers, &cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAllBookmarksObservers)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mHistoryObservers)
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
  , mNeedsToApplySortingMode(false)
  , mIsHistoryObserver(false)
  , mIsBookmarkFolderObserver(false)
  , mIsAllBookmarksObserver(false)
  , mBookmarkFolderObservers(64)
  , mBatchInProgress(false)
  , mSuppressNotifications(false)
{
  mRootNode->mResult = this;
}

nsNavHistoryResult::~nsNavHistoryResult()
{
  
  mBookmarkFolderObservers.Enumerate(&RemoveBookmarkFolderObserversCallback, nullptr);
}

void
nsNavHistoryResult::StopObserving()
{
  if (mIsBookmarkFolderObserver || mIsAllBookmarksObserver) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    if (bookmarks) {
      bookmarks->RemoveObserver(this);
      mIsBookmarkFolderObserver = false;
      mIsAllBookmarksObserver = false;
    }
  }
  if (mIsHistoryObserver) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    if (history) {
      history->RemoveObserver(this);
      mIsHistoryObserver = false;
    }
  }
}





nsresult
nsNavHistoryResult::Init(nsINavHistoryQuery** aQueries,
                         uint32_t aQueryCount,
                         nsNavHistoryQueryOptions *aOptions)
{
  nsresult rv;
  NS_ASSERTION(aOptions, "Must have valid options");
  NS_ASSERTION(aQueries && aQueryCount > 0, "Must have >1 query in result");

  
  
  
  for (uint32_t i = 0; i < aQueryCount; ++i) {
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

  NS_ASSERTION(mRootNode->mIndentLevel == -1,
               "Root node's indent level initialized wrong");
  mRootNode->FillStats();

  return NS_OK;
}





nsresult 
nsNavHistoryResult::NewHistoryResult(nsINavHistoryQuery** aQueries,
                                     uint32_t aQueryCount,
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
    *result = nullptr;
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
      history->AddObserver(this, true);
      mIsHistoryObserver = true;
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
    bookmarks->AddObserver(this, true);
    mIsAllBookmarksObserver = true;
  }
  
  
  
  if (mAllBookmarksObservers.IndexOf(aNode) == mAllBookmarksObservers.NoIndex) {
    mAllBookmarksObservers.AppendElement(aNode);
  }
}


void
nsNavHistoryResult::AddBookmarkFolderObserver(nsNavHistoryFolderResultNode* aNode,
                                              int64_t aFolder)
{
  if (!mIsBookmarkFolderObserver && !mIsAllBookmarksObserver) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    if (!bookmarks) {
      NS_NOTREACHED("Can't create bookmark service");
      return;
    }
    bookmarks->AddObserver(this, true);
    mIsBookmarkFolderObserver = true;
  }
  
  
  
  FolderObserverList* list = BookmarkFolderObserversForId(aFolder, true);
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
                                                 int64_t aFolder)
{
  FolderObserverList* list = BookmarkFolderObserversForId(aFolder, false);
  if (!list)
    return; 
  list->RemoveElement(aNode);
}


nsNavHistoryResult::FolderObserverList*
nsNavHistoryResult::BookmarkFolderObserversForId(int64_t aFolderId, bool aCreate)
{
  FolderObserverList* list;
  if (mBookmarkFolderObservers.Get(aFolderId, &list))
    return list;
  if (!aCreate)
    return nullptr;

  
  list = new FolderObserverList;
  mBookmarkFolderObservers.Put(aFolderId, list);
  return list;
}


NS_IMETHODIMP
nsNavHistoryResult::GetSortingMode(uint16_t* aSortingMode)
{
  *aSortingMode = mSortingMode;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::SetSortingMode(uint16_t aSortingMode)
{
  NS_ENSURE_STATE(mRootNode);

  if (aSortingMode > nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING)
    return NS_ERROR_INVALID_ARG;

  
  NS_ASSERTION(mOptions, "Options should always be present for a root query");

  mSortingMode = aSortingMode;

  if (!mRootNode->mExpanded) {
    
    mNeedsToApplySortingMode = true;
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
                                bool aOwnsWeak)
{
  NS_ENSURE_ARG(aObserver);
  nsresult rv = mObservers.AppendWeakElement(aObserver, aOwnsWeak);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aObserver->SetResult(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (mBatchInProgress) {
    NOTIFY_RESULT_OBSERVERS(this, Batching(true));
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
nsNavHistoryResult::GetSuppressNotifications(bool* _retval)
{
  *_retval = mSuppressNotifications;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::SetSuppressNotifications(bool aSuppressNotifications)
{
  mSuppressNotifications = aSuppressNotifications;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::GetRoot(nsINavHistoryContainerResultNode** aRoot)
{
  if (!mRootNode) {
    NS_NOTREACHED("Root is null");
    *aRoot = nullptr;
    return NS_ERROR_FAILURE;
  }
  nsRefPtr<nsNavHistoryContainerResultNode> node(mRootNode);
  node.forget(aRoot);
  return NS_OK;
}


void
nsNavHistoryResult::requestRefresh(nsNavHistoryContainerResultNode* aContainer)
{
  
  if (mRefreshParticipants.IndexOf(aContainer) == mRefreshParticipants.NoIndex)
    mRefreshParticipants.AppendElement(aContainer);
}






#define ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(_folderId, _functionCall) \
  PR_BEGIN_MACRO \
    FolderObserverList* _fol = BookmarkFolderObserversForId(_folderId, false); \
    if (_fol) { \
      FolderObserverList _listCopy(*_fol); \
      for (uint32_t _fol_i = 0; _fol_i < _listCopy.Length(); ++_fol_i) { \
        if (_listCopy[_fol_i]) \
          _listCopy[_fol_i]->_functionCall; \
      } \
    } \
  PR_END_MACRO
#define ENUMERATE_LIST_OBSERVERS(_listType, _functionCall, _observersList, _conditionCall) \
  PR_BEGIN_MACRO \
    _listType _listCopy(_observersList); \
    for (uint32_t _obs_i = 0; _obs_i < _listCopy.Length(); ++_obs_i) { \
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
    mBatchInProgress = true;
    ENUMERATE_HISTORY_OBSERVERS(OnBeginUpdateBatch());
    ENUMERATE_ALL_BOOKMARKS_OBSERVERS(OnBeginUpdateBatch());

    NOTIFY_RESULT_OBSERVERS(this, Batching(true));
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnEndUpdateBatch()
{
  
  
  
  
  
  if (mBatchInProgress) {
    ENUMERATE_HISTORY_OBSERVERS(OnEndUpdateBatch());
    ENUMERATE_ALL_BOOKMARKS_OBSERVERS(OnEndUpdateBatch());

    
    
    
    mBatchInProgress = false;
    NOTIFY_REFRESH_PARTICIPANTS();
    NOTIFY_RESULT_OBSERVERS(this, Batching(false));
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemAdded(int64_t aItemId,
                                int64_t aParentId,
                                int32_t aIndex,
                                uint16_t aItemType,
                                nsIURI* aURI,
                                const nsACString& aTitle,
                                PRTime aDateAdded,
                                const nsACString& aGUID,
                                const nsACString& aParentGUID)
{
  ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aParentId,
    OnItemAdded(aItemId, aParentId, aIndex, aItemType, aURI, aTitle, aDateAdded,
                aGUID, aParentGUID)
  );
  ENUMERATE_HISTORY_OBSERVERS(
    OnItemAdded(aItemId, aParentId, aIndex, aItemType, aURI, aTitle, aDateAdded,
                aGUID, aParentGUID)
  );
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
    OnItemAdded(aItemId, aParentId, aIndex, aItemType, aURI, aTitle, aDateAdded,
                aGUID, aParentGUID)
  );
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemRemoved(int64_t aItemId,
                                  int64_t aParentId,
                                  int32_t aIndex,
                                  uint16_t aItemType,
                                  nsIURI* aURI,
                                  const nsACString& aGUID,
                                  const nsACString& aParentGUID)
{
  ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aParentId,
      OnItemRemoved(aItemId, aParentId, aIndex, aItemType, aURI, aGUID,
                    aParentGUID));
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
      OnItemRemoved(aItemId, aParentId, aIndex, aItemType, aURI, aGUID,
                    aParentGUID));
  ENUMERATE_HISTORY_OBSERVERS(
      OnItemRemoved(aItemId, aParentId, aIndex, aItemType, aURI, aGUID,
                    aParentGUID));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemChanged(int64_t aItemId,
                                  const nsACString &aProperty,
                                  bool aIsAnnotationProperty,
                                  const nsACString &aNewValue,
                                  PRTime aLastModified,
                                  uint16_t aItemType,
                                  int64_t aParentId,
                                  const nsACString& aGUID,
                                  const nsACString& aParentGUID)
{
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
    OnItemChanged(aItemId, aProperty, aIsAnnotationProperty, aNewValue,
                  aLastModified, aItemType, aParentId, aGUID, aParentGUID));

  
  
  

  FolderObserverList* list = BookmarkFolderObserversForId(aParentId, false);
  if (!list)
    return NS_OK;

  for (uint32_t i = 0; i < list->Length(); ++i) {
    nsRefPtr<nsNavHistoryFolderResultNode> folder = list->ElementAt(i);
    if (folder) {
      uint32_t nodeIndex;
      nsRefPtr<nsNavHistoryResultNode> node =
        folder->FindChildById(aItemId, &nodeIndex);
      
      bool excludeItems = (mRootNode->mOptions->ExcludeItems()) ||
                             folder->mOptions->ExcludeItems();
      if (node &&
          (!excludeItems || !(node->IsURI() || node->IsSeparator())) &&
          folder->StartIncrementalUpdate()) {
        node->OnItemChanged(aItemId, aProperty, aIsAnnotationProperty,
                            aNewValue, aLastModified, aItemType, aParentId,
                            aGUID, aParentGUID);
      }
    }
  }

  
  
  
  
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnItemVisited(int64_t aItemId,
                                  int64_t aVisitId,
                                  PRTime aVisitTime,
                                  uint32_t aTransitionType,
                                  nsIURI* aURI,
                                  int64_t aParentId,
                                  const nsACString& aGUID,
                                  const nsACString& aParentGUID)
{
  ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aParentId,
      OnItemVisited(aItemId, aVisitId, aVisitTime, aTransitionType, aURI,
                    aParentId, aGUID, aParentGUID));
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(
      OnItemVisited(aItemId, aVisitId, aVisitTime, aTransitionType, aURI,
                    aParentId, aGUID, aParentGUID));
  
  
  
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryResult::OnItemMoved(int64_t aItemId,
                                int64_t aOldParent,
                                int32_t aOldIndex,
                                int64_t aNewParent,
                                int32_t aNewIndex,
                                uint16_t aItemType,
                                const nsACString& aGUID,
                                const nsACString& aOldParentGUID,
                                const nsACString& aNewParentGUID)
{
  ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aOldParent,
      OnItemMoved(aItemId, aOldParent, aOldIndex, aNewParent, aNewIndex,
                  aItemType, aGUID, aOldParentGUID, aNewParentGUID));
  if (aNewParent != aOldParent) {
    ENUMERATE_BOOKMARK_FOLDER_OBSERVERS(aNewParent,
        OnItemMoved(aItemId, aOldParent, aOldIndex, aNewParent, aNewIndex,
                    aItemType, aGUID, aOldParentGUID, aNewParentGUID));
  }
  ENUMERATE_ALL_BOOKMARKS_OBSERVERS(OnItemMoved(aItemId, aOldParent, aOldIndex,
                                                aNewParent, aNewIndex,
                                                aItemType, aGUID,
                                                aOldParentGUID,
                                                aNewParentGUID));
  ENUMERATE_HISTORY_OBSERVERS(OnItemMoved(aItemId, aOldParent, aOldIndex,
                                          aNewParent, aNewIndex, aItemType,
                                          aGUID, aOldParentGUID,
                                          aNewParentGUID));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnVisit(nsIURI* aURI, int64_t aVisitId, PRTime aTime,
                            int64_t aSessionId, int64_t aReferringId,
                            uint32_t aTransitionType, const nsACString& aGUID,
                            bool aHidden)
{
  uint32_t added = 0;

  ENUMERATE_HISTORY_OBSERVERS(OnVisit(aURI, aVisitId, aTime, aSessionId,
                                      aReferringId, aTransitionType, aGUID,
                                      aHidden, &added));

  if (!mRootNode->mExpanded)
    return NS_OK;

  
  
  
  bool todayIsMissing = false;
  uint32_t resultType = mRootNode->mOptions->ResultType();
  if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
      resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY) {
    uint32_t childCount;
    nsresult rv = mRootNode->GetChildCount(&childCount);
    NS_ENSURE_SUCCESS(rv, rv);
    if (childCount) {
      nsCOMPtr<nsINavHistoryResultNode> firstChild;
      rv = mRootNode->GetChild(0, getter_AddRefs(firstChild));
      NS_ENSURE_SUCCESS(rv, rv);
      nsAutoCString title;
      rv = firstChild->GetTitle(title);
      NS_ENSURE_SUCCESS(rv, rv);
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_OK);
      nsAutoCString todayLabel;
      history->GetStringFromName(
        MOZ_UTF16("finduri-AgeInDays-is-0"), todayLabel);
      todayIsMissing = !todayLabel.Equals(title);
    }
  }

  if (!added || todayIsMissing) {
    
    
    uint32_t resultType = mRootNode->mOptions->ResultType();
    if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
        resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY) {
      
      
      int64_t beginOfToday =
        nsNavHistory::NormalizeTime(nsINavHistoryQuery::TIME_RELATIVE_TODAY, 0);
      if (todayIsMissing || aTime < beginOfToday) {
        (void)mRootNode->GetAsQuery()->Refresh();
      }
      return NS_OK;
    }

    if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY) {
      (void)mRootNode->GetAsQuery()->Refresh();
      return NS_OK;
    }

    
    
    
    
    ENUMERATE_QUERY_OBSERVERS(Refresh(), mHistoryObservers, IsContainersQuery());
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnTitleChanged(nsIURI* aURI,
                                   const nsAString& aPageTitle,
                                   const nsACString& aGUID)
{
  ENUMERATE_HISTORY_OBSERVERS(OnTitleChanged(aURI, aPageTitle, aGUID));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnFrecencyChanged(nsIURI* aURI,
                                      int32_t aNewFrecency,
                                      const nsACString& aGUID,
                                      bool aHidden,
                                      PRTime aLastVisitDate)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnManyFrecenciesChanged()
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnDeleteURI(nsIURI *aURI,
                                const nsACString& aGUID,
                                uint16_t aReason)
{
  ENUMERATE_HISTORY_OBSERVERS(OnDeleteURI(aURI, aGUID, aReason));
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnClearHistory()
{
  ENUMERATE_HISTORY_OBSERVERS(OnClearHistory());
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryResult::OnPageChanged(nsIURI* aURI,
                                  uint32_t aChangedAttribute,
                                  const nsAString& aValue,
                                  const nsACString& aGUID)
{
  ENUMERATE_HISTORY_OBSERVERS(OnPageChanged(aURI, aChangedAttribute, aValue, aGUID));
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistoryResult::OnDeleteVisits(nsIURI* aURI,
                                   PRTime aVisitTime,
                                   const nsACString& aGUID,
                                   uint16_t aReason,
                                   uint32_t aTransitionType)
{
  ENUMERATE_HISTORY_OBSERVERS(OnDeleteVisits(aURI, aVisitTime, aGUID, aReason,
                                             aTransitionType));
  return NS_OK;
}
