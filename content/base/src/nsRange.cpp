








#include "nscore.h"
#include "nsRange.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMDocumentType.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMText.h"
#include "nsError.h"
#include "nsIContentIterator.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsGenericDOMDataNode.h"
#include "nsClientRect.h"
#include "nsLayoutUtils.h"
#include "nsTextFrame.h"
#include "nsFontFaceList.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Likely.h"

using namespace mozilla;







#define VALIDATE_ACCESS(node_)                                                     \
  PR_BEGIN_MACRO                                                                   \
    if (!node_) {                                                                  \
      return NS_ERROR_DOM_NOT_OBJECT_ERR;                                          \
    }                                                                              \
    if (!nsContentUtils::CanCallerAccess(node_)) {                                 \
      return NS_ERROR_DOM_SECURITY_ERR;                                            \
    }                                                                              \
  PR_END_MACRO

static void InvalidateAllFrames(nsINode* aNode)
{
  NS_PRECONDITION(aNode, "bad arg");

  nsIFrame* frame = nullptr;
  switch (aNode->NodeType()) {
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::ELEMENT_NODE:
    {
      nsIContent* content = static_cast<nsIContent*>(aNode);
      frame = content->GetPrimaryFrame();
      break;
    }
    case nsIDOMNode::DOCUMENT_NODE:
    {
      nsIDocument* doc = static_cast<nsIDocument*>(aNode);
      nsIPresShell* shell = doc ? doc->GetShell() : nullptr;
      frame = shell ? shell->GetRootFrame() : nullptr;
      break;
    }
  }
  for (nsIFrame* f = frame; f; f = f->GetNextContinuation()) {
    f->InvalidateFrameSubtree();
  }
}









nsresult
nsRange::CompareNodeToRange(nsINode* aNode, nsRange* aRange,
                            bool *outNodeBefore, bool *outNodeAfter)
{
  NS_ENSURE_STATE(aNode);
  
  
  
  
  
  
  
  if (!aRange || !aRange->IsPositioned()) 
    return NS_ERROR_UNEXPECTED; 
  
  
  int32_t nodeStart, nodeEnd;
  nsINode* parent = aNode->GetParentNode();
  if (!parent) {
    
    
    
    parent = aNode;
    nodeStart = 0;
    nodeEnd = aNode->GetChildCount();
  }
  else {
    nodeStart = parent->IndexOf(aNode);
    nodeEnd = nodeStart + 1;
  }

  nsINode* rangeStartParent = aRange->GetStartParent();
  nsINode* rangeEndParent = aRange->GetEndParent();
  int32_t rangeStartOffset = aRange->StartOffset();
  int32_t rangeEndOffset = aRange->EndOffset();

  
  bool disconnected = false;
  *outNodeBefore = nsContentUtils::ComparePoints(rangeStartParent,
                                                 rangeStartOffset,
                                                 parent, nodeStart,
                                                 &disconnected) > 0;
  NS_ENSURE_TRUE(!disconnected, NS_ERROR_DOM_WRONG_DOCUMENT_ERR);

  
  *outNodeAfter = nsContentUtils::ComparePoints(rangeEndParent,
                                                rangeEndOffset,
                                                parent, nodeEnd,
                                                &disconnected) < 0;
  NS_ENSURE_TRUE(!disconnected, NS_ERROR_DOM_WRONG_DOCUMENT_ERR);
  return NS_OK;
}

struct FindSelectedRangeData
{
  nsINode*  mNode;
  nsRange* mResult;
  uint32_t  mStartOffset;
  uint32_t  mEndOffset;
};

static PLDHashOperator
FindSelectedRange(nsPtrHashKey<nsRange>* aEntry, void* userArg)
{
  nsRange* range = aEntry->GetKey();
  if (range->IsInSelection() && !range->Collapsed()) {
    FindSelectedRangeData* data = static_cast<FindSelectedRangeData*>(userArg);
    int32_t cmp = nsContentUtils::ComparePoints(data->mNode, data->mEndOffset,
                                                range->GetStartParent(),
                                                range->StartOffset());
    if (cmp == 1) {
      cmp = nsContentUtils::ComparePoints(data->mNode, data->mStartOffset,
                                          range->GetEndParent(),
                                          range->EndOffset());
      if (cmp == -1) {
        data->mResult = range;
        return PL_DHASH_STOP;
      }
    }
  }
  return PL_DHASH_NEXT;
}

static nsINode*
GetNextRangeCommonAncestor(nsINode* aNode)
{
  while (aNode && !aNode->IsCommonAncestorForRangeInSelection()) {
    if (!aNode->IsDescendantOfCommonAncestorForRangeInSelection()) {
      return nullptr;
    }
    aNode = aNode->GetParentNode();
  }
  return aNode;
}

 bool
nsRange::IsNodeSelected(nsINode* aNode, uint32_t aStartOffset,
                        uint32_t aEndOffset)
{
  NS_PRECONDITION(aNode, "bad arg");

  FindSelectedRangeData data = { aNode, nullptr, aStartOffset, aEndOffset };
  nsINode* n = GetNextRangeCommonAncestor(aNode);
  NS_ASSERTION(n || !aNode->IsSelectionDescendant(),
               "orphan selection descendant");
  for (; n; n = GetNextRangeCommonAncestor(n->GetParentNode())) {
    RangeHashTable* ranges =
      static_cast<RangeHashTable*>(n->GetProperty(nsGkAtoms::range));
    ranges->EnumerateEntries(FindSelectedRange, &data);
    if (data.mResult) {
      return true;
    }
  }
  return false;
}





nsRange::~nsRange() 
{
  NS_ASSERTION(!IsInSelection(), "deleting nsRange that is in use");

  
  Telemetry::Accumulate(Telemetry::DOM_RANGE_DETACHED, mIsDetached);

  
  DoSetRange(nullptr, 0, nullptr, 0, nullptr);
} 


nsresult
nsRange::CreateRange(nsIDOMNode* aStartParent, int32_t aStartOffset,
                     nsIDOMNode* aEndParent, int32_t aEndOffset,
                     nsRange** aRange)
{
  MOZ_ASSERT(aRange);
  *aRange = NULL;

  nsRefPtr<nsRange> range = new nsRange();

  nsresult rv = range->SetStart(aStartParent, aStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = range->SetEnd(aEndParent, aEndOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  range.forget(aRange);
  return NS_OK;
}


nsresult
nsRange::CreateRange(nsIDOMNode* aStartParent, int32_t aStartOffset,
                     nsIDOMNode* aEndParent, int32_t aEndOffset,
                     nsIDOMRange** aRange)
{
  nsRefPtr<nsRange> range;
  nsresult rv = nsRange::CreateRange(aStartParent, aStartOffset, aEndParent,
                                     aEndOffset, getter_AddRefs(range));
  range.forget(aRange);
  return rv;
}





NS_IMPL_CYCLE_COLLECTING_ADDREF(nsRange)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsRange)

DOMCI_DATA(Range, nsRange)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsRange)
  NS_INTERFACE_MAP_ENTRY(nsIDOMRange)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMRange)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Range)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsRange)
  tmp->Reset();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsRange)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStartParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEndParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRoot)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

static void
RangeHashTableDtor(void* aObject, nsIAtom* aPropertyName, void* aPropertyValue,
                   void* aData)
{
  nsRange::RangeHashTable* ranges =
    static_cast<nsRange::RangeHashTable*>(aPropertyValue);
  delete ranges;
}

static void MarkDescendants(nsINode* aNode)
{
  
  
  
  
  if (!aNode->IsSelectionDescendant()) {
    
    nsINode* node = aNode->GetNextNode(aNode);
    while (node) {
      node->SetDescendantOfCommonAncestorForRangeInSelection();
      if (!node->IsCommonAncestorForRangeInSelection()) {
        node = node->GetNextNode(aNode);
      } else {
        
        node = node->GetNextNonChildNode(aNode);
      }
    }
  }
}

static void UnmarkDescendants(nsINode* aNode)
{
  
  
  
  
  if (!aNode->IsDescendantOfCommonAncestorForRangeInSelection()) {
    
    nsINode* node = aNode->GetNextNode(aNode);
    while (node) {
      node->ClearDescendantOfCommonAncestorForRangeInSelection();
      if (!node->IsCommonAncestorForRangeInSelection()) {
        node = node->GetNextNode(aNode);
      } else {
        
        node = node->GetNextNonChildNode(aNode);
      }
    }
  }
}

void
nsRange::RegisterCommonAncestor(nsINode* aNode)
{
  NS_PRECONDITION(aNode, "bad arg");
  NS_ASSERTION(IsInSelection(), "registering range not in selection");

  MarkDescendants(aNode);

  RangeHashTable* ranges =
    static_cast<RangeHashTable*>(aNode->GetProperty(nsGkAtoms::range));
  if (!ranges) {
    ranges = new RangeHashTable;
    ranges->Init();
    aNode->SetProperty(nsGkAtoms::range, ranges, RangeHashTableDtor, true);
  }
  ranges->PutEntry(this);
  aNode->SetCommonAncestorForRangeInSelection();
}

