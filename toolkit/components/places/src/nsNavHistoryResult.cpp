






































#include <stdio.h>
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"

#include "nsIArray.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsFaviconService.h"
#include "nsHashPropertyBag.h"
#include "nsIComponentManager.h"
#include "nsIDateTimeFormat.h"
#include "nsIDOMElement.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "nsILocalFile.h"
#include "nsIRemoteContainer.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsITreeColumns.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIWritablePropertyBag.h"
#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsPromiseFlatString.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "prtime.h"
#include "prprf.h"
#include "mozStorageHelper.h"

#define ICONURI_QUERY "chrome://browser/skin/places/query.png"






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




NS_IMPL_ISUPPORTS2(nsNavHistoryResultNode,
                   nsNavHistoryResultNode, nsINavHistoryResultNode)

nsNavHistoryResultNode::nsNavHistoryResultNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI) :
  mParent(nsnull),
  mURI(aURI),
  mTitle(aTitle),
  mAccessCount(aAccessCount),
  mTime(aTime),
  mFaviconURI(aIconURI),
  mBookmarkIndex(-1),
  mBookmarkId(-1),
  mIndentLevel(-1),
  mViewIndex(-1)
{
}

NS_IMETHODIMP
nsNavHistoryResultNode::GetIcon(nsIURI** aURI)
{
  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_NO_INTERFACE);
  if (mFaviconURI.IsEmpty()) {
    *aURI = nsnull;
    return NS_OK;
  }
  return faviconService->GetFaviconLinkForIconString(mFaviconURI, aURI);
}

NS_IMETHODIMP
nsNavHistoryResultNode::GetParent(nsINavHistoryContainerResultNode** aParent)
{
  NS_IF_ADDREF(*aParent = mParent);
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryResultNode::GetPropertyBag(nsIWritablePropertyBag** aBag)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  return result->PropertyBagFor(this, aBag);
}






void
nsNavHistoryResultNode::OnRemoving()
{
  mParent = nsnull;
  mViewIndex = -1;
}








nsNavHistoryResult*
nsNavHistoryResultNode::GetResult()
{
  nsNavHistoryResultNode* node = this;
  do {
    if (node->IsContainer()) {
      nsNavHistoryContainerResultNode* container =
        NS_STATIC_CAST(nsNavHistoryContainerResultNode*, node);
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
  if (! mParent) {
    
    
    
    
    if (IsFolder())
      return GetAsFolder()->mOptions;
    else if (IsQuery())
      return GetAsQuery()->mOptions;
    NS_NOTREACHED("Can't find a generating node for this container, perhaps FillStats has not been called on this tree yet?");
    return nsnull;
  }

  nsNavHistoryContainerResultNode* cur = mParent;
  while (cur) {
    if (cur->IsFolder())
      return cur->GetAsFolder()->mOptions;
    else if (cur->IsQuery())
      return cur->GetAsQuery()->mOptions;
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



NS_IMPL_ADDREF_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)
NS_IMPL_RELEASE_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)

NS_INTERFACE_MAP_BEGIN(nsNavHistoryContainerResultNode)
  NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsNavHistoryContainerResultNode)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryContainerResultNode)
NS_INTERFACE_MAP_END_INHERITING(nsNavHistoryResultNode)

nsNavHistoryContainerResultNode::nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    const nsACString& aIconURI, PRUint32 aContainerType, PRBool aReadOnly,
    const nsACString& aRemoteContainerType) :
  nsNavHistoryResultNode(aURI, aTitle, 0, 0, aIconURI),
  mResult(nsnull),
  mContainerType(aContainerType),
  mExpanded(PR_FALSE),
  mChildrenReadOnly(aReadOnly),
  mRemoteContainerType(aRemoteContainerType)
{
}







void
nsNavHistoryContainerResultNode::OnRemoving()
{
  nsNavHistoryResultNode::OnRemoving();
  for (PRInt32 i = 0; i < mChildren.Count(); i ++)
    mChildren[i]->OnRemoving();
  mChildren.Clear();
}








PRBool
nsNavHistoryContainerResultNode::AreChildrenVisible()
{
  
  if (! mExpanded)
    return PR_FALSE;

  
  if (mViewIndex >= 0)
    return PR_TRUE;

  nsNavHistoryResult* result = GetResult();
  if (! result) {
    NS_NOTREACHED("Invalid result");
    return PR_FALSE;
  }
  if (result->mRootNode == this && result->mView)
    return PR_TRUE;
  return PR_FALSE;
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
  if (mExpanded && ! aContainerOpen)
    CloseContainer();
  else if (! mExpanded && aContainerOpen)
    OpenContainer();
  return NS_OK;
}







nsresult
nsNavHistoryContainerResultNode::OpenContainer()
{
  NS_ASSERTION(! mExpanded, "Container must be expanded to close it");
  mExpanded = PR_TRUE;

  
















  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    result->GetView()->ContainerOpened(this);
  return NS_OK;
}









nsresult
nsNavHistoryContainerResultNode::CloseContainer(PRBool aUpdateView)
{
  NS_ASSERTION(mExpanded, "Container must be expanded to close it");

  
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
    if (mChildren[i]->IsContainer() && mChildren[i]->GetAsContainer()->mExpanded)
      mChildren[i]->GetAsContainer()->CloseContainer(PR_FALSE);
  }

  mExpanded = PR_FALSE;

  









  if (aUpdateView) {
    nsNavHistoryResult* result = GetResult();
    NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
    if (result->GetView())
      result->GetView()->ContainerClosed(this);
  }
  return NS_OK;
}













void
nsNavHistoryContainerResultNode::FillStats()
{
  mAccessCount = 0;
  mTime = 0;
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
    nsNavHistoryResultNode* node = mChildren[i];
    node->mParent = this;
    node->mIndentLevel = mIndentLevel + 1;
    if (node->IsContainer()) {
      nsNavHistoryContainerResultNode* container = node->GetAsContainer();
      container->mResult = mResult;
      container->FillStats();
    }
    mAccessCount += node->mAccessCount;
    
    
    
    if (node->mTime > mTime)
      mTime = node->mTime;
  }
}




















void
nsNavHistoryContainerResultNode::ReverseUpdateStats(PRInt32 aAccessCountChange)
{
  if (mParent) {
    mParent->mAccessCount += aAccessCountChange;
    PRBool timeChanged = PR_FALSE;
    if (mTime > mParent->mTime) {
      timeChanged = PR_TRUE;
      mParent->mTime = mTime;
    }

    
    
    PRUint32 sortMode = mParent->GetSortType();
    PRBool resorted = PR_FALSE;
    if (((sortMode == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING ||
          sortMode == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING) &&
         aAccessCountChange != 0) ||
        ((sortMode == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING ||
          sortMode == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING) &&
         timeChanged)) {

      SortComparator comparator = GetSortingComparator(sortMode);
      int ourIndex = mParent->FindChild(this);
      if (mParent->DoesChildNeedResorting(ourIndex, comparator)) {
        
        nsRefPtr<nsNavHistoryContainerResultNode> ourLock = this;
        nsNavHistoryContainerResultNode* ourParent = mParent;

        
        
        
        
        ourParent->RemoveChildAt(ourIndex, PR_TRUE);
        ourParent->InsertSortedChild(this, PR_TRUE);
        resorted = PR_TRUE;
      }
    }
    if (! resorted) {
      
      nsNavHistoryResult* result = GetResult();
      if (result && result->GetView()) {
        result->GetView()->ItemChanged(NS_STATIC_CAST(
            nsINavHistoryContainerResultNode*, mParent));
      }
    }

    mParent->ReverseUpdateStats(aAccessCountChange);
  }
}









PRUint32
nsNavHistoryContainerResultNode::GetSortType()
{
  if (mParent)
    return mParent->GetSortType();
  else if (mResult)
    return mResult->mSortingMode;
  NS_NOTREACHED("We should always have a result");
  return nsINavHistoryQueryOptions::SORT_BY_NONE;
}







nsNavHistoryContainerResultNode::SortComparator
nsNavHistoryContainerResultNode::GetSortingComparator(PRUint32 aSortType)
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
    default:
      NS_NOTREACHED("Bad sorting type");
      return nsnull;
  }
}










void
nsNavHistoryContainerResultNode::RecursiveSort(
    nsICollation* aCollation, SortComparator aComparator)
{
  mChildren.Sort(aComparator, NS_STATIC_CAST(void*, aCollation));
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
    if (mChildren[i]->IsContainer())
      mChildren[i]->GetAsContainer()->RecursiveSort(aCollation, aComparator);
  }
}







PRUint32
nsNavHistoryContainerResultNode::FindInsertionPoint(
    nsNavHistoryResultNode* aNode, SortComparator aComparator)
{
  if (mChildren.Count() == 0)
    return 0;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, 0);
  nsICollation* collation = history->GetCollation();

  
  
  if (aComparator(aNode, mChildren[0], collation) <= 0)
    return 0;
  if (aComparator(aNode, mChildren[mChildren.Count() - 1], collation) >= 0)
    return mChildren.Count();

  PRUint32 beginRange = 0; 
  PRUint32 endRange = mChildren.Count(); 
  while (1) {
    if (beginRange == endRange)
      return endRange;
    PRUint32 center = beginRange + (endRange - beginRange) / 2;
    if (aComparator(aNode, mChildren[center], collation) <= 0)
      endRange = center; 
    else
      beginRange = center + 1; 
  }
}








PRBool
nsNavHistoryContainerResultNode::DoesChildNeedResorting(PRUint32 aIndex,
    SortComparator aComparator)
{
  NS_ASSERTION(aIndex >= 0 && aIndex < PRUint32(mChildren.Count()),
               "Input index out of range");
  if (mChildren.Count() == 1)
    return PR_FALSE;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, 0);
  nsICollation* collation = history->GetCollation();

  if (aIndex > 0) {
    
    if (aComparator(mChildren[aIndex - 1], mChildren[aIndex], collation) > 0)
      return PR_TRUE;
  }
  if (aIndex < PRUint32(mChildren.Count()) - 1) {
    
    if (aComparator(mChildren[aIndex], mChildren[aIndex + 1], collation) > 0)
      return PR_TRUE;
  }
  return PR_FALSE;
}








PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_Bookmark(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return a->mBookmarkIndex - b->mBookmarkIndex;
}











PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_TitleLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRUint32 aType, bType;
  a->GetType(&aType);
  b->GetType(&bType);

  if (aType != bType) {
    return bType - aType;
  }

  if (aType == nsINavHistoryResultNode::RESULT_TYPE_DAY) {
    
    
    
    
    
    
    
    
    return -ComparePRTime(a->mTime, b->mTime);
  }

  nsICollation* collation = NS_STATIC_CAST(nsICollation*, closure);
  PRInt32 value = -1; 
  collation->CompareString(
      nsICollation::kCollationCaseInSensitive,
      NS_ConvertUTF8toUTF16(a->mTitle),
      NS_ConvertUTF8toUTF16(b->mTitle), &value);
  if (value == 0) {
    
    if (a->IsURI()) {
      value = a->mURI.Compare(b->mURI.get());
    }
    if (value == 0) {
      
      return ComparePRTime(a->mTime, b->mTime);
    }
  }
  return value;
}
PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_TitleGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_TitleLess(a, b, closure);
}






PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_DateLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = ComparePRTime(a->mTime, b->mTime);
  if (value == 0) {
    nsICollation* collation = NS_STATIC_CAST(nsICollation*, closure);
    collation->CompareString(
        nsICollation::kCollationCaseInSensitive,
        NS_ConvertUTF8toUTF16(a->mTitle),
        NS_ConvertUTF8toUTF16(b->mTitle), &value);
  }
  return value;
}
PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_DateGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = -ComparePRTime(a->mTime, b->mTime);
  if (value == 0) {
    nsICollation* collation = NS_STATIC_CAST(nsICollation*, closure);
    collation->CompareString(
        nsICollation::kCollationCaseInSensitive,
        NS_ConvertUTF8toUTF16(a->mTitle),
        NS_ConvertUTF8toUTF16(b->mTitle), &value);
    value = -value;
  }
  return value;
}







PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_URILess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRUint32 aType, bType;
  a->GetType(&aType);
  b->GetType(&bType);

  if (aType != bType) {
    
    return bType - aType;
  }

  PRInt32 value;
  if (a->IsURI()) {
    
    value = a->mURI.Compare(b->mURI.get());
  } else {
    
    nsICollation* collation = NS_STATIC_CAST(nsICollation*, closure);
    collation->CompareString(
        nsICollation::kCollationCaseInSensitive,
        NS_ConvertUTF8toUTF16(a->mTitle),
        NS_ConvertUTF8toUTF16(b->mTitle), &value);
  }

  
  if (value == 0) {
    return ComparePRTime(a->mTime, b->mTime);
  }
  return value;
}
PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_URIGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  return -SortComparison_URILess(a, b, closure);
}






PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_VisitCountLess(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = CompareIntegers(a->mAccessCount, b->mAccessCount);
  if (value == 0)
    return ComparePRTime(a->mTime, b->mTime);
  return value;
}
PRInt32 PR_CALLBACK nsNavHistoryContainerResultNode::SortComparison_VisitCountGreater(
    nsNavHistoryResultNode* a, nsNavHistoryResultNode* b, void* closure)
{
  PRInt32 value = -CompareIntegers(a->mAccessCount, b->mAccessCount);
  if (value == 0)
    return -ComparePRTime(a->mTime, b->mTime);
  return value;
}







nsNavHistoryResultNode*
nsNavHistoryContainerResultNode::FindChildURI(const nsACString& aSpec,
    PRUint32* aNodeIndex)
{
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
    if (mChildren[i]->IsURI()) {
      if (aSpec.Equals(mChildren[i]->mURI)) {
        *aNodeIndex = i;
        return mChildren[i];
      }
    }
  }
  return nsnull;
}







nsNavHistoryFolderResultNode*
nsNavHistoryContainerResultNode::FindChildFolder(PRInt64 aFolderId,
    PRUint32* aNodeIndex)
{
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
    if (mChildren[i]->IsFolder()) {
      nsNavHistoryFolderResultNode* folder = mChildren[i]->GetAsFolder();
      if (folder->mFolderId == aFolderId) {
        *aNodeIndex = i;
        return folder;
      }
    }
  }
  return nsnull;
}







nsNavHistoryContainerResultNode*
nsNavHistoryContainerResultNode::FindChildContainerByName(
    const nsACString& aTitle, PRUint32* aNodeIndex)
{
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
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
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  aNode->mViewIndex = -1;
  aNode->mParent = this;
  aNode->mIndentLevel = mIndentLevel + 1;
  if (! aIsTemporary && aNode->IsContainer()) {
    
    nsNavHistoryContainerResultNode* container = aNode->GetAsContainer();
    container->mResult = mResult;
    container->FillStats();
  }

  if (! mChildren.InsertObjectAt(aNode, aIndex))
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (! aIsTemporary) {
    mAccessCount += aNode->mAccessCount;
    if (mTime < aNode->mTime)
      mTime = aNode->mTime;
    if (result->GetView())
      result->GetView()->ItemChanged(
          NS_STATIC_CAST(nsINavHistoryContainerResultNode*, this));
    ReverseUpdateStats(aNode->mAccessCount);
  }

  
  
  
  if (mExpanded && result->GetView())
    result->GetView()->ItemInserted(this, aNode, aIndex);
  return NS_OK;
}







nsresult
nsNavHistoryContainerResultNode::InsertSortedChild(
    nsNavHistoryResultNode* aNode, PRBool aIsTemporary)
{

  if (mChildren.Count() == 0)
    return InsertChildAt(aNode, 0, aIsTemporary);

  SortComparator comparator = GetSortingComparator(GetSortType());
  if (comparator) {
    
    
    
    
    
    
    if (! aIsTemporary && aNode->IsContainer())
      aNode->GetAsContainer()->FillStats();

    return InsertChildAt(aNode, FindInsertionPoint(aNode, comparator),
                         aIsTemporary);
  }
  return InsertChildAt(aNode, mChildren.Count(), aIsTemporary);
}

















void
nsNavHistoryContainerResultNode::MergeResults(
    nsCOMArray<nsNavHistoryResultNode>* aAddition)
{
  
  
  
  for (PRUint32 i = 0; i < PRUint32(aAddition->Count()); i ++) {
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
        if (oldNode)
          ReplaceChildURIAt(oldIndex, curAddition);
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
    ReverseUpdateStats(accessCountChange);
  }

  
  
  nsCOMPtr<nsNavHistoryResultNode> oldItem = mChildren[aIndex];

  
  if (! mChildren.ReplaceObjectAt(aNode, aIndex))
    return NS_ERROR_FAILURE;

  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    result->GetView()->ItemReplaced(this, oldItem, aNode, aIndex);

  mChildren[aIndex]->OnRemoving();
  return NS_OK;
}













nsresult
nsNavHistoryContainerResultNode::RemoveChildAt(PRInt32 aIndex,
                                               PRBool aIsTemporary)
{
  NS_ASSERTION(aIndex >= 0 && aIndex < mChildren.Count(), "Invalid index");

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsNavHistoryResultNode> oldNode = mChildren[aIndex];

  
  PRUint32 oldAccessCount = 0;
  if (! aIsTemporary) {
    oldAccessCount = mAccessCount;
    mAccessCount -= mChildren[aIndex]->mAccessCount;
    NS_ASSERTION(mAccessCount >= 0, "Invalid access count while updating!");
  }

  
  mChildren.RemoveObjectAt(aIndex);
  if (result->GetView())
    result->GetView()->ItemRemoved(this, oldNode, aIndex);

  if (! aIsTemporary) {
    ReverseUpdateStats(mAccessCount - oldAccessCount);
    oldNode->OnRemoving();
  }
  return NS_OK;
}








PRBool
nsNavHistoryContainerResultNode::CanRemoteContainersChange()
{
  return (mContainerType != nsNavHistoryResultNode::RESULT_TYPE_FOLDER &&
          mContainerType != nsNavHistoryResultNode::RESULT_TYPE_QUERY);
}













void
nsNavHistoryContainerResultNode::RecursiveFindURIs(PRBool aOnlyOne,
    nsNavHistoryContainerResultNode* aContainer, const nsCString& aSpec,
    nsCOMArray<nsNavHistoryResultNode>* aMatches)
{
  for (PRInt32 child = 0; child < aContainer->mChildren.Count(); child ++) {
    PRUint32 type;
    aContainer->mChildren[child]->GetType(&type);
    if (nsNavHistoryResultNode::IsTypeURI(type)) {
      
      nsNavHistoryResultNode* uriNode = aContainer->mChildren[child];
      if (uriNode->mURI.Equals(aSpec)) {
        
        aMatches->AppendObject(uriNode);
        if (aOnlyOne)
          return;
      }
    } else if (nsNavHistoryResultNode::IsTypeQuerySubcontainer(type)) {
      
      RecursiveFindURIs(aOnlyOne, aContainer->mChildren[child]->GetAsContainer(),
                        aSpec, aMatches);
      if (aOnlyOne && aMatches->Count() > 0)
        return;
    }
  }
}










void
nsNavHistoryContainerResultNode::UpdateURIs(PRBool aRecursive, PRBool aOnlyOne,
    PRBool aUpdateSort, const nsCString& aSpec,
    void (*aCallback)(nsNavHistoryResultNode*,void*), void* aClosure)
{
  nsNavHistoryResult* result = GetResult();
  if (! result) {
    NS_NOTREACHED("Must have a result for this query");
    return;
  }

  
  
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
    
    
    
    return;
  }
  if (matches.Count() == 0)
    return;

  SortComparator comparator = nsnull;
  if (aUpdateSort)
    comparator = GetSortingComparator(GetSortType());

  
  
  
  
  for (PRInt32 i = 0; i < matches.Count(); i ++)
  {
    nsNavHistoryResultNode* node = matches[i];
    nsNavHistoryContainerResultNode* parent = node->mParent;
    if (! parent) {
      NS_NOTREACHED("All URI nodes being updated must have parents");
      continue;
    }
    PRUint32 oldAccessCount = node->mAccessCount;
    PRTime oldTime = node->mTime;
    aCallback(node, aClosure);

    if (oldAccessCount != node->mAccessCount || oldTime != node->mTime) {
      
      parent->mAccessCount += node->mAccessCount - oldAccessCount;
      if (node->mTime > parent->mTime)
        parent->mTime = node->mTime;
      if (result->GetView())
        result->GetView()->ItemChanged(
            NS_STATIC_CAST(nsINavHistoryContainerResultNode*, parent));
      parent->ReverseUpdateStats(node->mAccessCount - oldAccessCount);
    }

    if (aUpdateSort) {
      PRInt32 childIndex = parent->FindChild(node);
      if (childIndex >= 0 && parent->DoesChildNeedResorting(childIndex, comparator)) {
        
        parent->RemoveChildAt(childIndex, PR_TRUE);
        parent->InsertChildAt(node, parent->FindInsertionPoint(node, comparator),
                              PR_TRUE);
      } else if (result->GetView()) {
        result->GetView()->ItemChanged(node);
      }
    } else if (result->GetView()) {
      result->GetView()->ItemChanged(node);
    }
  }
}