void
nsRange::UnregisterCommonAncestor(nsINode* aNode)
{
  NS_PRECONDITION(aNode, "bad arg");
  NS_ASSERTION(aNode->IsCommonAncestorForRangeInSelection(), "wrong node");
  RangeHashTable* ranges =
    static_cast<RangeHashTable*>(aNode->GetProperty(nsGkAtoms::range));
  NS_ASSERTION(ranges->GetEntry(this), "unknown range");

  if (ranges->Count() == 1) {
    aNode->ClearCommonAncestorForRangeInSelection();
    aNode->DeleteProperty(nsGkAtoms::range);
    UnmarkDescendants(aNode);
  } else {
    ranges->RemoveEntry(this);
  }
}





void
nsRange::CharacterDataChanged(nsIDocument* aDocument,
                              nsIContent* aContent,
                              CharacterDataChangeInfo* aInfo)
{
  MOZ_ASSERT(mAssertNextInsertOrAppendIndex == -1,
             "splitText failed to notify insert/append?");
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  nsINode* newRoot = nullptr;
  nsINode* newStartNode = nullptr;
  nsINode* newEndNode = nullptr;
  uint32_t newStartOffset = 0;
  uint32_t newEndOffset = 0;

  if (aInfo->mDetails &&
      aInfo->mDetails->mType == CharacterDataChangeInfo::Details::eSplit) {
    
    
    
    
    
    
    nsINode* parentNode = aContent->GetParentNode();
    int32_t index = -1;
    if (parentNode == mEndParent && mEndOffset > 0 &&
        (index = parentNode->IndexOf(aContent)) + 1 == mEndOffset) {
      ++mEndOffset;
      mEndOffsetWasIncremented = true;
    }
    if (parentNode == mStartParent && mStartOffset > 0 &&
        (index != -1 ? index : parentNode->IndexOf(aContent)) + 1 == mStartOffset) {
      ++mStartOffset;
      mStartOffsetWasIncremented = true;
    }
#ifdef DEBUG
    if (mStartOffsetWasIncremented || mEndOffsetWasIncremented) {
      mAssertNextInsertOrAppendIndex =
        (mStartOffsetWasIncremented ? mStartOffset : mEndOffset) - 1;
      mAssertNextInsertOrAppendNode = aInfo->mDetails->mNextSibling;
    }
#endif
  }

  
  
  if (aContent == mStartParent &&
      aInfo->mChangeStart < static_cast<uint32_t>(mStartOffset)) {
    if (aInfo->mDetails) {
      
      NS_ASSERTION(aInfo->mDetails->mType ==
                   CharacterDataChangeInfo::Details::eSplit,
                   "only a split can start before the end");
      NS_ASSERTION(static_cast<uint32_t>(mStartOffset) <= aInfo->mChangeEnd + 1,
                   "mStartOffset is beyond the end of this node");
      newStartOffset = static_cast<uint32_t>(mStartOffset) - aInfo->mChangeStart;
      newStartNode = aInfo->mDetails->mNextSibling;
      if (MOZ_UNLIKELY(aContent == mRoot)) {
        newRoot = IsValidBoundary(newStartNode);
      }

      bool isCommonAncestor = IsInSelection() && mStartParent == mEndParent;
      if (isCommonAncestor) {
        UnregisterCommonAncestor(mStartParent);
        RegisterCommonAncestor(newStartNode);
      }
      if (mStartParent->IsDescendantOfCommonAncestorForRangeInSelection()) {
        newStartNode->SetDescendantOfCommonAncestorForRangeInSelection();
      }
    } else {
      
      
      mStartOffset = static_cast<uint32_t>(mStartOffset) <= aInfo->mChangeEnd ?
        aInfo->mChangeStart :
        mStartOffset + aInfo->mChangeStart - aInfo->mChangeEnd +
          aInfo->mReplaceLength;
    }
  }

  
  
  
  if (aContent == mEndParent &&
      aInfo->mChangeStart < static_cast<uint32_t>(mEndOffset)) {
    if (aInfo->mDetails && (aContent->GetParentNode() || newStartNode)) {
      
      NS_ASSERTION(aInfo->mDetails->mType ==
                   CharacterDataChangeInfo::Details::eSplit,
                   "only a split can start before the end");
      NS_ASSERTION(static_cast<uint32_t>(mEndOffset) <= aInfo->mChangeEnd + 1,
                   "mEndOffset is beyond the end of this node");
      newEndOffset = static_cast<uint32_t>(mEndOffset) - aInfo->mChangeStart;
      newEndNode = aInfo->mDetails->mNextSibling;

      bool isCommonAncestor = IsInSelection() && mStartParent == mEndParent;
      if (isCommonAncestor && !newStartNode) {
        
        UnregisterCommonAncestor(mStartParent);
        RegisterCommonAncestor(mStartParent->GetParentNode());
        newEndNode->SetDescendantOfCommonAncestorForRangeInSelection();
      } else if (mEndParent->IsDescendantOfCommonAncestorForRangeInSelection()) {
        newEndNode->SetDescendantOfCommonAncestorForRangeInSelection();
      }
    } else {
      mEndOffset = static_cast<uint32_t>(mEndOffset) <= aInfo->mChangeEnd ?
        aInfo->mChangeStart :
        mEndOffset + aInfo->mChangeStart - aInfo->mChangeEnd +
          aInfo->mReplaceLength;
    }
  }

  if (aInfo->mDetails &&
      aInfo->mDetails->mType == CharacterDataChangeInfo::Details::eMerge) {
    
    
    nsIContent* removed = aInfo->mDetails->mNextSibling;
    if (removed == mStartParent) {
      newStartOffset = static_cast<uint32_t>(mStartOffset) + aInfo->mChangeStart;
      newStartNode = aContent;
      if (MOZ_UNLIKELY(removed == mRoot)) {
        newRoot = IsValidBoundary(newStartNode);
      }
    }
    if (removed == mEndParent) {
      newEndOffset = static_cast<uint32_t>(mEndOffset) + aInfo->mChangeStart;
      newEndNode = aContent;
      if (MOZ_UNLIKELY(removed == mRoot)) {
        newRoot = IsValidBoundary(newEndNode);
      }
    }
    
    
    
    
    
    
    nsINode* parentNode = aContent->GetParentNode();
    if (parentNode == mStartParent && mStartOffset > 0 &&
        mStartOffset < parentNode->GetChildCount() &&
        removed == parentNode->GetChildAt(mStartOffset)) {
      newStartNode = aContent;
      newStartOffset = aInfo->mChangeStart;
    }
    if (parentNode == mEndParent && mEndOffset > 0 &&
        mEndOffset < parentNode->GetChildCount() &&
        removed == parentNode->GetChildAt(mEndOffset)) {
      newEndNode = aContent;
      newEndOffset = aInfo->mChangeEnd;
    }
  }

  if (newStartNode || newEndNode) {
    if (!newStartNode) {
      newStartNode = mStartParent;
      newStartOffset = mStartOffset;
    }
    if (!newEndNode) {
      newEndNode = mEndParent;
      newEndOffset = mEndOffset;
    }
    DoSetRange(newStartNode, newStartOffset, newEndNode, newEndOffset,
               newRoot ? newRoot : mRoot.get(),
               !newEndNode->GetParentNode() || !newStartNode->GetParentNode());
  }
}

void
nsRange::ContentAppended(nsIDocument* aDocument,
                         nsIContent*  aContainer,
                         nsIContent*  aFirstNewContent,
                         int32_t      aNewIndexInContainer)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  nsINode* container = NODE_FROM(aContainer, aDocument);
  if (container->IsSelectionDescendant() && IsInSelection()) {
    nsINode* child = aFirstNewContent;
    while (child) {
      if (!child->IsDescendantOfCommonAncestorForRangeInSelection()) {
        MarkDescendants(child);
        child->SetDescendantOfCommonAncestorForRangeInSelection();
      }
      child = child->GetNextSibling();
    }
  }

  if (mStartOffsetWasIncremented || mEndOffsetWasIncremented) {
    MOZ_ASSERT(mAssertNextInsertOrAppendIndex == aNewIndexInContainer);
    MOZ_ASSERT(mAssertNextInsertOrAppendNode == aFirstNewContent);
    MOZ_ASSERT(aFirstNewContent->IsNodeOfType(nsINode::eDATA_NODE));
    mStartOffsetWasIncremented = mEndOffsetWasIncremented = false;
#ifdef DEBUG
    mAssertNextInsertOrAppendIndex = -1;
    mAssertNextInsertOrAppendNode = nullptr;
#endif
  }
}

void
nsRange::ContentInserted(nsIDocument* aDocument,
                         nsIContent* aContainer,
                         nsIContent* aChild,
                         int32_t aIndexInContainer)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  nsINode* container = NODE_FROM(aContainer, aDocument);

  
  if (container == mStartParent && aIndexInContainer < mStartOffset &&
      !mStartOffsetWasIncremented) {
    ++mStartOffset;
  }
  if (container == mEndParent && aIndexInContainer < mEndOffset &&
      !mEndOffsetWasIncremented) {
    ++mEndOffset;
  }
  if (container->IsSelectionDescendant() &&
      !aChild->IsDescendantOfCommonAncestorForRangeInSelection()) {
    MarkDescendants(aChild);
    aChild->SetDescendantOfCommonAncestorForRangeInSelection();
  }

  if (mStartOffsetWasIncremented || mEndOffsetWasIncremented) {
    MOZ_ASSERT(mAssertNextInsertOrAppendIndex == aIndexInContainer);
    MOZ_ASSERT(mAssertNextInsertOrAppendNode == aChild);
    MOZ_ASSERT(aChild->IsNodeOfType(nsINode::eDATA_NODE));
    mStartOffsetWasIncremented = mEndOffsetWasIncremented = false;
#ifdef DEBUG
    mAssertNextInsertOrAppendIndex = -1;
    mAssertNextInsertOrAppendNode = nullptr;
#endif
  }
}

void
nsRange::ContentRemoved(nsIDocument* aDocument,
                        nsIContent* aContainer,
                        nsIContent* aChild,
                        int32_t aIndexInContainer,
                        nsIContent* aPreviousSibling)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");
  MOZ_ASSERT(!mStartOffsetWasIncremented && !mEndOffsetWasIncremented &&
             mAssertNextInsertOrAppendIndex == -1,
             "splitText failed to notify insert/append?");

  nsINode* container = NODE_FROM(aContainer, aDocument);
  bool gravitateStart = false;
  bool gravitateEnd = false;

  
  if (container == mStartParent) {
    if (aIndexInContainer < mStartOffset) {
      --mStartOffset;
    }
  }
  
  else if (nsContentUtils::ContentIsDescendantOf(mStartParent, aChild)) {
    gravitateStart = true;
  }

  
  if (container == mEndParent) {
    if (aIndexInContainer < mEndOffset) {
      --mEndOffset;
    }
  }
  else if (nsContentUtils::ContentIsDescendantOf(mEndParent, aChild)) {
    gravitateEnd = true;
  }

  if (gravitateStart || gravitateEnd) {
    DoSetRange(gravitateStart ? container : mStartParent.get(),
               gravitateStart ? aIndexInContainer : mStartOffset,
               gravitateEnd ? container : mEndParent.get(),
               gravitateEnd ? aIndexInContainer : mEndOffset,
               mRoot);
  }
  if (container->IsSelectionDescendant() &&
      aChild->IsDescendantOfCommonAncestorForRangeInSelection()) {
    aChild->ClearDescendantOfCommonAncestorForRangeInSelection();
    UnmarkDescendants(aChild);
  }
}

void
nsRange::ParentChainChanged(nsIContent *aContent)
{
  MOZ_ASSERT(!mStartOffsetWasIncremented && !mEndOffsetWasIncremented &&
             mAssertNextInsertOrAppendIndex == -1,
             "splitText failed to notify insert/append?");
  NS_ASSERTION(mRoot == aContent, "Wrong ParentChainChanged notification?");
  nsINode* newRoot = IsValidBoundary(mStartParent);
  NS_ASSERTION(newRoot, "No valid boundary or root found!");
  if (newRoot != IsValidBoundary(mEndParent)) {
    
    
    
    
    NS_ASSERTION(mEndParent->IsInNativeAnonymousSubtree(),
                 "This special case should happen only with "
                 "native-anonymous content");
    
    
    Reset();
    return;
  }
  
  
  DoSetRange(mStartParent, mStartOffset, mEndParent, mEndOffset, newRoot);
}




NS_IMETHODIMP
nsRange::IsPointInRange(nsIDOMNode* aParent, int32_t aOffset, bool* aResult)
{
  int16_t compareResult = 0;
  nsresult rv = ComparePoint(aParent, aOffset, &compareResult);
  
  if (rv == NS_ERROR_DOM_WRONG_DOCUMENT_ERR) {
    *aResult = false;
    return NS_OK;
  }

  *aResult = compareResult == 0;

  return rv;
}
  


NS_IMETHODIMP
nsRange::ComparePoint(nsIDOMNode* aParent, int32_t aOffset, int16_t* aResult)
{
  
  if (!mIsPositioned) 
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  NS_ENSURE_TRUE(parent, NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);

  if (!nsContentUtils::ContentIsDescendantOf(parent, mRoot)) {
    return NS_ERROR_DOM_WRONG_DOCUMENT_ERR;
  }
  
  if (parent->NodeType() == nsIDOMNode::DOCUMENT_TYPE_NODE) {
    return NS_ERROR_DOM_INVALID_NODE_TYPE_ERR;
  }

  if (aOffset < 0 || uint32_t(aOffset) > parent->Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  
  int32_t cmp;
  if ((cmp = nsContentUtils::ComparePoints(parent, aOffset,
                                           mStartParent, mStartOffset)) <= 0) {
    
    *aResult = cmp;
  }
  else if (nsContentUtils::ComparePoints(mEndParent, mEndOffset,
                                         parent, aOffset) == -1) {
    *aResult = 1;
  }
  else {
    *aResult = 0;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsRange::IntersectsNode(nsIDOMNode* aNode, bool* aResult)
{
  *aResult = false;

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  
  NS_ENSURE_ARG(node);

  NS_ENSURE_TRUE(mIsPositioned, NS_ERROR_NOT_INITIALIZED);

  
  nsINode* parent = node->GetParentNode();
  if (!parent) {
    
    
    *aResult = (GetRoot() == node);
    return NS_OK;
  }

  
  int32_t nodeIndex = parent->IndexOf(node);

  
  
  bool disconnected = false;
  *aResult = nsContentUtils::ComparePoints(mStartParent, mStartOffset,
                                           parent, nodeIndex + 1,
                                           &disconnected) < 0 &&
             nsContentUtils::ComparePoints(parent, nodeIndex,
                                           mEndParent, mEndOffset,
                                           &disconnected) < 0;

  
  if (disconnected) {
    *aResult = false;
  }
  return NS_OK;
}










void
nsRange::DoSetRange(nsINode* aStartN, int32_t aStartOffset,
                    nsINode* aEndN, int32_t aEndOffset,
                    nsINode* aRoot, bool aNotInsertedYet)
{
  NS_PRECONDITION((aStartN && aEndN && aRoot) ||
                  (!aStartN && !aEndN && !aRoot),
                  "Set all or none");
  NS_PRECONDITION(!aRoot || aNotInsertedYet ||
                  (nsContentUtils::ContentIsDescendantOf(aStartN, aRoot) &&
                   nsContentUtils::ContentIsDescendantOf(aEndN, aRoot) &&
                   aRoot == IsValidBoundary(aStartN) &&
                   aRoot == IsValidBoundary(aEndN)),
                  "Wrong root");
  NS_PRECONDITION(!aRoot ||
                  (aStartN->IsNodeOfType(nsINode::eCONTENT) &&
                   aEndN->IsNodeOfType(nsINode::eCONTENT) &&
                   aRoot ==
                    static_cast<nsIContent*>(aStartN)->GetBindingParent() &&
                   aRoot ==
                    static_cast<nsIContent*>(aEndN)->GetBindingParent()) ||
                  (!aRoot->GetParentNode() &&
                   (aRoot->IsNodeOfType(nsINode::eDOCUMENT) ||
                    aRoot->IsNodeOfType(nsINode::eATTRIBUTE) ||
                    aRoot->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT) ||
                     
                    aRoot->IsNodeOfType(nsINode::eCONTENT))),
                  "Bad root");

  if (mRoot != aRoot) {
    if (mRoot) {
      mRoot->RemoveMutationObserver(this);
    }
    if (aRoot) {
      aRoot->AddMutationObserver(this);
    }
  }
  bool checkCommonAncestor = (mStartParent != aStartN || mEndParent != aEndN) &&
                             IsInSelection() && !aNotInsertedYet;
  nsINode* oldCommonAncestor = checkCommonAncestor ? GetCommonAncestor() : nullptr;
  mStartParent = aStartN;
  mStartOffset = aStartOffset;
  mEndParent = aEndN;
  mEndOffset = aEndOffset;
  mIsPositioned = !!mStartParent;
  if (checkCommonAncestor) {
    nsINode* newCommonAncestor = GetCommonAncestor();
    if (newCommonAncestor != oldCommonAncestor) {
      if (oldCommonAncestor) {
        UnregisterCommonAncestor(oldCommonAncestor);
      }
      if (newCommonAncestor) {
        RegisterCommonAncestor(newCommonAncestor);
      } else {
        NS_ASSERTION(!mIsPositioned, "unexpected disconnected nodes");
        mInSelection = false;
      }
    }
  }

  
  
  mRoot = aRoot;
}

static int32_t
IndexOf(nsIDOMNode* aChildNode)
{
  

  nsCOMPtr<nsINode> child = do_QueryInterface(aChildNode);
  if (!child) {
    return -1;
  }

  nsINode *parent = child->GetParentNode();

  
  return parent ? parent->IndexOf(child) : -1;
}

nsINode*
nsRange::GetCommonAncestor() const
{
  return mIsPositioned ?
    nsContentUtils::GetCommonAncestor(mStartParent, mEndParent) :
    nullptr;
}

void
nsRange::Reset()
{
  DoSetRange(nullptr, 0, nullptr, 0, nullptr);
}





NS_IMETHODIMP
nsRange::GetStartContainer(nsIDOMNode** aStartParent)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  return CallQueryInterface(mStartParent, aStartParent);
}

NS_IMETHODIMP
nsRange::GetStartOffset(int32_t* aStartOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aStartOffset = mStartOffset;

  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetEndContainer(nsIDOMNode** aEndParent)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  return CallQueryInterface(mEndParent, aEndParent);
}

NS_IMETHODIMP
nsRange::GetEndOffset(int32_t* aEndOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aEndOffset = mEndOffset;

  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetCollapsed(bool* aIsCollapsed)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aIsCollapsed = Collapsed();

  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetCommonAncestorContainer(nsIDOMNode** aCommonParent)
{
  *aCommonParent = nullptr;
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  nsINode* container = nsContentUtils::GetCommonAncestor(mStartParent, mEndParent);
  if (container) {
    return CallQueryInterface(container, aCommonParent);
  }

  return NS_ERROR_NOT_INITIALIZED;
}

nsINode*
nsRange::IsValidBoundary(nsINode* aNode)
{
  if (!aNode) {
    return nullptr;
  }

  if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
    nsIContent* content = static_cast<nsIContent*>(aNode);
    if (content->Tag() == nsGkAtoms::documentTypeNodeName) {
      return nullptr;
    }

    if (!mMaySpanAnonymousSubtrees) {
      
      
      nsINode* root = content->GetBindingParent();
      if (root) {
        return root;
      }
    }
  }

  
  
  nsINode* root = aNode->GetCurrentDoc();
  if (root) {
    return root;
  }

  root = aNode;
  while ((aNode = aNode->GetParentNode())) {
    root = aNode;
  }

  NS_ASSERTION(!root->IsNodeOfType(nsINode::eDOCUMENT),
               "GetCurrentDoc should have returned a doc");

#ifdef DEBUG_smaug
  NS_WARN_IF_FALSE(root->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT) ||
                   root->IsNodeOfType(nsINode::eATTRIBUTE),
                   "Creating a DOM Range using root which isn't in DOM!");
#endif

  
  return root;
}