static void PR_CALLBACK setTitleCallback(
    nsNavHistoryResultNode* aNode, void* aClosure)
{
  const nsACString* newTitle = NS_REINTERPRET_CAST(nsACString*, aClosure);
  aNode->mTitle = *newTitle;
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
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 sortType = GetSortType();
  PRBool updateSorting =
    (sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING ||
     sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING);

  UpdateURIs(aRecursive, aOnlyOne, updateSorting, uriString,
             setTitleCallback,
             NS_CONST_CAST(void*, NS_REINTERPRET_CAST(const void*, &aNewTitle)));
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
  if (! mExpanded)
    return NS_ERROR_NOT_AVAILABLE;
  *aChildCount = mChildren.Count();
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChild(PRUint32 aIndex,
                                          nsINavHistoryResultNode** _retval)
{
  if (! mExpanded)
    return NS_ERROR_NOT_AVAILABLE;
  if (aIndex >= PRUint32(mChildren.Count()))
    return NS_ERROR_INVALID_ARG;
  NS_ADDREF(*_retval = mChildren[aIndex]);
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetChildrenReadOnly(PRBool *aChildrenReadOnly)
{
  *aChildrenReadOnly = mChildrenReadOnly;
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryContainerResultNode::GetRemoteContainerType(
    nsACString& aRemoteContainerType)
{
  aRemoteContainerType = mRemoteContainerType;
  return NS_OK;
}




#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendURINode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, nsINavHistoryResultNode** _retval)
{
  *_retval = nsnull;
  if (mRemoteContainerType.IsEmpty() || ! CanRemoteContainersChange())
    return NS_ERROR_INVALID_ARG; 

  nsRefPtr<nsNavHistoryResultNode> result =
      new nsNavHistoryResultNode(aURI, aTitle, aAccessCount, aTime, aIconURI);
  NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

  
  if (! mChildren.AppendObject(result))
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval = result);
  return NS_OK;
}
#endif




#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendVisitNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, PRInt64 aSession,
    nsINavHistoryVisitResultNode** _retval)
{
  *_retval = nsnull;
  if (mRemoteContainerType.IsEmpty() || ! CanRemoteContainersChange())
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
#endif




#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendFullVisitNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, PRInt64 aSession,
    PRInt64 aVisitId, PRInt64 aReferringVisitId, PRInt32 aTransitionType,
    nsINavHistoryFullVisitResultNode** _retval)
{
  *_retval = nsnull;
  if (mRemoteContainerType.IsEmpty() || ! CanRemoteContainersChange())
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
#endif




#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendContainerNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    PRUint32 aContainerType, const nsACString& aRemoteContainerType,
    nsINavHistoryContainerResultNode** _retval)
{
  *_retval = nsnull;
  if (mRemoteContainerType.IsEmpty() || ! CanRemoteContainersChange())
    return NS_ERROR_INVALID_ARG; 
  if (! IsTypeContainer(aContainerType) || IsTypeFolder(aContainerType) ||
      IsTypeQuery(aContainerType))
    return NS_ERROR_INVALID_ARG; 
  if (aContainerType == nsNavHistoryResultNode::RESULT_TYPE_REMOTE_CONTAINER &&
      aRemoteContainerType.IsEmpty())
    return NS_ERROR_INVALID_ARG; 
  if (aContainerType != nsNavHistoryResultNode::RESULT_TYPE_REMOTE_CONTAINER &&
      ! aRemoteContainerType.IsEmpty())
    return NS_ERROR_INVALID_ARG; 

  nsRefPtr<nsNavHistoryContainerResultNode> result =
      new nsNavHistoryContainerResultNode(EmptyCString(), aTitle, aIconURI,
                                          aContainerType, PR_TRUE,
                                          aRemoteContainerType);
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
  if (mRemoteContainerType.IsEmpty() || ! CanRemoteContainersChange())
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




#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::AppendFolderNode(
    PRInt64 aFolderId, nsINavHistoryFolderResultNode** _retval)
{
  *_retval = nsnull;
  if (mRemoteContainerType.IsEmpty() || ! CanRemoteContainersChange())
    return NS_ERROR_INVALID_ARG; 

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  
  nsRefPtr<nsNavHistoryResultNode> result;
  nsresult rv = bookmarks->ResultNodeForFolder(aFolderId,
                                               GetGeneratingOptions(),
                                               getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (! mChildren.AppendObject(result))
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval = result->GetAsFolder());
  return NS_OK;
}
#endif






#if 0 
NS_IMETHODIMP
nsNavHistoryContainerResultNode::ClearContents()
{
  if (mRemoteContainerType.IsEmpty() || ! CanRemoteContainersChange())
    return NS_ERROR_INVALID_ARG; 

  
  
  
  
  mChildren.Clear();

  PRUint32 oldAccessCount = mAccessCount;
  mAccessCount = 0;
  mTime = 0;
  ReverseUpdateStats(-PRInt32(oldAccessCount));
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
                                  PR_TRUE, EmptyCString()),
  mHasSearchTerms(PR_FALSE),
  mContentsValid(PR_FALSE),
  mBatchInProgress(PR_FALSE)
{
  
  if (mFaviconURI.IsEmpty())
    mFaviconURI.AppendLiteral(ICONURI_QUERY);
}

nsNavHistoryQueryResultNode::nsNavHistoryQueryResultNode(
    const nsACString& aTitle, const nsACString& aIconURI,
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, aIconURI,
                                  nsNavHistoryResultNode::RESULT_TYPE_QUERY,
                                  PR_TRUE, EmptyCString()),
  mQueries(aQueries),
  mOptions(aOptions),
  mContentsValid(PR_FALSE),
  mBatchInProgress(PR_FALSE)
{
  NS_ASSERTION(aQueries.Count() > 0, "Must have at least one query");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ASSERTION(history, "History service missing");
  mLiveUpdate = history->GetUpdateRequirements(mQueries, mOptions,
                                               &mHasSearchTerms);

  
  if (mFaviconURI.IsEmpty()) {
    mFaviconURI.AppendLiteral(ICONURI_QUERY);

    
    nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
    if (! faviconService)
      return;
    nsresult rv = VerifyQueriesSerialized();
    if (NS_FAILED(rv)) return;

    nsCOMPtr<nsIURI> queryURI;
    rv = NS_NewURI(getter_AddRefs(queryURI), mURI);
    if (NS_FAILED(rv)) return;
    nsCOMPtr<nsIURI> favicon;
    rv = faviconService->GetFaviconForPage(queryURI, getter_AddRefs(favicon));
    if (NS_FAILED(rv)) return;
    favicon->GetSpec(mFaviconURI);
  }
}








PRBool
nsNavHistoryQueryResultNode::CanExpand()
{
  nsNavHistoryQueryOptions* options = GetGeneratingOptions();
  if (options && options->ExpandQueries())
    return PR_TRUE;
  if (mResult && mResult->mRootNode == this)
    return PR_TRUE;
  return PR_FALSE;
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
  NS_ASSERTION(! mExpanded, "Container must be expanded to close it");
  mExpanded = PR_TRUE;
  if (! CanExpand())
    return NS_OK;
  if (! mContentsValid) {
    nsresult rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    result->GetView()->ContainerOpened(
        NS_STATIC_CAST(nsNavHistoryContainerResultNode*, this));
  return NS_OK;
}









NS_IMETHODIMP
nsNavHistoryQueryResultNode::GetHasChildren(PRBool* aHasChildren)
{
  if (! CanExpand()) {
    *aHasChildren = PR_FALSE;
    return NS_OK;
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
nsNavHistoryQueryResultNode::GetQueries(PRUint32* queryCount,
                                        nsINavHistoryQuery*** queries)
{
  nsresult rv = VerifyQueriesParsed();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mQueries.Count() > 0, "Must have >= 1 query");

  *queries = NS_STATIC_CAST(nsINavHistoryQuery**,
               nsMemory::Alloc(mQueries.Count() * sizeof(nsINavHistoryQuery*)));
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
  nsresult rv = VerifyQueriesParsed();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mOptions, "Options invalid");

  *aQueryOptions = mOptions;
  NS_ADDREF(*aQueryOptions);
  return NS_OK;
}




nsresult
nsNavHistoryQueryResultNode::VerifyQueriesParsed()
{
  if (mQueries.Count() > 0) {
    NS_ASSERTION(mOptions, "If a result has queries, it also needs options");
    return NS_OK;
  }
  NS_ASSERTION(! mURI.IsEmpty(),
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
  if (! mURI.IsEmpty()) {
    return NS_OK;
  }
  NS_ASSERTION(mQueries.Count() > 0 && mOptions,
               "Query nodes must have either a URI or query/options");

  nsTArray<nsINavHistoryQuery*> flatQueries;
  flatQueries.SetCapacity(mQueries.Count());
  for (PRInt32 i = 0; i < mQueries.Count(); i ++)
    flatQueries.AppendElement(NS_STATIC_CAST(nsINavHistoryQuery*,
                                             mQueries.ObjectAt(i)));

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = history->QueriesToQueryString(flatQueries.Elements(),
                                              flatQueries.Length(),
                                              mOptions, mURI);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(! mURI.IsEmpty(), NS_ERROR_FAILURE);
  return NS_OK;
}




nsresult
nsNavHistoryQueryResultNode::FillChildren()
{
  NS_ASSERTION(! mContentsValid,
               "Don't call FillChildren when contents are valid");
  NS_ASSERTION(mChildren.Count() == 0,
               "We are trying to fill children when there already are some");

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = VerifyQueriesParsed();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = history->GetQueryResults(mQueries, mOptions, &mChildren);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  FillStats();

  
  
  SortComparator comparator = GetSortingComparator(GetSortType());
  if (comparator)
    RecursiveSort(history->GetCollation(), comparator);

  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  result->AddEverythingObserver(this);

  mContentsValid = PR_TRUE;
  return NS_OK;
}













void
nsNavHistoryQueryResultNode::ClearChildren(PRBool aUnregister)
{
  for (PRInt32 i = 0; i < mChildren.Count(); i ++)
    mChildren[i]->OnRemoving();
  mChildren.Clear();

  if (aUnregister && mContentsValid) {
    nsNavHistoryResult* result = GetResult();
    if (result)
      result->RemoveEverythingObserver(this);
  }
  mContentsValid = PR_FALSE;
}







nsresult
nsNavHistoryQueryResultNode::Refresh()
{
  
  
  if (mBatchInProgress)
    return NS_OK;

  if (! mExpanded) {
    
    ClearChildren(PR_TRUE);
    return NS_OK; 
  }

  
  
  ClearChildren(PR_FALSE);
  FillChildren();

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    return result->GetView()->InvalidateContainer(
        NS_STATIC_CAST(nsNavHistoryContainerResultNode*, this));
  return NS_OK;
}


































PRUint32
nsNavHistoryQueryResultNode::GetSortType()
{
  if (mParent) {
    
    return mOptions->SortingMode();
  } else if (mResult) {
    return mResult->mSortingMode;
  }

  NS_NOTREACHED("We should always have a result");
  return nsINavHistoryQueryOptions::SORT_BY_NONE;
}



NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnBeginUpdateBatch()
{
  mBatchInProgress = PR_TRUE;
  return NS_OK;
}








NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnEndUpdateBatch()
{
  NS_ASSERTION(mBatchInProgress, "EndUpdateBatch without a begin");
  
  mBatchInProgress = PR_FALSE;
  return Refresh();
}









NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnVisit(nsIURI* aURI, PRInt64 aVisitId,
                                     PRTime aTime, PRInt64 aSessionId,
                                     PRInt64 aReferringId,
                                     PRUint32 aTransitionType)
{
  
  if (mBatchInProgress)
    return NS_OK;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  nsCOMPtr<nsNavHistoryResultNode> addition;
  switch(mLiveUpdate) {
    case QUERYUPDATE_TIME: {
      
      
      NS_ASSERTION(mQueries.Count() == 1, "Time updated queries can have only one object");
      nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(mQueries[0], &rv);
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
      
      rv = history->VisitIdToResultNode(aVisitId, mOptions,
                                        getter_AddRefs(addition));
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case QUERYUPDATE_SIMPLE: {
      
      
      rv = history->VisitIdToResultNode(aVisitId, mOptions,
                                        getter_AddRefs(addition));
      NS_ENSURE_SUCCESS(rv, rv);
      if (! history->EvaluateQueryForNode(mQueries, mOptions, addition))
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

  
  
  
  
  
  
  
  
  
  
  

  PRUint32 groupCount;
  const PRUint32* groupings = mOptions->GroupingMode(&groupCount);
  nsCOMArray<nsNavHistoryResultNode> grouped;
  if (groupCount > 0) {
    
    
    nsCOMArray<nsNavHistoryResultNode> itemSource;
    if (! itemSource.AppendObject(addition))
      return NS_ERROR_OUT_OF_MEMORY;
    history->RecursiveGroup(itemSource, groupings, groupCount, &grouped);
  } else {
    
    if (! grouped.AppendObject(addition))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  MergeResults(&grouped);
  return NS_OK;
}












NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnTitleChanged(nsIURI* aURI,
                                            const nsAString& aPageTitle,
                                            const nsAString& aUserTitle,
                                            PRBool aIsUserTitleChanged)
{
  if (mBatchInProgress)
    return NS_OK; 
  if (! mExpanded) {
    
    
    
    
    ClearChildren(PR_TRUE);
    return NS_OK; 
  }

  
  
  
  
  if (mHasSearchTerms) {
    return Refresh();
  }

  
  nsCAutoString newTitle;
  if (mOptions->ForceOriginalTitle() || aUserTitle.IsVoid()) {
    newTitle = NS_ConvertUTF16toUTF8(aPageTitle);
  } else {
    newTitle = NS_ConvertUTF16toUTF8(aUserTitle);
  }

  PRBool onlyOneEntry = (mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_URI);
  return ChangeTitles(aURI, newTitle, PR_TRUE, onlyOneEntry);
}







NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnDeleteURI(nsIURI *aURI)
{
  PRBool onlyOneEntry = (mOptions->ResultType() ==
                         nsINavHistoryQueryOptions::RESULTS_AS_URI);
  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsNavHistoryResultNode> matches;
  RecursiveFindURIs(onlyOneEntry, this, spec, &matches);
  if (matches.Count() == 0)
    return NS_OK; 

  for (PRInt32 i = 0; i < matches.Count(); i ++) {
    nsNavHistoryResultNode* node = matches[i];
    nsNavHistoryContainerResultNode* parent = node->mParent;
    NS_ASSERTION(parent, "URI nodes should always have parents");

    PRInt32 childIndex = parent->FindChild(node);
    NS_ASSERTION(childIndex >= 0, "Child not found in parent");
    parent->RemoveChildAt(childIndex);

    if (parent->mChildren.Count() == 0 && parent->IsQuerySubcontainer()) {
      
      
      
      matches.AppendObject(parent);
    }
  }
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnClearHistory()
{
  Refresh();
  return NS_OK;
}





static void PR_CALLBACK setFaviconCallback(
   nsNavHistoryResultNode* aNode, void* aClosure)
{
  const nsCString* newFavicon = NS_STATIC_CAST(nsCString*, aClosure);
  aNode->mFaviconURI = *newFavicon;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnPageChanged(nsIURI *aURI, PRUint32 aWhat,
                         const nsAString &aValue)
{
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (aWhat) {
    case nsINavHistoryObserver::ATTRIBUTE_FAVICON: {
      nsCString newFavicon = NS_ConvertUTF16toUTF8(aValue);
      PRBool onlyOneEntry = (mOptions->ResultType() ==
                             nsINavHistoryQueryOptions::RESULTS_AS_URI);
      UpdateURIs(PR_TRUE, onlyOneEntry, PR_FALSE, spec, setFaviconCallback,
                 &newFavicon);
      break;
    }
    default:
      NS_WARNING("Unknown page changed notification");
  }
  return NS_OK;
}







NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnPageExpired(nsIURI* aURI, PRTime aVisitTime,
                                           PRBool aWholeEntry)
{
  return NS_OK;
}








NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemAdded(PRInt64 aBookmarkId, nsIURI* aBookmark, PRInt64 aFolder,
                                          PRInt32 aIndex)
{
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    return Refresh();
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemRemoved(PRInt64 aBookmarkId, nsIURI* aBookmark, PRInt64 aFolder,
                                            PRInt32 aIndex)
{
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    return Refresh();
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemChanged(PRInt64 aBookmarkId, nsIURI* aBookmark,
                                            const nsACString& aProperty,
                                            const nsAString& aValue)
{
  NS_NOTREACHED("Everything observers should not get OnItemChanged, but should get the corresponding history notifications instead");
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnItemVisited(PRInt64 aBookmarkId, nsIURI* aBookmark,
                                           PRInt64 aVisitId, PRTime aTime)
{
  NS_NOTREACHED("Everything observers should not get OnItemVisited, but should get OnVisit instead");
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnFolderAdded(PRInt64 aFolder, PRInt64 aParent,
                                            PRInt32 aIndex)
{
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    return Refresh();
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnFolderRemoved(PRInt64 aFolder, PRInt64 aParent,
                                              PRInt32 aIndex)
{
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    return Refresh();
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnFolderMoved(PRInt64 aFolder, PRInt64 aOldParent,
                                            PRInt32 aOldIndex, PRInt64 aNewParent,
                                            PRInt32 aNewIndex)
{
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    return Refresh();
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnFolderChanged(PRInt64 aFolder,
                                              const nsACString& property)
{
  if (mLiveUpdate == QUERYUPDATE_COMPLEX_WITH_BOOKMARKS)
    return Refresh();
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnSeparatorAdded(PRInt64 aParent, PRInt32 aIndex)
{
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryResultNode::OnSeparatorRemoved(PRInt64 aParent,
                                                PRInt32 aIndex)
{
  return NS_OK;
}



































NS_IMPL_ISUPPORTS_INHERITED2(nsNavHistoryFolderResultNode,
                             nsNavHistoryContainerResultNode,
                             nsINavHistoryQueryResultNode,
                             nsINavHistoryFolderResultNode)

nsNavHistoryFolderResultNode::nsNavHistoryFolderResultNode(
    const nsACString& aTitle, nsNavHistoryQueryOptions* aOptions,
    PRInt64 aFolderId, const nsACString& aRemoteContainerType) :
  nsNavHistoryContainerResultNode(EmptyCString(), aTitle, EmptyCString(),
                                  nsNavHistoryResultNode::RESULT_TYPE_FOLDER,
                                  PR_FALSE, aRemoteContainerType),
  mContentsValid(PR_FALSE),
  mOptions(aOptions),
  mFolderId(aFolderId)
{
  
  
  
  
  
  
  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  if (! bookmarks)
    return;

  
  
  
  nsCOMPtr<nsIURI> folderURI;
  nsresult rv = bookmarks->GetFolderURI(aFolderId, getter_AddRefs(folderURI));
  if (NS_FAILED(rv))
    return;

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  if (! faviconService)
    return;
  nsCOMPtr<nsIURI> favicon;
  rv = faviconService->GetFaviconForPage(folderURI, getter_AddRefs(favicon));
  if (NS_FAILED(rv))
    return; 

  
  favicon->GetSpec(mFaviconURI);
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
  NS_ASSERTION(! mExpanded, "Container must be expanded to close it");
  nsresult rv;

  













  if (! mContentsValid) {
    rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  mExpanded = PR_TRUE;
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    result->GetView()->ContainerOpened(
        NS_STATIC_CAST(nsNavHistoryContainerResultNode*, this));
  return NS_OK;
}











NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetHasChildren(PRBool* aHasChildren)
{
  if (! mContentsValid) {
    nsresult rv = FillChildren();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  *aHasChildren = (mChildren.Count() > 0);
  return NS_OK;
}













NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetChildrenReadOnly(PRBool *aChildrenReadOnly)
{
  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_UNEXPECTED);
  return bookmarks->GetFolderReadonly(mFolderId, aChildrenReadOnly);
}







NS_IMETHODIMP
nsNavHistoryFolderResultNode::GetUri(nsACString& aURI)
{
  if (! mURI.IsEmpty()) {
    aURI = mURI;
    return NS_OK;
  }

  PRUint32 queryCount;
  nsINavHistoryQuery** queries;
  nsresult rv = GetQueries(&queryCount, &queries);
  NS_ENSURE_SUCCESS(rv, rv);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsCAutoString queryString;
  rv = history->QueriesToQueryString(queries, queryCount, mOptions, aURI);
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

  
  rv = query->SetFolders(&mFolderId, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  
  *queries = NS_STATIC_CAST(nsINavHistoryQuery**,
                            nsMemory::Alloc(sizeof(nsINavHistoryQuery*)));
  if (! *queries)
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
  NS_ASSERTION(! mContentsValid,
               "Don't call FillChildren when contents are valid");
  NS_ASSERTION(mChildren.Count() == 0,
               "We are trying to fill children when there already are some");

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = bookmarks->QueryFolderChildren(mFolderId, mOptions, &mChildren);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  

  
  
  FillStats();

  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  result->AddBookmarkObserver(this, mFolderId);

  mContentsValid = PR_TRUE;
  return NS_OK;
}






void
nsNavHistoryFolderResultNode::ClearChildren(PRBool unregister)
{
  for (PRInt32 i = 0; i < mChildren.Count(); i ++)
    mChildren[i]->OnRemoving();
  mChildren.Clear();

  if (unregister && mContentsValid) {
    nsNavHistoryResult* result = GetResult();
    if (result)
      result->RemoveBookmarkObserver(this, mFolderId);
  }
  mContentsValid = PR_FALSE;
}







nsresult
nsNavHistoryFolderResultNode::Refresh()
{
  if (! mExpanded) {
    
    ClearChildren(PR_TRUE);
    return NS_OK; 
  }

  
  
  
  ClearChildren(PR_TRUE);
  FillChildren();

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    return result->GetView()->InvalidateContainer(
        NS_STATIC_CAST(nsNavHistoryContainerResultNode*, this));
  return NS_OK;
}








PRBool
nsNavHistoryFolderResultNode::StartIncrementalUpdate()
{
  
  
  if (! mOptions->ExcludeItems() && ! mOptions->ExcludeQueries() && 
      ! mOptions->ExcludeReadOnlyFolders()) {

    
    if (mExpanded || AreChildrenVisible())
      return PR_TRUE;

    nsNavHistoryResult* result = GetResult();
    NS_ENSURE_TRUE(result, PR_FALSE);

    
    
    if (mParent && result->GetView())
      return PR_TRUE;
  }

  
  Refresh();
  return PR_FALSE;
}








void
nsNavHistoryFolderResultNode::ReindexRange(PRInt32 aStartIndex,
                                           PRInt32 aEndIndex,
                                           PRInt32 aDelta)
{
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
    nsNavHistoryResultNode* node = mChildren[i];
    if (node->mBookmarkIndex >= aStartIndex &&
        node->mBookmarkIndex <= aEndIndex)
      node->mBookmarkIndex += aDelta;
  }
}







nsNavHistoryResultNode*
nsNavHistoryFolderResultNode::FindChildURIById(PRInt64 aBookmarkId,
    PRUint32* aNodeIndex)
{
  for (PRInt32 i = 0; i < mChildren.Count(); i ++) {
    if (mChildren[i]->mBookmarkId == aBookmarkId) {
      *aNodeIndex = i;
      return mChildren[i];
    }
  }
  return nsnull;
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
nsNavHistoryFolderResultNode::OnItemAdded(PRInt64 aBookmarkId, nsIURI* aBookmark,
                                          PRInt64 aFolder, PRInt32 aIndex)
{
  NS_ASSERTION(aFolder == mFolderId, "Got wrong bookmark update");
  if (mOptions->ExcludeItems()) {
    
    
    ReindexRange(aIndex, PR_INT32_MAX, 1);
    return NS_OK; 
  }

  
  
  if (aIndex < 0) {
    NS_NOTREACHED("Invalid index for item adding: <0");
    aIndex = 0;
  } else if (aIndex > mChildren.Count()) {
    NS_NOTREACHED("Invalid index for item adding: greater than count");
    aIndex = mChildren.Count();
  }
  if (! StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex, PR_INT32_MAX, 1);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  nsNavHistoryResultNode* node;
  nsresult rv = history->UriToResultNode(aBookmark, mOptions, &node);
  NS_ENSURE_SUCCESS(rv, rv);
  node->mBookmarkIndex = aIndex;
  node->mBookmarkId = aBookmarkId;

  if (GetSortType() == nsINavHistoryQueryOptions::SORT_BY_NONE) {
    
    return InsertChildAt(node, aIndex);
  }
  
  return InsertSortedChild(node, PR_FALSE);
}




NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemRemoved(PRInt64 aBookmarkId, nsIURI* aBookmark,
                                            PRInt64 aFolder, PRInt32 aIndex)
{
  NS_ASSERTION(aFolder == mFolderId, "Got wrong bookmark update");
  if (mOptions->ExcludeItems()) {
    
    
    ReindexRange(aIndex, PR_INT32_MAX, -1);
    return NS_OK;
  }
  if (! StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex + 1, PR_INT32_MAX, -1);

  
  
  
  PRUint32 nodeIndex;
  nsNavHistoryResultNode* node = FindChildURIById(aBookmarkId, &nodeIndex);
  if (! node)
    return NS_ERROR_FAILURE; 

  return RemoveChildAt(nodeIndex);
}




NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemChanged(PRInt64 aBookmarkId, nsIURI* aBookmark,
                                            const nsACString& aProperty,
                                            const nsAString& aValue)
{
  if (mOptions->ExcludeItems())
    return NS_OK; 
  if (!StartIncrementalUpdate())
    return NS_OK;

  PRUint32 nodeIndex;
  nsNavHistoryResultNode* node = FindChildURIById(aBookmarkId, &nodeIndex);
  if (!node)
    return NS_ERROR_FAILURE;

  if (aProperty.EqualsLiteral("title")) {
    node->mTitle = NS_ConvertUTF16toUTF8(aValue);
    return NS_OK;
  }

  nsCAutoString spec;
  nsresult rv = aBookmark->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aProperty.EqualsLiteral("uri")) {
    node->mURI = spec;
    return NS_OK;
  }

  if (aProperty.EqualsLiteral("favicon")) {
    node->mFaviconURI = NS_ConvertUTF16toUTF8(aValue);
  } else if (aProperty.EqualsLiteral("cleartime")) {
    node->mTime = 0;
  } else {
    NS_NOTREACHED("Unknown bookmark property changing.");
  }

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    result->GetView()->ItemChanged(node);
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnItemVisited(PRInt64 aBookmarkId, nsIURI* aBookmark,
                                            PRInt64 aVisitId, PRTime aTime)
{
  if (mOptions->ExcludeItems())
    return NS_OK; 
  if (! StartIncrementalUpdate())
    return NS_OK;

  PRUint32 nodeIndex;
  nsNavHistoryResultNode* node = FindChildURIById(aBookmarkId, &nodeIndex);
  if (! node)
    return NS_ERROR_FAILURE;

  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  
  node->mTime = aTime;
  node->mAccessCount ++;

  
  PRInt32 oldAccessCount = mAccessCount;
  mAccessCount ++;
  if (aTime > mTime)
    mTime = aTime;
  ReverseUpdateStats(mAccessCount - oldAccessCount);

  
  PRUint32 sortType = GetSortType();
  if (sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING ||
      sortType == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING) {
    PRInt32 childIndex = FindChild(node);
    NS_ASSERTION(childIndex >= 0, "Could not find child we just got a reference to");
    if (childIndex >= 0) {
      SortComparator comparator = GetSortingComparator(GetSortType()); 
      nsCOMPtr<nsINavHistoryResultNode> nodeLock(node);
      RemoveChildAt(childIndex, PR_TRUE);
      InsertChildAt(node, FindInsertionPoint(node, comparator), PR_TRUE);
    }
  } else if (result->GetView()) {
    
    result->GetView()->ItemChanged(node);
  }
  return NS_OK;
}






NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnFolderAdded(PRInt64 aFolder, PRInt64 aParent,
                                            PRInt32 aIndex)
{
  NS_ASSERTION(aParent == mFolderId, "Got wrong bookmark update");
  if (! StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex, PR_INT32_MAX, 1);

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  nsNavHistoryResultNode* node;
  nsresult rv = bookmarks->ResultNodeForFolder(aFolder, mOptions, &node);
  NS_ENSURE_SUCCESS(rv, rv);
  node->mBookmarkIndex = aIndex;

  if (GetSortType() == nsINavHistoryQueryOptions::SORT_BY_NONE) {
    
    return InsertChildAt(node, aIndex);
  }
  
  return InsertSortedChild(node, PR_FALSE);
}




NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnFolderRemoved(PRInt64 aFolder, PRInt64 aParent,
                                              PRInt32 aIndex)
{
  
  
  
  if (mFolderId == aFolder)
    return NS_OK;

  NS_ASSERTION(aParent == mFolderId, "Got wrong bookmark update");
  if (! StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex + 1, PR_INT32_MAX, -1);

  PRUint32 index;
  nsNavHistoryFolderResultNode* node = FindChildFolder(aFolder, &index);
  if (! node) {
    NS_NOTREACHED("Removing folder we don't have");
    return NS_ERROR_FAILURE;
  }
  return RemoveChildAt(index);
}




NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnFolderMoved(PRInt64 aFolder, PRInt64 aOldParent,
                                            PRInt32 aOldIndex, PRInt64 aNewParent,
                                            PRInt32 aNewIndex)
{
  NS_ASSERTION(aOldParent == mFolderId || aNewParent == mFolderId,
               "Got a bookmark message that doesn't belong to us");
  if (! StartIncrementalUpdate())
    return NS_OK; 

  if (aOldParent == aNewParent) {
    
    

    
    ReindexRange(aOldIndex + 1, PR_INT32_MAX, -1);
    ReindexRange(aNewIndex, PR_INT32_MAX, 1);

    PRUint32 index;
    nsNavHistoryFolderResultNode* node = FindChildFolder(aFolder, &index);
    if (! node) {
      NS_NOTREACHED("Can't find folder that is moving!");
      return NS_ERROR_FAILURE;
    }
    NS_ASSERTION(index >= 0 && index < PRUint32(mChildren.Count()),
                 "Invalid index!");
    node->mBookmarkIndex = aNewIndex;

    
    PRInt32 sortType = GetSortType();
    SortComparator comparator = GetSortingComparator(sortType);
    if (DoesChildNeedResorting(index, comparator)) {
      
      
      nsRefPtr<nsNavHistoryContainerResultNode> lock(node);
      RemoveChildAt(index, PR_TRUE);
      InsertChildAt(node, FindInsertionPoint(node, comparator), PR_TRUE);
      return NS_OK;
    }

  } else {
    
    if (aOldParent == mFolderId)
      OnFolderRemoved(aFolder, aOldParent, aOldIndex);
    if (aNewParent == mFolderId)
      OnFolderAdded(aFolder, aNewParent, aNewIndex);
  }
  return NS_OK;
}








NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnFolderChanged(PRInt64 aFolder,
                                              const nsACString& property)
{
  
  
  
  

  if (property.EqualsLiteral("title")) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

    nsAutoString title;
    bookmarks->GetFolderTitle(mFolderId, title);
    mTitle = NS_ConvertUTF16toUTF8(title);

    PRInt32 sortType = GetSortType();
    if ((sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING ||
         sortType == nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING) &&
        mParent) {
      PRInt32 ourIndex = mParent->FindChild(this);
      SortComparator comparator = GetSortingComparator(sortType);
      if (mParent->DoesChildNeedResorting(ourIndex, comparator)) {
        
        
        mParent->RemoveChildAt(ourIndex, PR_TRUE);
        mParent->InsertChildAt(this, mParent->FindInsertionPoint(this, comparator),
                               PR_TRUE);
        return NS_OK;
      }
    }
  } else {
    NS_NOTREACHED("Unknown folder change event");
    return NS_ERROR_FAILURE;
  }

  
  nsNavHistoryResult* result = GetResult();
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  if (result->GetView())
    result->GetView()->ItemChanged(
        NS_STATIC_CAST(nsNavHistoryResultNode*, this));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnSeparatorAdded(PRInt64 aParent, PRInt32 aIndex)
{
  NS_ASSERTION(aParent == mFolderId, "Got wrong bookmark update");
  if (mOptions->ExcludeItems()) {
    
    
    ReindexRange(aIndex, PR_INT32_MAX, 1);
    return NS_OK;
  }
  if (! StartIncrementalUpdate())
    return NS_OK; 

  
  ReindexRange(aIndex, PR_INT32_MAX, 1);

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  nsNavHistoryResultNode* node = new nsNavHistorySeparatorResultNode();
  NS_ENSURE_TRUE(node, NS_ERROR_OUT_OF_MEMORY);
  node->mBookmarkIndex = aIndex;

  return InsertChildAt(node, aIndex);
}



NS_IMETHODIMP
nsNavHistoryFolderResultNode::OnSeparatorRemoved(PRInt64 aParent,
                                                 PRInt32 aIndex)
{
  NS_ASSERTION(aParent == mFolderId, "Got wrong bookmark update");
  if (mOptions->ExcludeItems()) {
    
    
    ReindexRange(aIndex, PR_INT32_MAX, -1);
    return NS_OK;
  }
  if (! StartIncrementalUpdate())
    return NS_OK; 

  ReindexRange(aIndex, PR_INT32_MAX, -1);

  if (aIndex >= mChildren.Count()) {
    NS_NOTREACHED("Removing separator at invalid index");
    return NS_OK;
  }

  if (!mChildren[aIndex]->IsSeparator()) {
    NS_NOTREACHED("OnSeparatorRemoved called for a non-separator node");
    return NS_OK;
  }

  return RemoveChildAt(aIndex);
}






nsNavHistorySeparatorResultNode::nsNavHistorySeparatorResultNode()
  : nsNavHistoryResultNode(EmptyCString(), EmptyCString(),
                           0, 0, EmptyCString())
{
}



NS_IMPL_ADDREF(nsNavHistoryResult)
NS_IMPL_RELEASE(nsNavHistoryResult)

NS_INTERFACE_MAP_BEGIN(nsNavHistoryResult)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryResult)
  NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsNavHistoryResult)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryResult)
  NS_INTERFACE_MAP_ENTRY(nsINavBookmarkObserver)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

nsNavHistoryResult::nsNavHistoryResult(nsNavHistoryContainerResultNode* aRoot) :
  mRootNode(aRoot),
  mIsHistoryObserver(PR_FALSE),
  mIsBookmarksObserver(PR_FALSE)
{
  mRootNode->mResult = this;
}

PR_STATIC_CALLBACK(PLDHashOperator)
RemoveBookmarkObserversCallback(nsTrimInt64HashKey::KeyType aKey,
                                nsNavHistoryResult::FolderObserverList*& aData,
                                void* userArg)
{
  delete aData;
  return PL_DHASH_REMOVE;
}

nsNavHistoryResult::~nsNavHistoryResult()
{
  
  mBookmarkObservers.Enumerate(&RemoveBookmarkObserversCallback, nsnull);
}






nsresult
nsNavHistoryResult::Init(nsINavHistoryQuery** aQueries,
                         PRUint32 aQueryCount,
                         nsNavHistoryQueryOptions *aOptions)
{
  nsresult rv;
  NS_ASSERTION(aOptions, "Must have valid options");
  NS_ASSERTION(aQueries && aQueryCount > 0, "Must have >1 query in result");

  
  
  
  for (PRUint32 i = 0; i < aQueryCount; i ++) {
    nsCOMPtr<nsINavHistoryQuery> queryClone;
    rv = aQueries[i]->Clone(getter_AddRefs(queryClone));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!mQueries.AppendObject(queryClone))
      return NS_ERROR_OUT_OF_MEMORY;
  }
  rv = aOptions->Clone(getter_AddRefs(mOptions));
  mSortingMode = aOptions->SortingMode();
  NS_ENSURE_SUCCESS(rv, rv);

  mPropertyBags.Init();
  if (! mBookmarkObservers.Init(128))
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
                                     nsNavHistoryResult** result)
{
  *result = new nsNavHistoryResult(aRoot);
  if (! *result)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result); 
  nsresult rv = (*result)->Init(aQueries, aQueryCount, aOptions);
  if (NS_FAILED(rv)) {
    NS_RELEASE(*result);
    *result = nsnull;
    return rv;
  }
  return NS_OK;
}

















nsresult
nsNavHistoryResult::PropertyBagFor(nsISupports* aObject,
                                   nsIWritablePropertyBag** aBag)
{
  *aBag = nsnull;
  if (mPropertyBags.Get(aObject, aBag) && *aBag)
    return NS_OK;

  nsresult rv = NS_NewHashPropertyBag(aBag);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! mPropertyBags.Put(aObject, *aBag)) {
    NS_RELEASE(*aBag);
    *aBag = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}






void
nsNavHistoryResult::AddEverythingObserver(nsNavHistoryQueryResultNode* aNode)
{
  if (! mIsHistoryObserver) {
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ASSERTION(history, "Can't create history service");
      history->AddObserver(this, PR_TRUE);
      mIsHistoryObserver = PR_TRUE;
  }
  if (mEverythingObservers.IndexOf(aNode) != mEverythingObservers.NoIndex) {
    NS_NOTREACHED("Attempting to register an observer twice!");
    return;
  }
  mEverythingObservers.AppendElement(aNode);
}






void
nsNavHistoryResult::AddBookmarkObserver(nsNavHistoryFolderResultNode* aNode,
                                        PRInt64 aFolder)
{
  if (! mIsBookmarksObserver) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    if (! bookmarks) {
      NS_NOTREACHED("Can't create bookmark service");
      return;
    }
    bookmarks->AddObserver(this, PR_TRUE);
    mIsBookmarksObserver = PR_TRUE;
  }
  FolderObserverList* list = BookmarkObserversForId(aFolder, PR_TRUE);
  if (list->IndexOf(aNode) != list->NoIndex) {
    NS_NOTREACHED("Attempting to register an observer twice!");
    return;
  }
  list->AppendElement(aNode);
}




void
nsNavHistoryResult::RemoveEverythingObserver(nsNavHistoryQueryResultNode* aNode)
{
  mEverythingObservers.RemoveElement(aNode);
}




void
nsNavHistoryResult::RemoveBookmarkObserver(nsNavHistoryFolderResultNode* aNode,
                                           PRInt64 aFolder)
{
  FolderObserverList* list = BookmarkObserversForId(aFolder, PR_FALSE);
  if (! list)
    return; 
  list->RemoveElement(aNode);
}




nsNavHistoryResult::FolderObserverList*
nsNavHistoryResult::BookmarkObserversForId(PRInt64 aFolderId, PRBool aCreate)
{
  FolderObserverList* list;
  if (mBookmarkObservers.Get(aFolderId, &list))
    return list;
  if (! aCreate)
    return nsnull;

  
  list = new FolderObserverList;
  mBookmarkObservers.Put(aFolderId, list);
  return list;
}



NS_IMETHODIMP
nsNavHistoryResult::GetSortingMode(PRUint32* aSortingMode)
{
  *aSortingMode = mSortingMode;
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistoryResult::SetSortingMode(PRUint32 aSortingMode)
{
  if (aSortingMode > nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING)
    return NS_ERROR_INVALID_ARG;
  if (! mRootNode)
    return NS_ERROR_FAILURE;

  
  NS_ASSERTION(mOptions, "Options should always be present for a root query");

  mSortingMode = aSortingMode;

  
  nsNavHistoryContainerResultNode::SortComparator comparator =
      nsNavHistoryContainerResultNode::GetSortingComparator(aSortingMode);
  if (comparator) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    mRootNode->RecursiveSort(history->GetCollation(), comparator);
  }

  if (mView) {
    mView->SortingChanged(aSortingMode);
    mView->InvalidateAll();
  }
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::GetViewer(nsINavHistoryResultViewer** aViewer)
{
  *aViewer = mView;
  NS_IF_ADDREF(*aViewer);
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryResult::SetViewer(nsINavHistoryResultViewer* aViewer)
{
  mView = aViewer;
  if (aViewer)
    aViewer->SetResult(this);
  return NS_OK;
}








NS_IMETHODIMP
nsNavHistoryResult::GetRoot(nsINavHistoryQueryResultNode** aRoot)
{
  if (! mRootNode) {
    NS_NOTREACHED("Root is null");
    *aRoot = nsnull;
    return NS_ERROR_FAILURE;
  }
  return mRootNode->QueryInterface(NS_GET_IID(nsINavHistoryQueryResultNode),
                                   NS_REINTERPRET_CAST(void**, aRoot));
}







#define ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(_folderId, _functionCall) \
  { \
    FolderObserverList* _fol = BookmarkObserversForId(_folderId, PR_FALSE); \
    if (_fol) { \
      FolderObserverList _listCopy(*_fol); \
      for (PRUint32 _fol_i = 0; _fol_i < _listCopy.Length(); _fol_i ++) { \
        if (_listCopy[_fol_i]) \
          _listCopy[_fol_i]->_functionCall; \
      } \
    } \
  }
#define ENUMERATE_HISTORY_OBSERVERS(_functionCall) \
  { \
    nsTArray<nsNavHistoryQueryResultNode*> observerCopy(mEverythingObservers); \
    for (PRUint32 _obs_i = 0; _obs_i < observerCopy.Length(); _obs_i ++) { \
      if (observerCopy[_obs_i]) \
      observerCopy[_obs_i]->_functionCall; \
    } \
  }



NS_IMETHODIMP
nsNavHistoryResult::OnBeginUpdateBatch()
{
  ENUMERATE_HISTORY_OBSERVERS(OnBeginUpdateBatch());
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnEndUpdateBatch()
{
  ENUMERATE_HISTORY_OBSERVERS(OnEndUpdateBatch());
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnItemAdded(PRInt64 aBookmarkId,
                                nsIURI *aBookmark,
                                PRInt64 aFolder,
                                PRInt32 aIndex)
{
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aFolder,
      OnItemAdded(aBookmarkId, aBookmark, aFolder, aIndex));
  ENUMERATE_HISTORY_OBSERVERS(OnItemAdded(aBookmarkId, aBookmark, aFolder, aIndex));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnItemRemoved(PRInt64 aBookmarkId, nsIURI *aBookmark,
                                  PRInt64 aFolder, PRInt32 aIndex)
{
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aFolder,
      OnItemRemoved(aBookmarkId, aBookmark, aFolder, aIndex));
  ENUMERATE_HISTORY_OBSERVERS(OnItemRemoved(aBookmarkId, aBookmark, aFolder, aIndex));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnItemChanged(PRInt64 aBookmarkId, nsIURI *aBookmark,
                                  const nsACString &aProperty,
                                  const nsAString &aValue)
{
  nsresult rv;
  nsNavBookmarks* bookmarkService = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarkService, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt64 folderId;
  rv = bookmarkService->GetFolderIdForItem(aBookmarkId, &folderId);
  NS_ENSURE_SUCCESS(rv, rv);
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(folderId,
      OnItemChanged(aBookmarkId, aBookmark, aProperty, aValue));

  
  
  
  
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnItemVisited(PRInt64 aBookmarkId, nsIURI* aBookmark,
                                  PRInt64 aVisitId, PRTime aVisitTime)
{
  nsresult rv;
  nsNavBookmarks* bookmarkService = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarkService, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt64 folderId;
  rv = bookmarkService->GetFolderIdForItem(aBookmarkId, &folderId);
  NS_ENSURE_SUCCESS(rv, rv);
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(folderId,
      OnItemVisited(aBookmarkId, aBookmark, aVisitId, aVisitTime));

  
  
  
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnFolderAdded(PRInt64 aFolder,
                                  PRInt64 aParent, PRInt32 aIndex)
{
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aParent,
      OnFolderAdded(aFolder, aParent, aIndex));
  ENUMERATE_HISTORY_OBSERVERS(OnFolderAdded(aFolder, aParent, aIndex));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnFolderRemoved(PRInt64 aFolder,
                                    PRInt64 aParent, PRInt32 aIndex)
{
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aParent,
      OnFolderRemoved(aFolder, aParent, aIndex));
  ENUMERATE_HISTORY_OBSERVERS(OnFolderRemoved(aFolder, aParent, aIndex));
  return NS_OK;
}







NS_IMETHODIMP
nsNavHistoryResult::OnFolderMoved(PRInt64 aFolder,
                                  PRInt64 aOldParent, PRInt32 aOldIndex,
                                  PRInt64 aNewParent, PRInt32 aNewIndex)
{
  { 
    ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aOldParent,
        OnFolderMoved(aFolder, aOldParent, aOldIndex, aNewParent, aNewIndex));
  }
  if (aNewParent != aOldParent) {
    ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aNewParent,
        OnFolderMoved(aFolder, aOldParent, aOldIndex, aNewParent, aNewIndex));
  }
  ENUMERATE_HISTORY_OBSERVERS(OnFolderMoved(aFolder, aOldParent, aOldIndex,
                                            aNewParent, aNewIndex));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnFolderChanged(PRInt64 aFolder,
                                   const nsACString &aProperty)
{
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aFolder,
      OnFolderChanged(aFolder, aProperty));
  ENUMERATE_HISTORY_OBSERVERS(OnFolderChanged(aFolder, aProperty));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnSeparatorAdded(PRInt64 aParent, PRInt32 aIndex)
{
  
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aParent,
      OnSeparatorAdded(aParent, aIndex));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnSeparatorRemoved(PRInt64 aParent, PRInt32 aIndex)
{
  
  ENUMERATE_BOOKMARK_OBSERVERS_FOR_FOLDER(aParent,
      OnSeparatorRemoved(aParent, aIndex));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnVisit(nsIURI* aURI, PRInt64 aVisitId, PRTime aTime,
                            PRInt64 aSessionId, PRInt64 aReferringId,
                            PRUint32 aTransitionType)
{
  ENUMERATE_HISTORY_OBSERVERS(OnVisit(aURI, aVisitId, aTime, aSessionId,
                                      aReferringId, aTransitionType));
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResult::OnTitleChanged(nsIURI* aURI, const nsAString& aPageTitle,
                                  const nsAString& aUserTitle,
                                  PRBool aIsUserTitleChanged)
{
  ENUMERATE_HISTORY_OBSERVERS(OnTitleChanged(aURI, aPageTitle, aUserTitle,
                                             aIsUserTitleChanged));
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
nsNavHistoryResult::OnPageExpired(nsIURI* aURI, PRTime aVisitTime,
                                  PRBool aWholeEntry)
{
  return NS_OK;
}




NS_IMPL_ADDREF(nsNavHistoryResultTreeViewer)
NS_IMPL_RELEASE(nsNavHistoryResultTreeViewer)

NS_INTERFACE_MAP_BEGIN(nsNavHistoryResultTreeViewer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryResultTreeViewer)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryResultViewer)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryResultTreeViewer)
  NS_INTERFACE_MAP_ENTRY(nsITreeView)
NS_INTERFACE_MAP_END

nsNavHistoryResultTreeViewer::nsNavHistoryResultTreeViewer() :
  mCollapseDuplicates(PR_TRUE),
  mShowSessions(PR_FALSE)
{

}







nsresult
nsNavHistoryResultTreeViewer::FinishInit()
{
  if (mTree && mResult) {
    mResult->mRootNode->SetContainerOpen(PR_TRUE);
    SortingChanged(mResult->mSortingMode);
  }
  
  return BuildVisibleList();
}









void
nsNavHistoryResultTreeViewer::ComputeShowSessions()
{
  NS_ASSERTION(mResult, "Must have a result to show sessions!");
  mShowSessions = PR_FALSE;

  nsNavHistoryQueryOptions* options = mResult->mOptions;
  NS_ASSERTION(options, "navHistoryResults must have valid options");

  if (!options->ShowSessions())
    return; 

  if (options->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_VISIT &&
      options->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_FULL_VISIT)
    return; 

  if (mResult->mSortingMode != nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING &&
      mResult->mSortingMode != nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING)
    return; 

  
  
  PRUint32 groupCount;
  const PRUint32* groupings = options->GroupingMode(&groupCount);
  for (PRUint32 i = 0; i < groupCount; i ++) {
    if (groupings[i] != nsINavHistoryQueryOptions::GROUP_BY_DAY)
      return; 
  }

  mShowSessions = PR_TRUE;
}




nsNavHistoryResultTreeViewer::SessionStatus
nsNavHistoryResultTreeViewer::GetRowSessionStatus(PRInt32 row)
{
  NS_ASSERTION(row >= 0 && row < PRInt32(mVisibleElements.Length()),
               "Invalid row!");

  nsNavHistoryResultNode *node = mVisibleElements[row];
  if (! node->IsVisit())
    return Session_None; 
  nsNavHistoryVisitResultNode* visit = node->GetAsVisit();

  if (visit->mSessionId != 0) {
    if (row == 0) {
      return Session_Start;
    } else {
      nsNavHistoryResultNode* previousNode = mVisibleElements[row - 1];
      if (previousNode->IsVisit() &&
          visit->mSessionId != previousNode->GetAsVisit()->mSessionId) {
        return Session_Start;
      } else {
        return Session_Continue;
      }
    }
  }
  return Session_None;
}










nsresult
nsNavHistoryResultTreeViewer::BuildVisibleList()
{
  if (mResult) {
    
    
    
    
    for (PRUint32 i = 0; i < mVisibleElements.Length(); i ++)
      mVisibleElements[i]->mViewIndex = -1;
  }
  mVisibleElements.Clear();

  if (mResult->mRootNode && mTree) {
    ComputeShowSessions();
    return BuildVisibleSection(mResult->mRootNode, &mVisibleElements, 0);
  }
  return NS_OK;
}















nsresult
nsNavHistoryResultTreeViewer::BuildVisibleSection(
    nsNavHistoryContainerResultNode* aContainer,
    VisibleList* aVisible,
    PRUint32 aVisibleStartIndex)
{
  if (! aContainer->mExpanded)
    return NS_OK; 
  for (PRInt32 i = 0; i < aContainer->mChildren.Count(); i ++) {
    nsNavHistoryResultNode* cur = aContainer->mChildren[i];

    
    if (mCollapseDuplicates) {
      PRUint32 showThis;
      while (i < aContainer->mChildren.Count() - 1 &&
             CanCollapseDuplicates(cur, aContainer->mChildren[i+1], &showThis)) {
        if (showThis) {
          
          cur->mViewIndex = -1;
          cur = aContainer->mChildren[i + 1];
        } else {
          
          aContainer->mChildren[i + 1]->mViewIndex = -1;
        }
        i ++;
      }
    }

    
    if (cur->IsSeparator()) {
      if (aContainer->GetSortType() != nsINavHistoryQueryOptions::SORT_BY_NONE) {
        cur->mViewIndex = -1;
        continue;
      }
    }

    
    cur->mViewIndex = aVisibleStartIndex + aVisible->Length();
    if (! aVisible->AppendElement(cur))
      return NS_ERROR_OUT_OF_MEMORY;

    
    if (cur->IsContainer()) {
      nsNavHistoryContainerResultNode* curContainer = cur->GetAsContainer();
      if (curContainer->mExpanded && curContainer->mChildren.Count() > 0) {
        nsresult rv = BuildVisibleSection(curContainer, aVisible,
                                          aVisibleStartIndex);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  return NS_OK;
}









PRUint32
nsNavHistoryResultTreeViewer::CountVisibleRowsForItem(
                                                 nsNavHistoryResultNode* aNode)
{
  NS_ASSERTION(aNode->mViewIndex >= 0, "Item is not visible, no rows to count");
  PRInt32 outerLevel = aNode->mIndentLevel;
  for (PRUint32 i = aNode->mViewIndex + 1; i < mVisibleElements.Length(); i ++) {
    if (mVisibleElements[i]->mIndentLevel <= outerLevel)
      return i - aNode->mViewIndex;
  }
  
  return mVisibleElements.Length() - aNode->mViewIndex;
}











nsresult
nsNavHistoryResultTreeViewer::RefreshVisibleSection(
                                    nsNavHistoryContainerResultNode* aContainer)
{
  NS_ASSERTION(mResult, "Need to have a result to update");
  if (! mTree)
    return NS_OK;

  
  
  if (aContainer == mResult->mRootNode)
    return BuildVisibleList();

  
  NS_ASSERTION(aContainer->mViewIndex >= 0 &&
               aContainer->mViewIndex < PRInt32(mVisibleElements.Length()),
               "Trying to expand a node that is not visible");
  NS_ASSERTION(mVisibleElements[aContainer->mViewIndex] == aContainer,
               "Visible index is out of sync!");

  PRUint32 startReplacement = aContainer->mViewIndex + 1;
  PRUint32 replaceCount = CountVisibleRowsForItem(aContainer) - 1;

  
  PRUint32 i;
  for (i = startReplacement; i < replaceCount; i ++)
    mVisibleElements[startReplacement + i]->mViewIndex = -1;

  
  VisibleList newElements;
  nsresult rv = BuildVisibleSection(aContainer, &newElements, startReplacement);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (! mVisibleElements.ReplaceElementsAt(aContainer->mViewIndex + 1,
      replaceCount, newElements.Elements(), newElements.Length()))
    return NS_ERROR_OUT_OF_MEMORY;

  if (replaceCount == newElements.Length()) {
    
    if (replaceCount > 0)
      mTree->InvalidateRange(startReplacement, startReplacement + replaceCount - 1);
  } else {
    
    
    for (i = startReplacement + newElements.Length();
         i < mVisibleElements.Length(); i ++)
      mVisibleElements[i]->mViewIndex = i;

    
    
    PRUint32 minLength;
    if (replaceCount > newElements.Length())
      minLength = newElements.Length();
    else
      minLength = replaceCount;
    mTree->InvalidateRange(startReplacement - 1, startReplacement + minLength - 1);

    
    mTree->RowCountChanged(startReplacement + minLength,
                            newElements.Length() - replaceCount);
  }
  return NS_OK;
}








PRBool
nsNavHistoryResultTreeViewer::CanCollapseDuplicates(
    nsNavHistoryResultNode* aTop, nsNavHistoryResultNode* aNext,
    PRUint32* aShowThisOne)
{
  if (! mCollapseDuplicates)
    return PR_FALSE;
  if (! aTop->IsVisit() || ! aNext->IsVisit())
    return PR_FALSE; 

  nsNavHistoryVisitResultNode* topVisit = aTop->GetAsVisit();
  nsNavHistoryVisitResultNode* nextVisit = aNext->GetAsVisit();

  if (! topVisit->mURI.Equals(nextVisit->mURI))
    return PR_FALSE; 

  
  *aShowThisOne = topVisit->mTime < nextVisit->mTime;
  return PR_TRUE;
}




nsNavHistoryResultTreeViewer::ColumnType
nsNavHistoryResultTreeViewer::GetColumnType(nsITreeColumn* col)
{
  const PRUnichar* idConst;
  col->GetIdConst(&idConst);
  switch(idConst[0]) {
    case PRUnichar('t'):
      return Column_Title;
    case PRUnichar('u'):
      return Column_URI;
    case PRUnichar('d'):
      return Column_Date;
    case PRUnichar('v'):
      return Column_VisitCount;
    default:
      return Column_Unknown;
  }
}






nsNavHistoryResultTreeViewer::ColumnType
nsNavHistoryResultTreeViewer::SortTypeToColumnType(PRUint32 aSortType,
                                                   PRBool* aDescending)
{
  switch(aSortType) {
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING:
      *aDescending = PR_FALSE;
      return Column_Title;
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING:
      *aDescending = PR_TRUE;
      return Column_Title;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING:
      *aDescending = PR_FALSE;
      return Column_Date;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING:
      *aDescending = PR_TRUE;
      return Column_Date;
    case nsINavHistoryQueryOptions::SORT_BY_URI_ASCENDING:
      *aDescending = PR_FALSE;
      return Column_URI;
    case nsINavHistoryQueryOptions::SORT_BY_URI_DESCENDING:
      *aDescending = PR_TRUE;
      return Column_URI;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING:
      *aDescending = PR_FALSE;
      return Column_VisitCount;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING:
      *aDescending = PR_TRUE;
      return Column_VisitCount;
    default:
      *aDescending = PR_FALSE;
      return Column_Unknown;
  }
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::ItemInserted(
    nsINavHistoryContainerResultNode *aParent, nsINavHistoryResultNode *aItem,
    PRUint32 aNewIndex)
{
  if (! mTree)
    return NS_OK; 
  if (! mResult) {
    NS_NOTREACHED("Got an inserted message with no result");
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<nsNavHistoryContainerResultNode> parent;
  aParent->QueryInterface(NS_GET_IID(nsNavHistoryContainerResultNode),
                          getter_AddRefs(parent));
  if (! parent) {
    NS_NOTREACHED("Parent is not a concrete node");
    return NS_ERROR_UNEXPECTED;
  }
  nsCOMPtr<nsNavHistoryResultNode> item = do_QueryInterface(aItem);
  if (! aItem) {
    NS_NOTREACHED("Item is not a concrete node");
    return NS_ERROR_UNEXPECTED;
  }

  
  if (item->IsSeparator()) {
    if (parent->GetSortType() != nsINavHistoryQueryOptions::SORT_BY_NONE) {
      item->mViewIndex = -1;
      return NS_OK;
    }
  }

  
  if (parent->mChildren.Count() == 1)
    ItemChanged(aParent);

  
  PRInt32 newViewIndex = -1;
  if (aNewIndex == 0) {
    
    
    
    newViewIndex = parent->mViewIndex + 1;
  } else {
    
    
    
    
    for (PRInt32 i = aNewIndex + 1; i < parent->mChildren.Count(); i ++) {
      if (parent->mChildren[i]->mViewIndex >= 0) {
        
        
        newViewIndex = parent->mChildren[i]->mViewIndex;
        break;
      }
    }
    if (newViewIndex < 0) {
      
      
      
      PRUint32 lastRowCount =
        CountVisibleRowsForItem(parent->mChildren[aNewIndex - 1]);
      newViewIndex = parent->mChildren[aNewIndex - 1]->mViewIndex +
        lastRowCount;
    }
  }

  
  
  
  
  PRUint32 showThis;
  if (newViewIndex > 0 &&
      CanCollapseDuplicates(mVisibleElements[newViewIndex - 1],
                            item, &showThis)) {
    if (showThis == 0) {
      
      item->mViewIndex = -1;
      return NS_OK;
    } else {
      
      ItemReplaced(parent, mVisibleElements[newViewIndex - 1], item, 0);
      return NS_OK;
    }
  }

  
  
  if (newViewIndex < PRInt32(mVisibleElements.Length()) &&
      CanCollapseDuplicates(item, mVisibleElements[newViewIndex],
                            &showThis)) {
    if (showThis == 0) {
      
      ItemReplaced(parent, mVisibleElements[newViewIndex], item, 0);
      return NS_OK;
    } else {
      
      item->mViewIndex = -1;
      return NS_OK;
    }
  }

  
  item->mViewIndex = newViewIndex;
  mVisibleElements.InsertElementAt(newViewIndex, item);
  for (PRUint32 i = newViewIndex + 1; i < mVisibleElements.Length(); i ++)
    mVisibleElements[i]->mViewIndex = i;
  mTree->RowCountChanged(newViewIndex, 1);

  
  
  
  if (mShowSessions) {
    if (newViewIndex > 0)
      mTree->InvalidateRange(newViewIndex - 1, newViewIndex - 1);
    if (newViewIndex < PRInt32(mVisibleElements.Length()) - 1)
      mTree->InvalidateRange(newViewIndex + 1, newViewIndex + 1);
  }

  if (item->IsContainer() && item->GetAsContainer()->mExpanded)
    RefreshVisibleSection(item->GetAsContainer());
  return NS_OK;
}













NS_IMETHODIMP
nsNavHistoryResultTreeViewer::ItemRemoved(
    nsINavHistoryContainerResultNode *aParent, nsINavHistoryResultNode *aItem,
    PRUint32 aOldIndex)
{
  NS_ASSERTION(mResult, "Got a notification but have no result!");
  if (! mTree)
      return NS_OK; 

  nsCOMPtr<nsNavHistoryResultNode> item = do_QueryInterface(aItem);
  NS_ENSURE_TRUE(item, NS_ERROR_INVALID_ARG);

  PRInt32 oldViewIndex = item->mViewIndex;
  if (oldViewIndex < 0)
    return NS_OK; 

  
  PRInt32 count = CountVisibleRowsForItem(item);

  
  
  while (PR_TRUE) {
    if (oldViewIndex > NS_STATIC_CAST(PRInt32, mVisibleElements.Length())) {
      NS_NOTREACHED("Trying to remove invalid row");
      return NS_OK;
    }
    mVisibleElements.RemoveElementsAt(oldViewIndex, count);
    for (PRUint32 i = oldViewIndex; i < mVisibleElements.Length(); i ++)
      mVisibleElements[i]->mViewIndex = i;

    mTree->RowCountChanged(oldViewIndex, -count);

    
    if (oldViewIndex > 0 &&
        oldViewIndex < NS_STATIC_CAST(PRInt32, mVisibleElements.Length())) {
      PRUint32 showThisOne;
      if (CanCollapseDuplicates(mVisibleElements[oldViewIndex - 1],
                                mVisibleElements[oldViewIndex], &showThisOne))
      {
        
        
        
        
        oldViewIndex = oldViewIndex - 1 + showThisOne;
        mVisibleElements[oldViewIndex]->mViewIndex = -1;
        count = 1; 
        continue;
      }
    }
    break; 
  }

  
  PRBool hasChildren;
  aParent->GetHasChildren(&hasChildren);
  if (! hasChildren)
    ItemChanged(aParent);
  return NS_OK;
}












NS_IMETHODIMP
nsNavHistoryResultTreeViewer::ItemReplaced(
    nsINavHistoryContainerResultNode* parent, nsINavHistoryResultNode* aOldItem,
    nsINavHistoryResultNode* aNewItem, PRUint32 aIndexDoNotUse)
{
  if (! mTree)
    return NS_OK;

  PRInt32 viewIndex;
  aOldItem->GetViewIndex(&viewIndex);
  aNewItem->SetViewIndex(viewIndex);
  if (viewIndex >= 0 &&
      viewIndex < NS_STATIC_CAST(PRInt32, mVisibleElements.Length())) {
    nsCOMPtr<nsNavHistoryResultNode> newItem = do_QueryInterface(aNewItem);
    NS_ASSERTION(newItem, "Node is not one of our concrete ones");
    mVisibleElements[viewIndex] = newItem;
  }
  aOldItem->SetViewIndex(-1);
  mTree->InvalidateRow(viewIndex);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::ItemChanged(nsINavHistoryResultNode *item)
{
  NS_ASSERTION(mResult, "Got a notification but have no result!");
  PRInt32 viewIndex;
  item->GetViewIndex(&viewIndex);
  if (mTree && viewIndex >= 0)
    return mTree->InvalidateRow(viewIndex);
  return NS_OK; 
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::ContainerOpened(
    nsINavHistoryContainerResultNode *item)
{
  NS_ASSERTION(mResult, "Got a notification but have no result!");
  if (! mTree || item < 0)
    return NS_OK; 
  PRInt32 viewIndex;
  item->GetViewIndex(&viewIndex);
  if (viewIndex >= NS_STATIC_CAST(PRInt32, mVisibleElements.Length())) {
    
    NS_NOTREACHED("Invalid visible index");
    return NS_ERROR_UNEXPECTED;
  }
  return RefreshVisibleSection(
      NS_STATIC_CAST(nsNavHistoryContainerResultNode*, item));
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::ContainerClosed(
    nsINavHistoryContainerResultNode *item)
{
  NS_ASSERTION(mResult, "Got a notification but have no result!");
  if (! mTree || item < 0)
    return NS_OK; 

  PRInt32 viewIndex;
  item->GetViewIndex(&viewIndex);
  if (viewIndex >= NS_STATIC_CAST(PRInt32, mVisibleElements.Length())) {
    
    NS_NOTREACHED("Invalid visible index");
    return NS_ERROR_UNEXPECTED;
  }
  return RefreshVisibleSection(
      NS_STATIC_CAST(nsNavHistoryContainerResultNode*, item));
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::InvalidateContainer(
                                       nsINavHistoryContainerResultNode* aItem)
{
  NS_ASSERTION(mResult, "Got message but don't have a result!");
  if (! mTree)
    return NS_OK;

  return RefreshVisibleSection(
      NS_STATIC_CAST(nsNavHistoryContainerResultNode*, aItem));
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::InvalidateAll()
{
  NS_ASSERTION(mResult, "Got message but don't have a result!");
  if (! mTree)
    return NS_OK;

  PRInt32 oldRowCount = mVisibleElements.Length();

  
  nsresult rv = BuildVisibleList();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mTree->RowCountChanged(0, mVisibleElements.Length() - oldRowCount);
  NS_ENSURE_SUCCESS(rv, rv);
  return mTree->Invalidate();
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::SortingChanged(PRUint32 aSortingMode)
{
  if (! mTree || ! mResult)
    return NS_OK;

  nsCOMPtr<nsITreeColumns> columns;
  nsresult rv = mTree->GetColumns(getter_AddRefs(columns));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(sortDirectionKey, "sortDirection");

  
  
  nsCOMPtr<nsITreeColumn> column;
  rv = columns->GetSortedColumn(getter_AddRefs(column));
  NS_ENSURE_SUCCESS(rv, rv);
  if (column) {
    nsCOMPtr<nsIDOMElement> element;
    rv = column->GetElement(getter_AddRefs(element));
  NS_ENSURE_SUCCESS(rv, rv);
    element->SetAttribute(sortDirectionKey, EmptyString());
  }

  
  if (aSortingMode == nsINavHistoryQueryOptions::SORT_BY_NONE)
    return NS_OK;
  PRBool desiredIsDescending;
  ColumnType desiredColumn = SortTypeToColumnType(aSortingMode,
                                                  &desiredIsDescending);
  PRInt32 colCount;
  rv = columns->GetCount(&colCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRInt32 i = 0; i < colCount; i ++) {
    columns->GetColumnAt(i, getter_AddRefs(column));
    NS_ENSURE_SUCCESS(rv, rv);
    if (GetColumnType(column) == desiredColumn) {
      
      nsCOMPtr<nsIDOMElement> element;
      rv = column->GetElement(getter_AddRefs(element));
      NS_ENSURE_SUCCESS(rv, rv);
      if (desiredIsDescending)
        element->SetAttribute(sortDirectionKey, NS_LITERAL_STRING("descending"));
      else
        element->SetAttribute(sortDirectionKey, NS_LITERAL_STRING("ascending"));
      break;
    }
  }

  return NS_OK;
}









NS_IMETHODIMP
nsNavHistoryResultTreeViewer::GetResult(nsINavHistoryResult** aResult)
{
  *aResult = mResult;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryResultTreeViewer::SetResult(nsINavHistoryResult* aResult)
{
  if (aResult) {
    nsresult rv = aResult->QueryInterface(NS_GET_IID(nsNavHistoryResult),
                                          getter_AddRefs(mResult));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    mResult = nsnull;
  }
  return FinishInit();
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::AddViewObserver(
                  nsINavHistoryResultViewObserver* aObserver, PRBool aOwnsWeak)
{
  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::RemoveViewObserver(
                                    nsINavHistoryResultViewObserver* aObserver)
{
  return mObservers.RemoveWeakElement(aObserver);
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::GetCollapseDuplicates(PRBool* aCollapseDuplicates)
{
  *aCollapseDuplicates = mCollapseDuplicates;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryResultTreeViewer::SetCollapseDuplicates(PRBool aCollapseDuplicates)
{
  if ((aCollapseDuplicates && mCollapseDuplicates) ||
      (! aCollapseDuplicates && ! mCollapseDuplicates))
    return NS_OK; 

  mCollapseDuplicates = aCollapseDuplicates;

  if (mResult && mTree)
    InvalidateAll();
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::GetFlatItemCount(PRUint32 *aItemCount)
{
  *aItemCount = mVisibleElements.Length();
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::NodeForTreeIndex(
    PRUint32 index, nsINavHistoryResultNode** aResult)
{
  if (index >= PRUint32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  NS_ADDREF(*aResult = mVisibleElements[index]);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::TreeIndexForNode(nsINavHistoryResultNode* aNode,
                                               PRUint32* aResult)
{
  nsresult rv;
  nsCOMPtr<nsNavHistoryResultNode> node = do_QueryInterface(aNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (node->mViewIndex < 0) {
    *aResult = nsINavHistoryResultTreeViewer::INDEX_INVISIBLE;
  } else {
    *aResult = node->mViewIndex;
    NS_ASSERTION(mVisibleElements[*aResult] == node,
                 "Node's visible index and array out of sync");
  }
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = mVisibleElements.Length();
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetSelection(nsITreeSelection** aSelection)
{
  if (! mSelection) {
    *aSelection = nsnull;
    return NS_ERROR_FAILURE;
  }
  NS_ADDREF(*aSelection = mSelection);
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryResultTreeViewer::SetSelection(nsITreeSelection* aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetRowProperties(PRInt32 row,
                                                   nsISupportsArray *properties)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;

  nsNavHistoryResultNode *node = mVisibleElements[row];

  
  if (node->IsContainer()) {
    properties->AppendElement(nsNavHistory::sContainerAtom);
  }

  
  if (! mShowSessions)
    return NS_OK; 

  switch (GetRowSessionStatus(row))
  {
    case Session_Start:
      properties->AppendElement(nsNavHistory::sSessionStartAtom);
      break;
    case Session_Continue:
      properties->AppendElement(nsNavHistory::sSessionContinueAtom);
      break;
    case Session_None:
      break;
    default: 
      NS_NOTREACHED("Invalid session type");
  }
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::GetCellProperties(PRInt32 row, nsITreeColumn *col,
                                      nsISupportsArray *properties)
{
  

























  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::GetColumnProperties(nsITreeColumn *col,
                                        nsISupportsArray *properties)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::IsContainer(PRInt32 row, PRBool *_retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  *_retval = mVisibleElements[row]->IsContainer();
  if (*_retval) {
    
    if (mVisibleElements[row]->IsQuery() &&
        ! mVisibleElements[row]->GetAsQuery()->CanExpand())
      *_retval = PR_FALSE;
  }
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::IsContainerOpen(PRInt32 row, PRBool *_retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  if (! mVisibleElements[row]->IsContainer())
    return NS_ERROR_INVALID_ARG;

  nsINavHistoryContainerResultNode* container =
    mVisibleElements[row]->GetAsContainer();
  return container->GetContainerOpen(_retval);
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::IsContainerEmpty(PRInt32 row, PRBool *_retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  if (! mVisibleElements[row]->IsContainer())
    return NS_ERROR_INVALID_ARG;

  nsINavHistoryContainerResultNode* container =
    mVisibleElements[row]->GetAsContainer();
  PRBool hasChildren = PR_FALSE;
  nsresult rv = container->GetHasChildren(&hasChildren);
  *_retval = ! hasChildren;
  return rv;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::IsSeparator(PRInt32 row, PRBool *_retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;

  *_retval = mVisibleElements[row]->IsSeparator();
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::IsSorted(PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::CanDrop(PRInt32 row, PRInt32 orientation,
                                          PRBool *_retval)
{
  PRUint32 count = mObservers.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    const nsCOMPtr<nsINavHistoryResultViewObserver> &obs = mObservers[i];
    if (obs) {
      obs->CanDrop(row, orientation, _retval);
      if (*_retval) {
        break;
      }
    }
  }
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::Drop(PRInt32 row, PRInt32 orientation)
{
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnDrop(row, orientation))
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetParentIndex(PRInt32 row, PRInt32* _retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  nsNavHistoryResultNode* parent = mVisibleElements[row]->mParent;
  if (! parent || parent->mViewIndex < 0)
    *_retval = -1;
  else
    *_retval = parent->mViewIndex;
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::HasNextSibling(PRInt32 row,
                                                 PRInt32 afterIndex,
                                                 PRBool* _retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  if (row == PRInt32(mVisibleElements.Length()) - 1) {
    
    *_retval = PR_FALSE;
    return NS_OK;
  }
  *_retval = (mVisibleElements[row]->mIndentLevel ==
              mVisibleElements[row + 1]->mIndentLevel);
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetLevel(PRInt32 row, PRInt32* _retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  *_retval = mVisibleElements[row]->mIndentLevel;
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetImageSrc(PRInt32 row, nsITreeColumn *col,
                                              nsAString& _retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;

  
  PRInt32 colIndex;
  col->GetIndex(&colIndex);
  if (colIndex != 0) {
    _retval.Truncate(0);
    return NS_OK;
  }

  nsNavHistoryResultNode* node = mVisibleElements[row];

  
  
  
  if (node->IsSeparator() ||
      (node->IsContainer() && node->mFaviconURI.IsEmpty())) {
    _retval.Truncate(0);
    return NS_OK;
  }

  
  
  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_NO_INTERFACE);

  nsCAutoString spec;
  faviconService->GetFaviconSpecForIconString(
                                      mVisibleElements[row]->mFaviconURI, spec);
  CopyUTF8toUTF16(spec, _retval);
  return NS_OK;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetProgressMode(PRInt32 row,
                                                  nsITreeColumn* col,
                                                  PRInt32* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetCellValue(
    PRInt32 row, nsITreeColumn *col, nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP nsNavHistoryResultTreeViewer::GetCellText(PRInt32 row,
                                              nsITreeColumn *col,
                                              nsAString& _retval)
{
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;
  PRInt32 colIndex;
  col->GetIndex(&colIndex);

  nsNavHistoryResultNode* node = mVisibleElements[row];

  switch (GetColumnType(col)) {
    case Column_Title:
    {
      
      
      
      
      if (node->IsSeparator()) {
        _retval.Truncate(0);
      } else if (! node->mTitle.IsEmpty()) {
        _retval = NS_ConvertUTF8toUTF16(node->mTitle);
      } else {
        nsXPIDLString value;
        nsresult rv =
          nsNavHistory::GetHistoryService()->GetBundle()->GetStringFromName(
            NS_LITERAL_STRING("noTitle").get(), getter_Copies(value));
        NS_ENSURE_SUCCESS(rv, rv);
        _retval = value;
      }
#if 0
      
      
      nsString newTitle;
      newTitle.AssignLiteral("(");
      newTitle.AppendInt(node->mBookmarkIndex);
      newTitle.AppendLiteral(") ");
      newTitle.Append(_retval);
      _retval = newTitle;
#endif
      break;
    }
    case Column_URI:
    {
      if (node->IsURI())
        _retval = NS_ConvertUTF8toUTF16(node->mURI);
      else
        _retval.Truncate(0);
      break;
    }
    case Column_Date:
    {
      if (node->mTime == 0 || ! node->IsURI()) {
        
        
        
        
        _retval.Truncate(0);
      } else {
        if (GetRowSessionStatus(row) != Session_Continue)
          return FormatFriendlyTime(node->mTime, _retval);
        _retval.Truncate(0);
        break;
      }
      break;
    }
    case Column_VisitCount:
    {
      if (node->IsSeparator()) {
        _retval.Truncate(0);
      } else {
        _retval = NS_ConvertUTF8toUTF16(nsPrintfCString("%d", node->mAccessCount));
      }
      break;
    }
    default:
      return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}









NS_IMETHODIMP nsNavHistoryResultTreeViewer::SetTree(nsITreeBoxObject* tree)
{
  PRBool hasOldTree = (mTree != nsnull);
  mTree = tree;

  
  
  FinishInit();

  if (! tree && hasOldTree && mResult) {
    
    
    mResult->SetViewer(nsnull);
  }
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::ToggleOpenState(PRInt32 row)
{
  if (! mResult)
    return NS_ERROR_UNEXPECTED;
  if (row < 0 || row >= PRInt32(mVisibleElements.Length()))
    return NS_ERROR_INVALID_ARG;

  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnToggleOpenState(row))

  nsNavHistoryResultNode* node = mVisibleElements[row];
  if (! node->IsContainer())
    return NS_OK; 
  nsNavHistoryContainerResultNode* container = node->GetAsContainer();

  nsresult rv;
  if (container->mExpanded)
    rv = container->CloseContainer();
  else
    rv = container->OpenContainer();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}







NS_IMETHODIMP
nsNavHistoryResultTreeViewer::CycleHeader(nsITreeColumn* col)
{
  if (! mResult)
    return NS_ERROR_UNEXPECTED;

  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnCycleHeader(col))

  PRInt32 colIndex;
  col->GetIndex(&colIndex);

  
  
  
  
  
  
  
  
  
  
  PRBool allowTriState = PR_FALSE;
  if (mResult->mRootNode->IsFolder())
    allowTriState = PR_TRUE;

  PRInt32 oldSort = mResult->mSortingMode;
  PRInt32 newSort;
  switch (GetColumnType(col)) {
    case Column_Title:
      if (oldSort == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING) {
        newSort = nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING;
      } else {
        if (allowTriState && oldSort == nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING)
          newSort = nsINavHistoryQueryOptions::SORT_BY_NONE;
        else
          newSort = nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING;
      }
      break;
    case Column_URI:
      if (oldSort == nsINavHistoryQueryOptions::SORT_BY_URI_ASCENDING) {
        newSort = nsINavHistoryQueryOptions::SORT_BY_URI_DESCENDING;
      } else {
        if (allowTriState && oldSort == nsINavHistoryQueryOptions::SORT_BY_URI_DESCENDING)
          newSort = nsINavHistoryQueryOptions::SORT_BY_NONE;
        else
          newSort = nsINavHistoryQueryOptions::SORT_BY_URI_ASCENDING;
      }
      break;
    case Column_Date:
      if (oldSort == nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING) {
        newSort = nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING;
      } else {
        if (allowTriState && oldSort == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING)
          newSort = nsINavHistoryQueryOptions::SORT_BY_NONE;
        else
          newSort = nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING;
      }
      break;
    case Column_VisitCount:
      
      
      
      if (oldSort == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING) {
        newSort = nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING;
      } else {
        if (allowTriState && oldSort == nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING)
          newSort = nsINavHistoryQueryOptions::SORT_BY_NONE;
        else
          newSort = nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING;
      }
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  return mResult->SetSortingMode(newSort);
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::SelectionChanged()
{
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnSelectionChanged())
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::CycleCell(PRInt32 row, nsITreeColumn* col)
{
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnCycleCell(row, col))
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::IsEditable(PRInt32 row, nsITreeColumn* col,
                                         PRBool* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
nsNavHistoryResultTreeViewer::IsSelectable(PRInt32 row, nsITreeColumn* col,
                                           PRBool* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::SetCellValue(PRInt32 row, nsITreeColumn* col,
                                           const nsAString & value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::SetCellText(PRInt32 row, nsITreeColumn* col,
                                          const nsAString & value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::PerformAction(const PRUnichar* action)
{
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnPerformAction(action))
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::PerformActionOnRow(const PRUnichar* action,
                                                 PRInt32 row)
{
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnPerformActionOnRow(action, row))
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistoryResultTreeViewer::PerformActionOnCell(const PRUnichar* action,
                                                  PRInt32 row,
                                                  nsITreeColumn* col)
{
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryResultViewObserver,
                      OnPerformActionOnCell(action, row, col))
  return NS_OK;
}




nsresult
nsNavHistoryResultTreeViewer::FormatFriendlyTime(PRTime aTime,
                                                 nsAString& aResult)
{
  
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  PRTime now = history->GetNow();

  const PRInt64 ago = now - aTime;

  








































  
  
  
  
  
  nsDateFormatSelector dateFormat = nsIScriptableDateFormat::dateFormatShort;
  if (ago > -10000000 && ago < (PRInt64)1000000 * 24 * 60 * 60) {
    PRExplodedTime explodedTime;
    PR_ExplodeTime(aTime, PR_LocalTimeParameters, &explodedTime);
    explodedTime.tm_min =
      explodedTime.tm_hour =
      explodedTime.tm_sec =
      explodedTime.tm_usec = 0;
    PRTime midnight = PR_ImplodeTime(&explodedTime);
    if (aTime > midnight)
      dateFormat = nsIScriptableDateFormat::dateFormatNone;
  }
  nsAutoString resultString;
  history->GetDateFormatter()->FormatPRTime(history->GetLocale(), dateFormat,
                               nsIScriptableDateFormat::timeFormatNoSeconds,
                               aTime, resultString);
  aResult = resultString;
  return NS_OK;
}