NS_IMETHODIMP
nsRange::SetStart(nsIDOMNode* aParent, int32_t aOffset)
{
  VALIDATE_ACCESS(aParent);

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  AutoInvalidateSelection atEndOfBlock(this);
  return SetStart(parent, aOffset);
}

 nsresult
nsRange::SetStart(nsINode* aParent, int32_t aOffset)
{
  nsINode* newRoot = IsValidBoundary(aParent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);

  if (aOffset < 0 || uint32_t(aOffset) > aParent->Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  
  if (!mIsPositioned || newRoot != mRoot ||
      nsContentUtils::ComparePoints(aParent, aOffset,
                                    mEndParent, mEndOffset) == 1) {
    DoSetRange(aParent, aOffset, aParent, aOffset, newRoot);

    return NS_OK;
  }

  DoSetRange(aParent, aOffset, mEndParent, mEndOffset, mRoot);
  
  return NS_OK;
}

NS_IMETHODIMP
nsRange::SetStartBefore(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> parent;
  nsresult rv = aSibling->GetParentNode(getter_AddRefs(parent));
  if (NS_FAILED(rv) || !parent) {
    return NS_ERROR_DOM_INVALID_NODE_TYPE_ERR;
  }

  return SetStart(parent, IndexOf(aSibling));
}

NS_IMETHODIMP
nsRange::SetStartAfter(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);

  nsCOMPtr<nsIDOMNode> nParent;
  nsresult res = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(res) || !nParent) {
    return NS_ERROR_DOM_INVALID_NODE_TYPE_ERR;
  }

  return SetStart(nParent, IndexOf(aSibling) + 1);
}

NS_IMETHODIMP
nsRange::SetEnd(nsIDOMNode* aParent, int32_t aOffset)
{
  VALIDATE_ACCESS(aParent);

  AutoInvalidateSelection atEndOfBlock(this);
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  return SetEnd(parent, aOffset);
}


 nsresult
nsRange::SetEnd(nsINode* aParent, int32_t aOffset)
{
  nsINode* newRoot = IsValidBoundary(aParent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);

  if (aOffset < 0 || uint32_t(aOffset) > aParent->Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  
  if (!mIsPositioned || newRoot != mRoot ||
      nsContentUtils::ComparePoints(mStartParent, mStartOffset,
                                    aParent, aOffset) == 1) {
    DoSetRange(aParent, aOffset, aParent, aOffset, newRoot);

    return NS_OK;
  }

  DoSetRange(mStartParent, mStartOffset, aParent, aOffset, mRoot);

  return NS_OK;
}

NS_IMETHODIMP
nsRange::SetEndBefore(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> nParent;
  nsresult rv = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(rv) || !nParent) {
    return NS_ERROR_DOM_INVALID_NODE_TYPE_ERR;
  }

  return SetEnd(nParent, IndexOf(aSibling));
}

NS_IMETHODIMP
nsRange::SetEndAfter(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> nParent;
  nsresult res = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(res) || !nParent) {
    return NS_ERROR_DOM_INVALID_NODE_TYPE_ERR;
  }

  return SetEnd(nParent, IndexOf(aSibling) + 1);
}

NS_IMETHODIMP
nsRange::Collapse(bool aToStart)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  AutoInvalidateSelection atEndOfBlock(this);
  if (aToStart)
    DoSetRange(mStartParent, mStartOffset, mStartParent, mStartOffset, mRoot);
  else
    DoSetRange(mEndParent, mEndOffset, mEndParent, mEndOffset, mRoot);

  return NS_OK;
}

NS_IMETHODIMP
nsRange::SelectNode(nsIDOMNode* aN)
{
  VALIDATE_ACCESS(aN);
  
  nsCOMPtr<nsINode> node = do_QueryInterface(aN);
  NS_ENSURE_TRUE(node, NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);

  nsINode* parent = node->GetParentNode();
  nsINode* newRoot = IsValidBoundary(parent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);

  int32_t index = parent->IndexOf(node);
  if (index < 0) {
    return NS_ERROR_DOM_INVALID_NODE_TYPE_ERR;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  DoSetRange(parent, index, parent, index + 1, newRoot);
  
  return NS_OK;
}

NS_IMETHODIMP
nsRange::SelectNodeContents(nsIDOMNode* aN)
{
  VALIDATE_ACCESS(aN);

  nsCOMPtr<nsINode> node = do_QueryInterface(aN);
  nsINode* newRoot = IsValidBoundary(node);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);
  
  AutoInvalidateSelection atEndOfBlock(this);
  DoSetRange(node, 0, node, node->Length(), newRoot);
  
  return NS_OK;
}














class NS_STACK_CLASS RangeSubtreeIterator
{
private:

  enum RangeSubtreeIterState { eDone=0,
                               eUseStart,
                               eUseIterator,
                               eUseEnd };

  nsCOMPtr<nsIContentIterator>  mIter;
  RangeSubtreeIterState         mIterState;

  nsCOMPtr<nsIDOMNode> mStart;
  nsCOMPtr<nsIDOMNode> mEnd;

public:

  RangeSubtreeIterator()
    : mIterState(eDone)
  {
  }
  ~RangeSubtreeIterator()
  {
  }

  nsresult Init(nsIDOMRange *aRange);
  already_AddRefed<nsIDOMNode> GetCurrentNode();
  void First();
  void Last();
  void Next();
  void Prev();

  bool IsDone()
  {
    return mIterState == eDone;
  }
};

nsresult
RangeSubtreeIterator::Init(nsIDOMRange *aRange)
{
  mIterState = eDone;
  bool collapsed;
  aRange->GetCollapsed(&collapsed);
  if (collapsed) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> node;

  
  
  

  nsresult res = aRange->GetStartContainer(getter_AddRefs(node));
  if (!node) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCharacterData> startData = do_QueryInterface(node);
  if (startData) {
    mStart = node;
  } else {
    int32_t startIndex;
    aRange->GetStartOffset(&startIndex);
    nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
    if (iNode->IsElement() && 
        int32_t(iNode->AsElement()->GetChildCount()) == startIndex) {
      mStart = node;
    }
  }

  
  
  

  res = aRange->GetEndContainer(getter_AddRefs(node));
  if (!node) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCharacterData> endData = do_QueryInterface(node);
  if (endData) {
    mEnd = node;
  } else {
    int32_t endIndex;
    aRange->GetEndOffset(&endIndex);
    nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
    if (iNode->IsElement() && endIndex == 0) {
      mEnd = node;
    }
  }

  if (mStart && mStart == mEnd)
  {
    
    
    

    mEnd = nullptr;
  }
  else
  {
    
    

    mIter = NS_NewContentSubtreeIterator();

    res = mIter->Init(aRange);
    if (NS_FAILED(res)) return res;

    if (mIter->IsDone())
    {
      
      
      

      mIter = nullptr;
    }
  }

  
  

  First();

  return NS_OK;
}

already_AddRefed<nsIDOMNode>
RangeSubtreeIterator::GetCurrentNode()
{
  nsIDOMNode *node = nullptr;

  if (mIterState == eUseStart && mStart) {
    NS_ADDREF(node = mStart);
  } else if (mIterState == eUseEnd && mEnd)
    NS_ADDREF(node = mEnd);
  else if (mIterState == eUseIterator && mIter)
  {
    nsINode* n = mIter->GetCurrentNode();

    if (n) {
      CallQueryInterface(n, &node);
    }
  }

  return node;
}

void
RangeSubtreeIterator::First()
{
  if (mStart)
    mIterState = eUseStart;
  else if (mIter)
  {
    mIter->First();

    mIterState = eUseIterator;
  }
  else if (mEnd)
    mIterState = eUseEnd;
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Last()
{
  if (mEnd)
    mIterState = eUseEnd;
  else if (mIter)
  {
    mIter->Last();

    mIterState = eUseIterator;
  }
  else if (mStart)
    mIterState = eUseStart;
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Next()
{
  if (mIterState == eUseStart)
  {
    if (mIter)
    {
      mIter->First();

      mIterState = eUseIterator;
    }
    else if (mEnd)
      mIterState = eUseEnd;
    else
      mIterState = eDone;
  }
  else if (mIterState == eUseIterator)
  {
    mIter->Next();

    if (mIter->IsDone())
    {
      if (mEnd)
        mIterState = eUseEnd;
      else
        mIterState = eDone;
    }
  }
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Prev()
{
  if (mIterState == eUseEnd)
  {
    if (mIter)
    {
      mIter->Last();

      mIterState = eUseIterator;
    }
    else if (mStart)
      mIterState = eUseStart;
    else
      mIterState = eDone;
  }
  else if (mIterState == eUseIterator)
  {
    mIter->Prev();

    if (mIter->IsDone())
    {
      if (mStart)
        mIterState = eUseStart;
      else
        mIterState = eDone;
    }
  }
  else
    mIterState = eDone;
}












static nsresult
CollapseRangeAfterDelete(nsIDOMRange *aRange)
{
  NS_ENSURE_ARG_POINTER(aRange);

  

  bool isCollapsed = false;
  nsresult res = aRange->GetCollapsed(&isCollapsed);
  if (NS_FAILED(res)) return res;

  if (isCollapsed)
  {
    
    
    
    
    
    
    
    
    
    
    

    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMNode> commonAncestor;
  res = aRange->GetCommonAncestorContainer(getter_AddRefs(commonAncestor));
  if(NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMNode> startContainer, endContainer;

  res = aRange->GetStartContainer(getter_AddRefs(startContainer));
  if (NS_FAILED(res)) return res;

  res = aRange->GetEndContainer(getter_AddRefs(endContainer));
  if (NS_FAILED(res)) return res;

  
  
  
  

  if (startContainer == commonAncestor)
    return aRange->Collapse(true);
  if (endContainer == commonAncestor)
    return aRange->Collapse(false);

  
  
  

  nsCOMPtr<nsIDOMNode> nodeToSelect(startContainer), parent;

  while (nodeToSelect)
  {
    nsresult res = nodeToSelect->GetParentNode(getter_AddRefs(parent));
    if (NS_FAILED(res)) return res;

    if (parent == commonAncestor)
      break; 

    nodeToSelect = parent;
  }

  if (!nodeToSelect)
    return NS_ERROR_FAILURE; 

  res = aRange->SelectNode(nodeToSelect);
  if (NS_FAILED(res)) return res;

  return aRange->Collapse(false);
}










static nsresult SplitDataNode(nsIDOMCharacterData* aStartNode,
                              uint32_t aStartIndex,
                              nsIDOMCharacterData** aEndNode,
                              bool aCloneAfterOriginal = true)
{
  nsresult rv;
  nsCOMPtr<nsINode> node = do_QueryInterface(aStartNode);
  NS_ENSURE_STATE(node && node->IsNodeOfType(nsINode::eDATA_NODE));
  nsGenericDOMDataNode* dataNode = static_cast<nsGenericDOMDataNode*>(node.get());

  nsCOMPtr<nsIContent> newData;
  rv = dataNode->SplitData(aStartIndex, getter_AddRefs(newData),
                           aCloneAfterOriginal);
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(newData, aEndNode);
}

NS_IMETHODIMP
PrependChild(nsIDOMNode* aParent, nsIDOMNode* aChild)
{
  nsCOMPtr<nsIDOMNode> first, tmpNode;
  aParent->GetFirstChild(getter_AddRefs(first));
  return aParent->InsertBefore(aChild, first, getter_AddRefs(tmpNode));
}



static bool
ValidateCurrentNode(nsRange* aRange, RangeSubtreeIterator& aIter)
{
  bool before, after;
  nsCOMPtr<nsIDOMNode> domNode = aIter.GetCurrentNode();
  if (!domNode) {
    
    
    return true;
  }
  nsCOMPtr<nsINode> node = do_QueryInterface(domNode);
  MOZ_ASSERT(node);

  nsresult res = nsRange::CompareNodeToRange(node, aRange, &before, &after);

  return NS_SUCCEEDED(res) && !before && !after;
}

nsresult nsRange::CutContents(nsIDOMDocumentFragment** aFragment)
{ 
  if (aFragment) {
    *aFragment = nullptr;
  }

  nsresult rv;

  nsCOMPtr<nsIDocument> doc = mStartParent->OwnerDoc();

  nsCOMPtr<nsIDOMNode> commonAncestor;
  rv = GetCommonAncestorContainer(getter_AddRefs(commonAncestor));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocumentFragment> retval;
  if (aFragment) {
    rv = NS_NewDocumentFragment(getter_AddRefs(retval),
                                doc->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsCOMPtr<nsIDOMNode> commonCloneAncestor(do_QueryInterface(retval));

  
  mozAutoSubtreeModified subtree(mRoot ? mRoot->OwnerDoc(): nullptr, nullptr);

  
  

  nsCOMPtr<nsIDOMNode> startContainer = do_QueryInterface(mStartParent);
  int32_t              startOffset = mStartOffset;
  nsCOMPtr<nsIDOMNode> endContainer = do_QueryInterface(mEndParent);
  int32_t              endOffset = mEndOffset;

  if (retval) {
    
    
    
    nsCOMPtr<nsIDOMDocument> commonAncestorDocument(do_QueryInterface(commonAncestor));
    if (commonAncestorDocument) {
      nsCOMPtr<nsIDOMDocumentType> doctype;
      rv = commonAncestorDocument->GetDoctype(getter_AddRefs(doctype));
      NS_ENSURE_SUCCESS(rv, rv);

      if (doctype &&
          nsContentUtils::ComparePoints(startContainer, startOffset,
                                        doctype.get(), 0) < 0 &&
          nsContentUtils::ComparePoints(doctype.get(), 0,
                                        endContainer, endOffset) < 0) {
        return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
      }
    }
  }

  
  

  RangeSubtreeIterator iter;

  rv = iter.Init(this);
  if (NS_FAILED(rv)) return rv;

  if (iter.IsDone())
  {
    
    rv = CollapseRangeAfterDelete(this);
    if (NS_SUCCEEDED(rv) && aFragment) {
      NS_ADDREF(*aFragment = retval);
    }
    return rv;
  }

  

  iter.Last();

  bool handled = false;

  
  
  

  while (!iter.IsDone())
  {
    nsCOMPtr<nsIDOMNode> nodeToResult;
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());

    
    

    iter.Prev();

    handled = false;

    
    
    
    
    

    nsCOMPtr<nsIDOMCharacterData> charData(do_QueryInterface(node));

    if (charData)
    {
      uint32_t dataLength = 0;

      if (node == startContainer)
      {
        if (node == endContainer)
        {
          
          

          if (endOffset > startOffset)
          {
            if (retval) {
              nsAutoString cutValue;
              rv = charData->SubstringData(startOffset, endOffset - startOffset,
                                           cutValue);
              NS_ENSURE_SUCCESS(rv, rv);
              nsCOMPtr<nsIDOMNode> clone;
              rv = charData->CloneNode(false, 1, getter_AddRefs(clone));
              NS_ENSURE_SUCCESS(rv, rv);
              clone->SetNodeValue(cutValue);
              nodeToResult = clone;
            }

            nsMutationGuard guard;
            rv = charData->DeleteData(startOffset, endOffset - startOffset);
            NS_ENSURE_SUCCESS(rv, rv);
            NS_ENSURE_STATE(!guard.Mutated(0) ||
                            ValidateCurrentNode(this, iter));
          }

          handled = true;
        }
        else
        {
          

          rv = charData->GetLength(&dataLength);
          NS_ENSURE_SUCCESS(rv, rv);

          if (dataLength >= (uint32_t)startOffset)
          {
            nsMutationGuard guard;
            nsCOMPtr<nsIDOMCharacterData> cutNode;
            rv = SplitDataNode(charData, startOffset, getter_AddRefs(cutNode));
            NS_ENSURE_SUCCESS(rv, rv);
            NS_ENSURE_STATE(!guard.Mutated(1) ||
                            ValidateCurrentNode(this, iter));
            nodeToResult = cutNode;
          }

          handled = true;
        }
      }
      else if (node == endContainer)
      {
        

        if (endOffset >= 0)
        {
          nsMutationGuard guard;
          nsCOMPtr<nsIDOMCharacterData> cutNode;
          


          rv = SplitDataNode(charData, endOffset, getter_AddRefs(cutNode),
                             false);
          NS_ENSURE_SUCCESS(rv, rv);
          NS_ENSURE_STATE(!guard.Mutated(1) ||
                          ValidateCurrentNode(this, iter));
          nodeToResult = cutNode;
        }

        handled = true;
      }       
    }

    if (!handled && (node == endContainer || node == startContainer))
    {
      nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
      if (iNode && iNode->IsElement() &&
          ((node == endContainer && endOffset == 0) ||
           (node == startContainer &&
            int32_t(iNode->AsElement()->GetChildCount()) == startOffset)))
      {
        if (retval) {
          nsCOMPtr<nsIDOMNode> clone;
          rv = node->CloneNode(false, 1, getter_AddRefs(clone));
          NS_ENSURE_SUCCESS(rv, rv);
          nodeToResult = clone;
        }
        handled = true;
      }
    }

    if (!handled)
    {
      
      
      nodeToResult = node;
    }

    uint32_t parentCount = 0;
    nsCOMPtr<nsIDOMNode> tmpNode;
    
    if (retval) {
      nsCOMPtr<nsIDOMNode> oldCommonAncestor = commonAncestor;
      if (!iter.IsDone()) {
        
        nsCOMPtr<nsIDOMNode> prevNode(iter.GetCurrentNode());
        NS_ENSURE_STATE(prevNode);

        
        
        nsContentUtils::GetCommonAncestor(node, prevNode,
                                          getter_AddRefs(commonAncestor));
        NS_ENSURE_STATE(commonAncestor);

        nsCOMPtr<nsIDOMNode> parentCounterNode = node;
        while (parentCounterNode && parentCounterNode != commonAncestor)
        {
          ++parentCount;
          tmpNode = parentCounterNode;
          tmpNode->GetParentNode(getter_AddRefs(parentCounterNode));
          NS_ENSURE_STATE(parentCounterNode);
        }
      }

      
      nsCOMPtr<nsIDOMNode> closestAncestor, farthestAncestor;
      rv = CloneParentsBetween(oldCommonAncestor, node,
                               getter_AddRefs(closestAncestor),
                               getter_AddRefs(farthestAncestor));
      NS_ENSURE_SUCCESS(rv, rv);

      if (farthestAncestor)
      {
        rv = PrependChild(commonCloneAncestor, farthestAncestor);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsMutationGuard guard;
      nsCOMPtr<nsIDOMNode> parent;
      nodeToResult->GetParentNode(getter_AddRefs(parent));
      rv = closestAncestor ? PrependChild(closestAncestor, nodeToResult)
                           : PrependChild(commonCloneAncestor, nodeToResult);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(!guard.Mutated(parent ? 2 : 1) ||
                      ValidateCurrentNode(this, iter));
    } else if (nodeToResult) {
      nsMutationGuard guard;
      nsCOMPtr<nsINode> node = do_QueryInterface(nodeToResult);
      nsINode* parent = node->GetParentNode();
      if (parent) {
        mozilla::ErrorResult error;
        parent->RemoveChild(*node, error);
        NS_ENSURE_FALSE(error.Failed(), error.ErrorCode());
      }
      NS_ENSURE_STATE(!guard.Mutated(1) ||
                      ValidateCurrentNode(this, iter));
    }

    if (!iter.IsDone() && retval) {
      
      nsCOMPtr<nsIDOMNode> newCloneAncestor = nodeToResult;
      for (uint32_t i = parentCount; i; --i)
      {
        tmpNode = newCloneAncestor;
        tmpNode->GetParentNode(getter_AddRefs(newCloneAncestor));
        NS_ENSURE_STATE(newCloneAncestor);
      }
      commonCloneAncestor = newCloneAncestor;
    }
  }

  rv = CollapseRangeAfterDelete(this);
  if (NS_SUCCEEDED(rv) && aFragment) {
    NS_ADDREF(*aFragment = retval);
  }
  return rv;
}

NS_IMETHODIMP
nsRange::DeleteContents()
{
  return CutContents(nullptr);
}

NS_IMETHODIMP
nsRange::ExtractContents(nsIDOMDocumentFragment** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  return CutContents(aReturn);
}

NS_IMETHODIMP
nsRange::CompareBoundaryPoints(uint16_t aHow, nsIDOMRange* aOtherRange,
                               int16_t* aCmpRet)
{
  nsRange* otherRange = static_cast<nsRange*>(aOtherRange);
  NS_ENSURE_TRUE(otherRange, NS_ERROR_NULL_POINTER);

  if (!mIsPositioned || !otherRange->IsPositioned())
    return NS_ERROR_NOT_INITIALIZED;

  nsINode *ourNode, *otherNode;
  int32_t ourOffset, otherOffset;

  switch (aHow) {
    case nsIDOMRange::START_TO_START:
      ourNode = mStartParent;
      ourOffset = mStartOffset;
      otherNode = otherRange->GetStartParent();
      otherOffset = otherRange->StartOffset();
      break;
    case nsIDOMRange::START_TO_END:
      ourNode = mEndParent;
      ourOffset = mEndOffset;
      otherNode = otherRange->GetStartParent();
      otherOffset = otherRange->StartOffset();
      break;
    case nsIDOMRange::END_TO_START:
      ourNode = mStartParent;
      ourOffset = mStartOffset;
      otherNode = otherRange->GetEndParent();
      otherOffset = otherRange->EndOffset();
      break;
    case nsIDOMRange::END_TO_END:
      ourNode = mEndParent;
      ourOffset = mEndOffset;
      otherNode = otherRange->GetEndParent();
      otherOffset = otherRange->EndOffset();
      break;
    default:
      
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  if (mRoot != otherRange->GetRoot())
    return NS_ERROR_DOM_WRONG_DOCUMENT_ERR;

  *aCmpRet = nsContentUtils::ComparePoints(ourNode, ourOffset,
                                           otherNode, otherOffset);

  return NS_OK;
}

 nsresult
nsRange::CloneParentsBetween(nsIDOMNode *aAncestor,
                             nsIDOMNode *aNode,
                             nsIDOMNode **aClosestAncestor,
                             nsIDOMNode **aFarthestAncestor)
{
  NS_ENSURE_ARG_POINTER((aAncestor && aNode && aClosestAncestor && aFarthestAncestor));

  *aClosestAncestor  = nullptr;
  *aFarthestAncestor = nullptr;

  if (aAncestor == aNode)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> parent, firstParent, lastParent;

  nsresult res = aNode->GetParentNode(getter_AddRefs(parent));

  while(parent && parent != aAncestor)
  {
    nsCOMPtr<nsIDOMNode> clone, tmpNode;

    res = parent->CloneNode(false, 1, getter_AddRefs(clone));

    if (NS_FAILED(res)) return res;
    if (!clone)         return NS_ERROR_FAILURE;

    if (! firstParent)
      firstParent = lastParent = clone;
    else
    {
      res = clone->AppendChild(lastParent, getter_AddRefs(tmpNode));

      if (NS_FAILED(res)) return res;

      lastParent = clone;
    }

    tmpNode = parent;
    res = tmpNode->GetParentNode(getter_AddRefs(parent));
  }

  *aClosestAncestor  = firstParent;
  NS_IF_ADDREF(*aClosestAncestor);

  *aFarthestAncestor = lastParent;
  NS_IF_ADDREF(*aFarthestAncestor);

  return NS_OK;
}

NS_IMETHODIMP
nsRange::CloneContents(nsIDOMDocumentFragment** aReturn)
{
  nsresult res;
  nsCOMPtr<nsIDOMNode> commonAncestor;
  res = GetCommonAncestorContainer(getter_AddRefs(commonAncestor));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMDocument> document =
    do_QueryInterface(mStartParent->OwnerDoc());
  NS_ASSERTION(document, "CloneContents needs a document to continue.");
  if (!document) return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsIDOMDocumentFragment> clonedFrag;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(document));

  res = NS_NewDocumentFragment(getter_AddRefs(clonedFrag),
                               doc->NodeInfoManager());
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMNode> commonCloneAncestor(do_QueryInterface(clonedFrag));
  if (!commonCloneAncestor) return NS_ERROR_FAILURE;

  
  

  RangeSubtreeIterator iter;

  res = iter.Init(this);
  if (NS_FAILED(res)) return res;

  if (iter.IsDone())
  {
    

    *aReturn = clonedFrag;
    NS_IF_ADDREF(*aReturn);
    return NS_OK;
  }

  iter.First();

  
  
  
  
  
  
  
  
  

  while (!iter.IsDone())
  {
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());
    nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
    bool deepClone = !iNode->IsElement() ||
                       (!(iNode == mEndParent && mEndOffset == 0) &&
                        !(iNode == mStartParent &&
                          mStartOffset ==
                            int32_t(iNode->AsElement()->GetChildCount())));

    

    nsCOMPtr<nsIDOMNode> clone;
    res = node->CloneNode(deepClone, 1, getter_AddRefs(clone));
    if (NS_FAILED(res)) return res;

    
    
    
    
    

    nsCOMPtr<nsIDOMCharacterData> charData(do_QueryInterface(clone));

    if (charData)
    {
      if (iNode == mEndParent)
      {
        
        

        uint32_t dataLength = 0;
        res = charData->GetLength(&dataLength);
        if (NS_FAILED(res)) return res;

        if (dataLength > (uint32_t)mEndOffset)
        {
          res = charData->DeleteData(mEndOffset, dataLength - mEndOffset);
          if (NS_FAILED(res)) return res;
        }
      }       

      if (iNode == mStartParent)
      {
        
        

        if (mStartOffset > 0)
        {
          res = charData->DeleteData(0, mStartOffset);
          if (NS_FAILED(res)) return res;
        }
      }
    }

    

    nsCOMPtr<nsIDOMNode> closestAncestor, farthestAncestor;

    res = CloneParentsBetween(commonAncestor, node,
                              getter_AddRefs(closestAncestor),
                              getter_AddRefs(farthestAncestor));

    if (NS_FAILED(res)) return res;

    

    nsCOMPtr<nsIDOMNode> tmpNode;

    if (farthestAncestor)
    {
      res = commonCloneAncestor->AppendChild(farthestAncestor,
                                             getter_AddRefs(tmpNode));

      if (NS_FAILED(res)) return res;
    }

    

    if (closestAncestor)
    {
      
      

      res = closestAncestor->AppendChild(clone, getter_AddRefs(tmpNode));
    }
    else
    {
      
      

      res = commonCloneAncestor->AppendChild(clone, getter_AddRefs(tmpNode));
    }
    if (NS_FAILED(res)) return res;

    
    

    iter.Next();

    if (iter.IsDone())
      break; 

    nsCOMPtr<nsIDOMNode> nextNode(iter.GetCurrentNode());
    if (!nextNode) return NS_ERROR_FAILURE;

    
    nsContentUtils::GetCommonAncestor(node, nextNode, getter_AddRefs(commonAncestor));

    if (!commonAncestor)
      return NS_ERROR_FAILURE;

    

    while (node && node != commonAncestor)
    {
      tmpNode = node;
      res = tmpNode->GetParentNode(getter_AddRefs(node));
      if (NS_FAILED(res)) return res;
      if (!node) return NS_ERROR_FAILURE;

      tmpNode = clone;
      res = tmpNode->GetParentNode(getter_AddRefs(clone));
      if (NS_FAILED(res)) return res;
      if (!clone) return NS_ERROR_FAILURE;
    }

    commonCloneAncestor = clone;
  }

  *aReturn = clonedFrag;
  NS_IF_ADDREF(*aReturn);

  return NS_OK;
}

already_AddRefed<nsRange>
nsRange::CloneRange() const
{
  nsRefPtr<nsRange> range = new nsRange();

  range->SetMaySpanAnonymousSubtrees(mMaySpanAnonymousSubtrees);

  range->DoSetRange(mStartParent, mStartOffset, mEndParent, mEndOffset, mRoot);

  return range.forget();
}

NS_IMETHODIMP
nsRange::CloneRange(nsIDOMRange** aReturn)
{
  *aReturn = CloneRange().get();
  return NS_OK;
}

NS_IMETHODIMP
nsRange::InsertNode(nsIDOMNode* aNode)
{
  VALIDATE_ACCESS(aNode);
  
  nsresult res;
  int32_t tStartOffset;
  this->GetStartOffset(&tStartOffset);

  nsCOMPtr<nsIDOMNode> tStartContainer;
  res = this->GetStartContainer(getter_AddRefs(tStartContainer));
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMNode> referenceNode;
  nsCOMPtr<nsIDOMNode> referenceParentNode = tStartContainer;

  nsCOMPtr<nsIDOMText> startTextNode(do_QueryInterface(tStartContainer));
  nsCOMPtr<nsIDOMNodeList> tChildList;
  if (startTextNode) {
    res = tStartContainer->GetParentNode(getter_AddRefs(referenceParentNode));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(referenceParentNode, NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);

    nsCOMPtr<nsIDOMText> secondPart;
    res = startTextNode->SplitText(tStartOffset, getter_AddRefs(secondPart));
    NS_ENSURE_SUCCESS(res, res);

    referenceNode = secondPart;
  } else {
    res = tStartContainer->GetChildNodes(getter_AddRefs(tChildList));
    NS_ENSURE_SUCCESS(res, res);

    
    res = tChildList->Item(tStartOffset, getter_AddRefs(referenceNode));
    NS_ENSURE_SUCCESS(res, res);
  }

  
  
  
  int32_t newOffset;

  if (referenceNode) {
    newOffset = IndexOf(referenceNode);
  } else {
    uint32_t length;
    res = tChildList->GetLength(&length);
    NS_ENSURE_SUCCESS(res, res);
    newOffset = length;
  }

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_STATE(node);
  if (node->NodeType() == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    newOffset += node->GetChildCount();
  } else {
    newOffset++;
  }

  
  nsCOMPtr<nsIDOMNode> tResultNode;
  res = referenceParentNode->InsertBefore(aNode, referenceNode, getter_AddRefs(tResultNode));
  NS_ENSURE_SUCCESS(res, res);

  if (Collapsed()) {
    return SetEnd(referenceParentNode, newOffset);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRange::SurroundContents(nsIDOMNode* aNewParent)
{
  VALIDATE_ACCESS(aNewParent);

  NS_ENSURE_TRUE(mRoot, NS_ERROR_DOM_INVALID_STATE_ERR);
  
  
  if (mStartParent != mEndParent) {
    bool startIsText = mStartParent->IsNodeOfType(nsINode::eTEXT);
    bool endIsText = mEndParent->IsNodeOfType(nsINode::eTEXT);
    nsINode* startGrandParent = mStartParent->GetParentNode();
    nsINode* endGrandParent = mEndParent->GetParentNode();
    NS_ENSURE_TRUE((startIsText && endIsText &&
                    startGrandParent &&
                    startGrandParent == endGrandParent) ||
                   (startIsText &&
                    startGrandParent &&
                    startGrandParent == mEndParent) ||
                   (endIsText &&
                    endGrandParent &&
                    endGrandParent == mStartParent),
                   NS_ERROR_DOM_INVALID_STATE_ERR);
  }

  
  
  uint16_t nodeType;
  nsresult res = aNewParent->GetNodeType(&nodeType);
  if (NS_FAILED(res)) return res;
  if (nodeType == nsIDOMNode::DOCUMENT_NODE ||
      nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE ||
      nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    return NS_ERROR_DOM_INVALID_NODE_TYPE_ERR;
  }

  

  nsCOMPtr<nsIDOMDocumentFragment> docFrag;

  res = ExtractContents(getter_AddRefs(docFrag));

  if (NS_FAILED(res)) return res;
  if (!docFrag) return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsIDOMNodeList> children;
  res = aNewParent->GetChildNodes(getter_AddRefs(children));

  if (NS_FAILED(res)) return res;
  if (!children) return NS_ERROR_FAILURE;

  uint32_t numChildren = 0;
  res = children->GetLength(&numChildren);
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMNode> tmpNode;

  while (numChildren)
  {
    nsCOMPtr<nsIDOMNode> child;
    res = children->Item(--numChildren, getter_AddRefs(child));

    if (NS_FAILED(res)) return res;
    if (!child) return NS_ERROR_FAILURE;

    res = aNewParent->RemoveChild(child, getter_AddRefs(tmpNode));
    if (NS_FAILED(res)) return res;
  }

  

  res = InsertNode(aNewParent);
  if (NS_FAILED(res)) return res;

  

  res = aNewParent->AppendChild(docFrag, getter_AddRefs(tmpNode));
  if (NS_FAILED(res)) return res;

  

  return SelectNode(aNewParent);
}

NS_IMETHODIMP
nsRange::ToString(nsAString& aReturn)
{ 
  
  aReturn.Truncate();
  
  
  if (!mIsPositioned) {
    return NS_OK;
  }

#ifdef DEBUG_range
      printf("Range dump: -----------------------\n");
#endif 
    
  
  if (mStartParent == mEndParent)
  {
    nsCOMPtr<nsIDOMText> textNode( do_QueryInterface(mStartParent) );
    
    if (textNode)
    {
#ifdef DEBUG_range
      
      nsCOMPtr<nsIContent> cN (do_QueryInterface(mStartParent));
      if (cN) cN->List(stdout);
      printf("End Range dump: -----------------------\n");
#endif 

      
      if (NS_FAILED(textNode->SubstringData(mStartOffset,mEndOffset-mStartOffset,aReturn)))
        return NS_ERROR_UNEXPECTED;
      return NS_OK;
    }
  } 
  
  



  nsCOMPtr<nsIContentIterator> iter = NS_NewContentIterator();
  nsresult rv = iter->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString tempString;
 
  
  
  while (!iter->IsDone())
  {
    nsINode *n = iter->GetCurrentNode();

#ifdef DEBUG_range
    
    n->List(stdout);
#endif 
    nsCOMPtr<nsIDOMText> textNode(do_QueryInterface(n));
    if (textNode) 
    {
      if (n == mStartParent) 
      {
        uint32_t strLength;
        textNode->GetLength(&strLength);
        textNode->SubstringData(mStartOffset,strLength-mStartOffset,tempString);
        aReturn += tempString;
      }
      else if (n == mEndParent)  
      {
        textNode->SubstringData(0,mEndOffset,tempString);
        aReturn += tempString;
      }
      else  
      {
        textNode->GetData(tempString);
        aReturn += tempString;
      }
    }

    iter->Next();
  }

#ifdef DEBUG_range
  printf("End Range dump: -----------------------\n");
#endif 
  return NS_OK;
}



NS_IMETHODIMP
nsRange::Detach()
{
  
  mIsDetached = true;
  return NS_OK;
}

NS_IMETHODIMP    
nsRange::CreateContextualFragment(const nsAString& aFragment,
                                  nsIDOMDocumentFragment** aReturn)
{
  if (mIsPositioned) {
    return nsContentUtils::CreateContextualFragment(mStartParent, aFragment,
                                                    false, aReturn);
  }
  return NS_ERROR_FAILURE;
}

static void ExtractRectFromOffset(nsIFrame* aFrame,
                                  const nsIFrame* aRelativeTo, 
                                  const int32_t aOffset, nsRect* aR, bool aKeepLeft)
{
  nsPoint point;
  aFrame->GetPointFromOffset(aOffset, &point);

  point += aFrame->GetOffsetTo(aRelativeTo);

  
  
  NS_ASSERTION(aR->x <= point.x && point.x <= aR->XMost(),
                   "point.x should not be outside of rect r");

  if (aKeepLeft) {
    aR->width = point.x - aR->x;
  } else {
    aR->width = aR->XMost() - point.x;
    aR->x = point.x;
  }
}

static nsresult GetPartialTextRect(nsLayoutUtils::RectCallback* aCallback,
                                   nsIContent* aContent, int32_t aStartOffset, int32_t aEndOffset)
{
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame && frame->GetType() == nsGkAtoms::textFrame) {
    nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
    nsIFrame* relativeTo = nsLayoutUtils::GetContainingBlockForClientRect(textFrame);
    for (nsTextFrame* f = textFrame; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
      int32_t fstart = f->GetContentOffset(), fend = f->GetContentEnd();
      if (fend <= aStartOffset || fstart >= aEndOffset)
        continue;

      
      f->EnsureTextRun(nsTextFrame::eInflated);
      NS_ENSURE_TRUE(f->GetTextRun(nsTextFrame::eInflated), NS_ERROR_OUT_OF_MEMORY);
      bool rtl = f->GetTextRun(nsTextFrame::eInflated)->IsRightToLeft();
      nsRect r(f->GetOffsetTo(relativeTo), f->GetSize());
      if (fstart < aStartOffset) {
        
        ExtractRectFromOffset(f, relativeTo, aStartOffset, &r, rtl);
      }
      if (fend > aEndOffset) {
        
        ExtractRectFromOffset(f, relativeTo, aEndOffset, &r, !rtl);
      }
      aCallback->AddRect(r);
    }
  }
  return NS_OK;
}

static void CollectClientRects(nsLayoutUtils::RectCallback* aCollector, 
                               nsRange* aRange,
                               nsINode* aStartParent, int32_t aStartOffset,
                               nsINode* aEndParent, int32_t aEndOffset)
{
  
  nsCOMPtr<nsIDOMNode> startContainer = do_QueryInterface(aStartParent);
  nsCOMPtr<nsIDOMNode> endContainer = do_QueryInterface(aEndParent);

  
  if (!aStartParent->IsInDoc()) {
    return;
  }

  aStartParent->GetCurrentDoc()->FlushPendingNotifications(Flush_Layout);

  
  if (!aStartParent->IsInDoc()) {
    return;
  }

  RangeSubtreeIterator iter;

  nsresult rv = iter.Init(aRange);
  if (NS_FAILED(rv)) return;

  if (iter.IsDone()) {
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(aStartParent);
    if (content && content->IsNodeOfType(nsINode::eTEXT)) {
      nsIFrame* frame = content->GetPrimaryFrame();
      if (frame && frame->GetType() == nsGkAtoms::textFrame) {
        nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
        int32_t outOffset;
        nsIFrame* outFrame;
        textFrame->GetChildFrameContainingOffset(aStartOffset, false, 
          &outOffset, &outFrame);
        if (outFrame) {
           nsIFrame* relativeTo = 
             nsLayoutUtils::GetContainingBlockForClientRect(outFrame);
           nsRect r(outFrame->GetOffsetTo(relativeTo), outFrame->GetSize());
           ExtractRectFromOffset(outFrame, relativeTo, aStartOffset, &r, false);
           r.width = 0;
           aCollector->AddRect(r);
        }
      }
    }
    return;
  }

  do {
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());
    iter.Next();
    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    if (!content)
      continue;
    if (content->IsNodeOfType(nsINode::eTEXT)) {
       if (node == startContainer) {
         int32_t offset = startContainer == endContainer ? 
           aEndOffset : content->GetText()->GetLength();
         GetPartialTextRect(aCollector, content, aStartOffset, offset);
         continue;
       } else if (node == endContainer) {
         GetPartialTextRect(aCollector, content, 0, aEndOffset);
         continue;
       }
    }

    nsIFrame* frame = content->GetPrimaryFrame();
    if (frame) {
      nsLayoutUtils::GetAllInFlowRects(frame,
        nsLayoutUtils::GetContainingBlockForClientRect(frame), aCollector);
    }
  } while (!iter.IsDone());
}

NS_IMETHODIMP
nsRange::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  *aResult = nullptr;

  
  nsClientRect* rect = new nsClientRect();
  if (!rect)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rect);

  if (!mStartParent)
    return NS_OK;

  nsLayoutUtils::RectAccumulator accumulator;
  
  CollectClientRects(&accumulator, this, mStartParent, mStartOffset, 
    mEndParent, mEndOffset);

  nsRect r = accumulator.mResultRect.IsEmpty() ? accumulator.mFirstRect : 
    accumulator.mResultRect;
  rect->SetLayoutRect(r);
  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetClientRects(nsIDOMClientRectList** aResult)
{
  *aResult = nullptr;

  if (!mStartParent)
    return NS_OK;

  nsRefPtr<nsClientRectList> rectList =
    new nsClientRectList(static_cast<nsIDOMRange*>(this));
  if (!rectList)
    return NS_ERROR_OUT_OF_MEMORY;

  nsLayoutUtils::RectListBuilder builder(rectList);

  CollectClientRects(&builder, this, mStartParent, mStartOffset, 
    mEndParent, mEndOffset);

  if (NS_FAILED(builder.mRV))
    return builder.mRV;
  rectList.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetUsedFontFaces(nsIDOMFontFaceList** aResult)
{
  *aResult = nullptr;

  NS_ENSURE_TRUE(mStartParent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMNode> startContainer = do_QueryInterface(mStartParent);
  nsCOMPtr<nsIDOMNode> endContainer = do_QueryInterface(mEndParent);

  
  nsIDocument* doc = mStartParent->OwnerDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);
  doc->FlushPendingNotifications(Flush_Frames);

  
  NS_ENSURE_TRUE(mStartParent->IsInDoc(), NS_ERROR_UNEXPECTED);

  nsRefPtr<nsFontFaceList> fontFaceList = new nsFontFaceList();

  RangeSubtreeIterator iter;
  nsresult rv = iter.Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  while (!iter.IsDone()) {
    
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());
    iter.Next();

    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    if (!content) {
      continue;
    }
    nsIFrame* frame = content->GetPrimaryFrame();
    if (!frame) {
      continue;
    }

    if (content->IsNodeOfType(nsINode::eTEXT)) {
       if (node == startContainer) {
         int32_t offset = startContainer == endContainer ? 
           mEndOffset : content->GetText()->GetLength();
         nsLayoutUtils::GetFontFacesForText(frame, mStartOffset, offset,
                                            true, fontFaceList);
         continue;
       }
       if (node == endContainer) {
         nsLayoutUtils::GetFontFacesForText(frame, 0, mEndOffset,
                                            true, fontFaceList);
         continue;
       }
    }

    nsLayoutUtils::GetFontFacesForFrames(frame, fontFaceList);
  }

  fontFaceList.forget(aResult);
  return NS_OK;
}

nsINode*
nsRange::GetRegisteredCommonAncestor()
{
  NS_ASSERTION(IsInSelection(),
               "GetRegisteredCommonAncestor only valid for range in selection");
  nsINode* ancestor = GetNextRangeCommonAncestor(mStartParent);
  while (ancestor) {
    RangeHashTable* ranges =
      static_cast<RangeHashTable*>(ancestor->GetProperty(nsGkAtoms::range));
    if (ranges->GetEntry(this)) {
      break;
    }
    ancestor = GetNextRangeCommonAncestor(ancestor->GetParentNode());
  }
  NS_ASSERTION(ancestor, "can't find common ancestor for selected range");
  return ancestor;
}

 bool nsRange::AutoInvalidateSelection::mIsNested;

nsRange::AutoInvalidateSelection::~AutoInvalidateSelection()
{
  NS_ASSERTION(mWasInSelection == mRange->IsInSelection(),
               "Range got unselected in AutoInvalidateSelection block");
  if (!mCommonAncestor) {
    return;
  }
  mIsNested = false;
  ::InvalidateAllFrames(mCommonAncestor);
  nsINode* commonAncestor = mRange->GetRegisteredCommonAncestor();
  if (commonAncestor != mCommonAncestor) {
    ::InvalidateAllFrames(commonAncestor);
  }
}
