








#include "nscore.h"
#include "nsRange.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMText.h"
#include "nsError.h"
#include "nsIContentIterator.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsGenericDOMDataNode.h"
#include "nsTextFrame.h"
#include "nsFontFaceList.h"
#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/dom/DocumentType.h"
#include "mozilla/dom/RangeBinding.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Likely.h"
#include "nsCSSFrameConstructor.h"

using namespace mozilla;
using namespace mozilla::dom;

JSObject*
nsRange::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return RangeBinding::Wrap(aCx, this, aGivenProto);
}





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
nsRange::CreateRange(nsINode* aStartParent, int32_t aStartOffset,
                     nsINode* aEndParent, int32_t aEndOffset,
                     nsRange** aRange)
{
  nsCOMPtr<nsIDOMNode> startDomNode = do_QueryInterface(aStartParent);
  nsCOMPtr<nsIDOMNode> endDomNode = do_QueryInterface(aEndParent);

  nsresult rv = CreateRange(startDomNode, aStartOffset, endDomNode, aEndOffset,
                            aRange);

  return rv;

}


nsresult
nsRange::CreateRange(nsIDOMNode* aStartParent, int32_t aStartOffset,
                     nsIDOMNode* aEndParent, int32_t aEndOffset,
                     nsRange** aRange)
{
  MOZ_ASSERT(aRange);
  *aRange = nullptr;

  nsCOMPtr<nsINode> startParent = do_QueryInterface(aStartParent);
  NS_ENSURE_ARG_POINTER(startParent);

  nsRefPtr<nsRange> range = new nsRange(startParent);

  nsresult rv = range->SetStart(startParent, aStartOffset);
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
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_LAST_RELEASE(nsRange,
                                                   DoSetRange(nullptr, 0, nullptr, 0, nullptr))


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsRange)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMRange)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMRange)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsRange)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsRange)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOwner);
  tmp->Reset();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsRange)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStartParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEndParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRoot)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsRange)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

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
    aNode->SetProperty(nsGkAtoms::range, ranges,
                       nsINode::DeleteProperty<nsRange::RangeHashTable>, true);
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
        uint32_t(mStartOffset) < parentNode->GetChildCount() &&
        removed == parentNode->GetChildAt(mStartOffset)) {
      newStartNode = aContent;
      newStartOffset = aInfo->mChangeStart;
    }
    if (parentNode == mEndParent && mEndOffset > 0 &&
        uint32_t(mEndOffset) < parentNode->GetChildCount() &&
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
  bool didCheckStartParentDescendant = false;

  
  if (container == mStartParent) {
    if (aIndexInContainer < mStartOffset) {
      --mStartOffset;
    }
  } else { 
    didCheckStartParentDescendant = true;
    gravitateStart = nsContentUtils::ContentIsDescendantOf(mStartParent, aChild);
  }

  
  if (container == mEndParent) {
    if (aIndexInContainer < mEndOffset) {
      --mEndOffset;
    }
  } else if (didCheckStartParentDescendant && mStartParent == mEndParent) {
    gravitateEnd = gravitateStart;
  } else {
    gravitateEnd = nsContentUtils::ContentIsDescendantOf(mEndParent, aChild);
  }

  if (!mEnableGravitationOnElementRemoval) {
    
    return;
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
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  if (!parent) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  *aResult = IsPointInRange(*parent, aOffset, rv);
  return rv.StealNSResult();
}

bool
nsRange::IsPointInRange(nsINode& aParent, uint32_t aOffset, ErrorResult& aRv)
{
  uint16_t compareResult = ComparePoint(aParent, aOffset, aRv);
  
  if (aRv.ErrorCodeIs(NS_ERROR_DOM_WRONG_DOCUMENT_ERR)) {
    aRv.SuppressException();
    return false;
  }

  return compareResult == 0;
}



NS_IMETHODIMP
nsRange::ComparePoint(nsIDOMNode* aParent, int32_t aOffset, int16_t* aResult)
{
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  NS_ENSURE_TRUE(parent, NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);

  ErrorResult rv;
  *aResult = ComparePoint(*parent, aOffset, rv);
  return rv.StealNSResult();
}

int16_t
nsRange::ComparePoint(nsINode& aParent, uint32_t aOffset, ErrorResult& aRv)
{
  
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_NOT_INITIALIZED);
    return 0;
  }

  if (!nsContentUtils::ContentIsDescendantOf(&aParent, mRoot)) {
    aRv.Throw(NS_ERROR_DOM_WRONG_DOCUMENT_ERR);
    return 0;
  }

  if (aParent.NodeType() == nsIDOMNode::DOCUMENT_TYPE_NODE) {
    aRv.Throw(NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);
    return 0;
  }

  if (aOffset > aParent.Length()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return 0;
  }

  int32_t cmp;
  if ((cmp = nsContentUtils::ComparePoints(&aParent, aOffset,
                                           mStartParent, mStartOffset)) <= 0) {

    return cmp;
  }
  if (nsContentUtils::ComparePoints(mEndParent, mEndOffset,
                                    &aParent, aOffset) == -1) {
    return 1;
  }

  return 0;
}

NS_IMETHODIMP
nsRange::IntersectsNode(nsIDOMNode* aNode, bool* aResult)
{
  *aResult = false;

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  
  NS_ENSURE_ARG(node);

  ErrorResult rv;
  *aResult = IntersectsNode(*node, rv);
  return rv.StealNSResult();
}

bool
nsRange::IntersectsNode(nsINode& aNode, ErrorResult& aRv)
{
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_NOT_INITIALIZED);
    return false;
  }

  
  nsINode* parent = aNode.GetParentNode();
  if (!parent) {
    
    
    return GetRoot() == &aNode;
  }

  
  int32_t nodeIndex = parent->IndexOf(&aNode);

  
  
  bool disconnected = false;
  bool result = nsContentUtils::ComparePoints(mStartParent, mStartOffset,
                                             parent, nodeIndex + 1,
                                             &disconnected) < 0 &&
               nsContentUtils::ComparePoints(parent, nodeIndex,
                                             mEndParent, mEndOffset,
                                             &disconnected) < 0;

  
  if (disconnected) {
    result = false;
  }
  return result;
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
IndexOf(nsINode* aChild)
{
  nsINode* parent = aChild->GetParentNode();

  return parent ? parent->IndexOf(aChild) : -1;
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

nsINode*
nsRange::GetStartContainer(ErrorResult& aRv) const
{
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_NOT_INITIALIZED);
    return nullptr;
  }

  return mStartParent;
}

NS_IMETHODIMP
nsRange::GetStartOffset(int32_t* aStartOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aStartOffset = mStartOffset;

  return NS_OK;
}

uint32_t
nsRange::GetStartOffset(ErrorResult& aRv) const
{
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_NOT_INITIALIZED);
    return 0;
  }

  return mStartOffset;
}

NS_IMETHODIMP
nsRange::GetEndContainer(nsIDOMNode** aEndParent)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  return CallQueryInterface(mEndParent, aEndParent);
}

nsINode*
nsRange::GetEndContainer(ErrorResult& aRv) const
{
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_NOT_INITIALIZED);
    return nullptr;
  }

  return mEndParent;
}

NS_IMETHODIMP
nsRange::GetEndOffset(int32_t* aEndOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aEndOffset = mEndOffset;

  return NS_OK;
}

uint32_t
nsRange::GetEndOffset(ErrorResult& aRv) const
{
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_NOT_INITIALIZED);
    return 0;
  }

  return mEndOffset;
}

NS_IMETHODIMP
nsRange::GetCollapsed(bool* aIsCollapsed)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aIsCollapsed = Collapsed();

  return NS_OK;
}

nsINode*
nsRange::GetCommonAncestorContainer(ErrorResult& aRv) const
{
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_NOT_INITIALIZED);
    return nullptr;
  }

  return nsContentUtils::GetCommonAncestor(mStartParent, mEndParent);
}

NS_IMETHODIMP
nsRange::GetCommonAncestorContainer(nsIDOMNode** aCommonParent)
{
  ErrorResult rv;
  nsINode* commonAncestor = GetCommonAncestorContainer(rv);
  if (commonAncestor) {
    NS_ADDREF(*aCommonParent = commonAncestor->AsDOMNode());
  } else {
    *aCommonParent = nullptr;
  }

  return rv.StealNSResult();
}

nsINode*
nsRange::IsValidBoundary(nsINode* aNode)
{
  if (!aNode) {
    return nullptr;
  }

  if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
    if (aNode->NodeInfo()->NameAtom() == nsGkAtoms::documentTypeNodeName) {
      return nullptr;
    }

    nsIContent* content = static_cast<nsIContent*>(aNode);

    if (!mMaySpanAnonymousSubtrees) {
      
      ShadowRoot* containingShadow = content->GetContainingShadow();
      if (containingShadow) {
        return containingShadow;
      }

      
      
      nsINode* root = content->GetBindingParent();
      if (root) {
        return root;
      }
    }
  }

  
  
  nsINode* root = aNode->GetUncomposedDoc();
  if (root) {
    return root;
  }

  root = aNode->SubtreeRoot();

  NS_ASSERTION(!root->IsNodeOfType(nsINode::eDOCUMENT),
               "GetUncomposedDoc should have returned a doc");

  
  return root;
}

void
nsRange::SetStart(nsINode& aNode, uint32_t aOffset, ErrorResult& aRv)
{
 if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  aRv = SetStart(&aNode, aOffset);
}

NS_IMETHODIMP
nsRange::SetStart(nsIDOMNode* aParent, int32_t aOffset)
{
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  if (!parent) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  SetStart(*parent, aOffset, rv);
  return rv.StealNSResult();
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

void
nsRange::SetStartBefore(nsINode& aNode, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  aRv = SetStart(aNode.GetParentNode(), IndexOf(&aNode));
}

NS_IMETHODIMP
nsRange::SetStartBefore(nsIDOMNode* aSibling)
{
  nsCOMPtr<nsINode> sibling = do_QueryInterface(aSibling);
  if (!sibling) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  SetStartBefore(*sibling, rv);
  return rv.StealNSResult();
}

void
nsRange::SetStartAfter(nsINode& aNode, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  aRv = SetStart(aNode.GetParentNode(), IndexOf(&aNode) + 1);
}

NS_IMETHODIMP
nsRange::SetStartAfter(nsIDOMNode* aSibling)
{
  nsCOMPtr<nsINode> sibling = do_QueryInterface(aSibling);
  if (!sibling) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  SetStartAfter(*sibling, rv);
  return rv.StealNSResult();
}

void
nsRange::SetEnd(nsINode& aNode, uint32_t aOffset, ErrorResult& aRv)
{
 if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }
  AutoInvalidateSelection atEndOfBlock(this);
  aRv = SetEnd(&aNode, aOffset);
}

NS_IMETHODIMP
nsRange::SetEnd(nsIDOMNode* aParent, int32_t aOffset)
{
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  if (!parent) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  SetEnd(*parent, aOffset, rv);
  return rv.StealNSResult();
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

void
nsRange::SetEndBefore(nsINode& aNode, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  aRv = SetEnd(aNode.GetParentNode(), IndexOf(&aNode));
}

NS_IMETHODIMP
nsRange::SetEndBefore(nsIDOMNode* aSibling)
{
  nsCOMPtr<nsINode> sibling = do_QueryInterface(aSibling);
  if (!sibling) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  SetEndBefore(*sibling, rv);
  return rv.StealNSResult();
}

void
nsRange::SetEndAfter(nsINode& aNode, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  aRv = SetEnd(aNode.GetParentNode(), IndexOf(&aNode) + 1);
}

NS_IMETHODIMP
nsRange::SetEndAfter(nsIDOMNode* aSibling)
{
  nsCOMPtr<nsINode> sibling = do_QueryInterface(aSibling);
  if (!sibling) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  SetEndAfter(*sibling, rv);
  return rv.StealNSResult();
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
  nsCOMPtr<nsINode> node = do_QueryInterface(aN);
  NS_ENSURE_TRUE(node, NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);

  ErrorResult rv;
  SelectNode(*node, rv);
  return rv.StealNSResult();
}

void
nsRange::SelectNode(nsINode& aNode, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  nsINode* parent = aNode.GetParentNode();
  nsINode* newRoot = IsValidBoundary(parent);
  if (!newRoot) {
    aRv.Throw(NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);
    return;
  }

  int32_t index = parent->IndexOf(&aNode);
  if (index < 0) {
    aRv.Throw(NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);
    return;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  DoSetRange(parent, index, parent, index + 1, newRoot);
}

NS_IMETHODIMP
nsRange::SelectNodeContents(nsIDOMNode* aN)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aN);
  NS_ENSURE_TRUE(node, NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);

  ErrorResult rv;
  SelectNodeContents(*node, rv);
  return rv.StealNSResult();
}

void
nsRange::SelectNodeContents(nsINode& aNode, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  nsINode* newRoot = IsValidBoundary(&aNode);
  if (!newRoot) {
    aRv.Throw(NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);
    return;
  }

  AutoInvalidateSelection atEndOfBlock(this);
  DoSetRange(&aNode, 0, &aNode, aNode.Length(), newRoot);
}














class MOZ_STACK_CLASS RangeSubtreeIterator
{
private:

  enum RangeSubtreeIterState { eDone=0,
                               eUseStart,
                               eUseIterator,
                               eUseEnd };

  nsCOMPtr<nsIContentIterator>  mIter;
  RangeSubtreeIterState         mIterState;

  nsCOMPtr<nsINode> mStart;
  nsCOMPtr<nsINode> mEnd;

public:

  RangeSubtreeIterator()
    : mIterState(eDone)
  {
  }
  ~RangeSubtreeIterator()
  {
  }

  nsresult Init(nsRange *aRange);
  already_AddRefed<nsINode> GetCurrentNode();
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
RangeSubtreeIterator::Init(nsRange *aRange)
{
  mIterState = eDone;
  if (aRange->Collapsed()) {
    return NS_OK;
  }

  
  
  

  ErrorResult rv;
  nsCOMPtr<nsINode> node = aRange->GetStartContainer(rv);
  if (!node) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCharacterData> startData = do_QueryInterface(node);
  if (startData || (node->IsElement() &&
                    node->AsElement()->GetChildCount() == aRange->GetStartOffset(rv))) {
    mStart = node;
  }

  
  
  

  node = aRange->GetEndContainer(rv);
  if (!node) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCharacterData> endData = do_QueryInterface(node);
  if (endData || (node->IsElement() && aRange->GetEndOffset(rv) == 0)) {
    mEnd = node;
  }

  if (mStart && mStart == mEnd)
  {
    
    
    

    mEnd = nullptr;
  }
  else
  {
    
    

    mIter = NS_NewContentSubtreeIterator();

    nsresult res = mIter->Init(aRange);
    if (NS_FAILED(res)) return res;

    if (mIter->IsDone())
    {
      
      
      

      mIter = nullptr;
    }
  }

  
  

  First();

  return NS_OK;
}

already_AddRefed<nsINode>
RangeSubtreeIterator::GetCurrentNode()
{
  nsCOMPtr<nsINode> node;

  if (mIterState == eUseStart && mStart) {
    node = mStart;
  } else if (mIterState == eUseEnd && mEnd) {
    node = mEnd;
  } else if (mIterState == eUseIterator && mIter) {
    node = mIter->GetCurrentNode();
  }

  return node.forget();
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
CollapseRangeAfterDelete(nsRange* aRange)
{
  NS_ENSURE_ARG_POINTER(aRange);

  
  if (aRange->Collapsed())
  {
    
    
    
    
    
    
    
    
    
    
    

    return NS_OK;
  }

  
  

  ErrorResult rv;
  nsCOMPtr<nsINode> commonAncestor = aRange->GetCommonAncestorContainer(rv);
  if (rv.Failed()) return rv.StealNSResult();

  nsCOMPtr<nsINode> startContainer = aRange->GetStartContainer(rv);
  if (rv.Failed()) return rv.StealNSResult();
  nsCOMPtr<nsINode> endContainer = aRange->GetEndContainer(rv);
  if (rv.Failed()) return rv.StealNSResult();

  
  
  
  

  if (startContainer == commonAncestor)
    return aRange->Collapse(true);
  if (endContainer == commonAncestor)
    return aRange->Collapse(false);

  
  
  

  nsCOMPtr<nsINode> nodeToSelect(startContainer);

  while (nodeToSelect)
  {
    nsCOMPtr<nsINode> parent = nodeToSelect->GetParentNode();
    if (parent == commonAncestor)
      break; 

    nodeToSelect = parent;
  }

  if (!nodeToSelect)
    return NS_ERROR_FAILURE; 

  aRange->SelectNode(*nodeToSelect, rv);
  if (rv.Failed()) return rv.StealNSResult();

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
PrependChild(nsINode* aParent, nsINode* aChild)
{
  nsCOMPtr<nsINode> first = aParent->GetFirstChild();
  ErrorResult rv;
  aParent->InsertBefore(*aChild, first, rv);
  return rv.StealNSResult();
}



static bool
ValidateCurrentNode(nsRange* aRange, RangeSubtreeIterator& aIter)
{
  bool before, after;
  nsCOMPtr<nsINode> node = aIter.GetCurrentNode();
  if (!node) {
    
    
    return true;
  }

  nsresult res = nsRange::CompareNodeToRange(node, aRange, &before, &after);

  return NS_SUCCEEDED(res) && !before && !after;
}

nsresult
nsRange::CutContents(DocumentFragment** aFragment)
{
  if (aFragment) {
    *aFragment = nullptr;
  }

  nsCOMPtr<nsIDocument> doc = mStartParent->OwnerDoc();

  ErrorResult res;
  nsCOMPtr<nsINode> commonAncestor = GetCommonAncestorContainer(res);
  NS_ENSURE_TRUE(!res.Failed(), res.StealNSResult());

  
  nsRefPtr<DocumentFragment> retval;
  if (aFragment) {
    retval = new DocumentFragment(doc->NodeInfoManager());
  }
  nsCOMPtr<nsINode> commonCloneAncestor = retval.get();

  
  mozAutoSubtreeModified subtree(mRoot ? mRoot->OwnerDoc(): nullptr, nullptr);

  
  

  nsCOMPtr<nsINode> startContainer = mStartParent;
  int32_t              startOffset = mStartOffset;
  nsCOMPtr<nsINode> endContainer = mEndParent;
  int32_t              endOffset = mEndOffset;

  if (retval) {
    
    
    
    nsCOMPtr<nsIDocument> commonAncestorDocument = do_QueryInterface(commonAncestor);
    if (commonAncestorDocument) {
      nsRefPtr<DocumentType> doctype = commonAncestorDocument->GetDoctype();

      if (doctype &&
          nsContentUtils::ComparePoints(startContainer, startOffset,
                                        doctype, 0) < 0 &&
          nsContentUtils::ComparePoints(doctype, 0,
                                        endContainer, endOffset) < 0) {
        return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
      }
    }
  }

  
  

  RangeSubtreeIterator iter;

  nsresult rv = iter.Init(this);
  if (NS_FAILED(rv)) return rv;

  if (iter.IsDone())
  {
    
    rv = CollapseRangeAfterDelete(this);
    if (NS_SUCCEEDED(rv) && aFragment) {
      retval.forget(aFragment);
    }
    return rv;
  }

  

  iter.Last();

  bool handled = false;

  
  
  

  while (!iter.IsDone())
  {
    nsCOMPtr<nsINode> nodeToResult;
    nsCOMPtr<nsINode> node = iter.GetCurrentNode();

    
    

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
              nodeToResult = do_QueryInterface(clone);
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
            nodeToResult = do_QueryInterface(cutNode);
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
          nodeToResult = do_QueryInterface(cutNode);
        }

        handled = true;
      }
    }

    if (!handled && (node == endContainer || node == startContainer))
    {
      if (node && node->IsElement() &&
          ((node == endContainer && endOffset == 0) ||
           (node == startContainer &&
            int32_t(node->AsElement()->GetChildCount()) == startOffset)))
      {
        if (retval) {
          ErrorResult rv;
          nodeToResult = node->CloneNode(false, rv);
          NS_ENSURE_TRUE(!rv.Failed(), rv.StealNSResult());
        }
        handled = true;
      }
    }

    if (!handled)
    {
      
      
      nodeToResult = node;
    }

    uint32_t parentCount = 0;
    
    if (retval) {
      nsCOMPtr<nsINode> oldCommonAncestor = commonAncestor;
      if (!iter.IsDone()) {
        
        nsCOMPtr<nsINode> prevNode = iter.GetCurrentNode();
        NS_ENSURE_STATE(prevNode);

        
        
        commonAncestor = nsContentUtils::GetCommonAncestor(node, prevNode);
        NS_ENSURE_STATE(commonAncestor);

        nsCOMPtr<nsINode> parentCounterNode = node;
        while (parentCounterNode && parentCounterNode != commonAncestor)
        {
          ++parentCount;
          parentCounterNode = parentCounterNode->GetParentNode();
          NS_ENSURE_STATE(parentCounterNode);
        }
      }

      
      nsCOMPtr<nsINode> closestAncestor, farthestAncestor;
      rv = CloneParentsBetween(oldCommonAncestor, node,
                               getter_AddRefs(closestAncestor),
                               getter_AddRefs(farthestAncestor));
      NS_ENSURE_SUCCESS(rv, rv);

      if (farthestAncestor)
      {
        nsCOMPtr<nsINode> n = do_QueryInterface(commonCloneAncestor);
        rv = PrependChild(n, farthestAncestor);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsMutationGuard guard;
      nsCOMPtr<nsINode> parent = nodeToResult->GetParentNode();
      rv = closestAncestor ? PrependChild(closestAncestor, nodeToResult)
                           : PrependChild(commonCloneAncestor, nodeToResult);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(!guard.Mutated(parent ? 2 : 1) ||
                      ValidateCurrentNode(this, iter));
    } else if (nodeToResult) {
      nsMutationGuard guard;
      nsCOMPtr<nsINode> node = nodeToResult;
      nsINode* parent = node->GetParentNode();
      if (parent) {
        mozilla::ErrorResult error;
        parent->RemoveChild(*node, error);
        NS_ENSURE_FALSE(error.Failed(), error.StealNSResult());
      }
      NS_ENSURE_STATE(!guard.Mutated(1) ||
                      ValidateCurrentNode(this, iter));
    }

    if (!iter.IsDone() && retval) {
      
      nsCOMPtr<nsINode> newCloneAncestor = nodeToResult;
      for (uint32_t i = parentCount; i; --i)
      {
        newCloneAncestor = newCloneAncestor->GetParentNode();
        NS_ENSURE_STATE(newCloneAncestor);
      }
      commonCloneAncestor = newCloneAncestor;
    }
  }

  rv = CollapseRangeAfterDelete(this);
  if (NS_SUCCEEDED(rv) && aFragment) {
    retval.forget(aFragment);
  }
  return rv;
}

NS_IMETHODIMP
nsRange::DeleteContents()
{
  return CutContents(nullptr);
}

void
nsRange::DeleteContents(ErrorResult& aRv)
{
  aRv = CutContents(nullptr);
}

NS_IMETHODIMP
nsRange::ExtractContents(nsIDOMDocumentFragment** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  nsRefPtr<DocumentFragment> fragment;
  nsresult rv = CutContents(getter_AddRefs(fragment));
  fragment.forget(aReturn);
  return rv;
}

already_AddRefed<DocumentFragment>
nsRange::ExtractContents(ErrorResult& rv)
{
  nsRefPtr<DocumentFragment> fragment;
  rv = CutContents(getter_AddRefs(fragment));
  return fragment.forget();
}

NS_IMETHODIMP
nsRange::CompareBoundaryPoints(uint16_t aHow, nsIDOMRange* aOtherRange,
                               int16_t* aCmpRet)
{
  nsRange* otherRange = static_cast<nsRange*>(aOtherRange);
  NS_ENSURE_TRUE(otherRange, NS_ERROR_NULL_POINTER);

  ErrorResult rv;
  *aCmpRet = CompareBoundaryPoints(aHow, *otherRange, rv);
  return rv.StealNSResult();
}

int16_t
nsRange::CompareBoundaryPoints(uint16_t aHow, nsRange& aOtherRange,
                               ErrorResult& rv)
{
  if (!mIsPositioned || !aOtherRange.IsPositioned()) {
    rv.Throw(NS_ERROR_NOT_INITIALIZED);
    return 0;
  }

  nsINode *ourNode, *otherNode;
  int32_t ourOffset, otherOffset;

  switch (aHow) {
    case nsIDOMRange::START_TO_START:
      ourNode = mStartParent;
      ourOffset = mStartOffset;
      otherNode = aOtherRange.GetStartParent();
      otherOffset = aOtherRange.StartOffset();
      break;
    case nsIDOMRange::START_TO_END:
      ourNode = mEndParent;
      ourOffset = mEndOffset;
      otherNode = aOtherRange.GetStartParent();
      otherOffset = aOtherRange.StartOffset();
      break;
    case nsIDOMRange::END_TO_START:
      ourNode = mStartParent;
      ourOffset = mStartOffset;
      otherNode = aOtherRange.GetEndParent();
      otherOffset = aOtherRange.EndOffset();
      break;
    case nsIDOMRange::END_TO_END:
      ourNode = mEndParent;
      ourOffset = mEndOffset;
      otherNode = aOtherRange.GetEndParent();
      otherOffset = aOtherRange.EndOffset();
      break;
    default:
      
      rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return 0;
  }

  if (mRoot != aOtherRange.GetRoot()) {
    rv.Throw(NS_ERROR_DOM_WRONG_DOCUMENT_ERR);
    return 0;
  }

  return nsContentUtils::ComparePoints(ourNode, ourOffset,
                                       otherNode, otherOffset);
}

 nsresult
nsRange::CloneParentsBetween(nsINode *aAncestor,
                             nsINode *aNode,
                             nsINode **aClosestAncestor,
                             nsINode **aFarthestAncestor)
{
  NS_ENSURE_ARG_POINTER((aAncestor && aNode && aClosestAncestor && aFarthestAncestor));

  *aClosestAncestor  = nullptr;
  *aFarthestAncestor = nullptr;

  if (aAncestor == aNode)
    return NS_OK;

  nsCOMPtr<nsINode> firstParent, lastParent;
  nsCOMPtr<nsINode> parent = aNode->GetParentNode();

  while(parent && parent != aAncestor)
  {
    ErrorResult rv;
    nsCOMPtr<nsINode> clone = parent->CloneNode(false, rv);

    if (rv.Failed()) {
      return rv.StealNSResult();
    }
    if (!clone) {
      return NS_ERROR_FAILURE;
    }

    if (! firstParent) {
      firstParent = lastParent = clone;
    } else {
      clone->AppendChild(*lastParent, rv);
      if (rv.Failed()) return rv.StealNSResult();

      lastParent = clone;
    }

    parent = parent->GetParentNode();
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
  ErrorResult rv;
  *aReturn = CloneContents(rv).take();
  return rv.StealNSResult();
}

already_AddRefed<DocumentFragment>
nsRange::CloneContents(ErrorResult& aRv)
{
  nsCOMPtr<nsINode> commonAncestor = GetCommonAncestorContainer(aRv);
  MOZ_ASSERT(!aRv.Failed(), "GetCommonAncestorContainer() shouldn't fail!");

  nsCOMPtr<nsIDocument> doc = mStartParent->OwnerDoc();
  NS_ASSERTION(doc, "CloneContents needs a document to continue.");
  if (!doc) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  
  


  nsRefPtr<DocumentFragment> clonedFrag =
    new DocumentFragment(doc->NodeInfoManager());

  nsCOMPtr<nsINode> commonCloneAncestor = clonedFrag.get();

  
  

  RangeSubtreeIterator iter;

  aRv = iter.Init(this);
  if (aRv.Failed()) {
    return nullptr;
  }

  if (iter.IsDone())
  {
    
    return clonedFrag.forget();
  }

  iter.First();

  
  
  
  
  
  
  
  
  

  while (!iter.IsDone())
  {
    nsCOMPtr<nsINode> node = iter.GetCurrentNode();
    bool deepClone = !node->IsElement() ||
                       (!(node == mEndParent && mEndOffset == 0) &&
                        !(node == mStartParent &&
                          mStartOffset ==
                            int32_t(node->AsElement()->GetChildCount())));

    

    nsCOMPtr<nsINode> clone = node->CloneNode(deepClone, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    
    
    
    
    

    nsCOMPtr<nsIDOMCharacterData> charData(do_QueryInterface(clone));

    if (charData)
    {
      if (node == mEndParent)
      {
        
        

        uint32_t dataLength = 0;
        aRv = charData->GetLength(&dataLength);
        if (aRv.Failed()) {
          return nullptr;
        }

        if (dataLength > (uint32_t)mEndOffset)
        {
          aRv = charData->DeleteData(mEndOffset, dataLength - mEndOffset);
          if (aRv.Failed()) {
            return nullptr;
          }
        }
      }

      if (node == mStartParent)
      {
        
        

        if (mStartOffset > 0)
        {
          aRv = charData->DeleteData(0, mStartOffset);
          if (aRv.Failed()) {
            return nullptr;
          }
        }
      }
    }

    

    nsCOMPtr<nsINode> closestAncestor, farthestAncestor;

    aRv = CloneParentsBetween(commonAncestor, node,
                              getter_AddRefs(closestAncestor),
                              getter_AddRefs(farthestAncestor));

    if (aRv.Failed()) {
      return nullptr;
    }

    

    if (farthestAncestor)
    {
      commonCloneAncestor->AppendChild(*farthestAncestor, aRv);

      if (aRv.Failed()) {
        return nullptr;
      }
    }

    

    nsCOMPtr<nsINode> cloneNode = do_QueryInterface(clone);
    if (closestAncestor)
    {
      
      

      closestAncestor->AppendChild(*cloneNode, aRv);
    }
    else
    {
      
      

      commonCloneAncestor->AppendChild(*cloneNode, aRv);
    }
    if (aRv.Failed()) {
      return nullptr;
    }

    
    

    iter.Next();

    if (iter.IsDone())
      break; 

    nsCOMPtr<nsINode> nextNode = iter.GetCurrentNode();
    if (!nextNode) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    
    commonAncestor = nsContentUtils::GetCommonAncestor(node, nextNode);

    if (!commonAncestor) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    

    while (node && node != commonAncestor)
    {
      node = node->GetParentNode();
      if (aRv.Failed()) {
        return nullptr;
      }

      if (!node) {
        aRv.Throw(NS_ERROR_FAILURE);
        return nullptr;
      }

      cloneNode = cloneNode->GetParentNode();
      if (!cloneNode) {
        aRv.Throw(NS_ERROR_FAILURE);
        return nullptr;
      }
    }

    commonCloneAncestor = cloneNode;
  }

  return clonedFrag.forget();
}

already_AddRefed<nsRange>
nsRange::CloneRange() const
{
  nsRefPtr<nsRange> range = new nsRange(mOwner);

  range->SetMaySpanAnonymousSubtrees(mMaySpanAnonymousSubtrees);

  range->DoSetRange(mStartParent, mStartOffset, mEndParent, mEndOffset, mRoot);

  return range.forget();
}

NS_IMETHODIMP
nsRange::CloneRange(nsIDOMRange** aReturn)
{
  *aReturn = CloneRange().take();
  return NS_OK;
}

NS_IMETHODIMP
nsRange::InsertNode(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  if (!node) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }

  ErrorResult rv;
  InsertNode(*node, rv);
  return rv.StealNSResult();
}

void
nsRange::InsertNode(nsINode& aNode, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNode)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  int32_t tStartOffset = StartOffset();

  nsCOMPtr<nsINode> tStartContainer = GetStartContainer(aRv);
  if (aRv.Failed()) {
    return;
  }

  
  nsCOMPtr<nsINode> referenceNode;
  nsCOMPtr<nsINode> referenceParentNode = tStartContainer;

  nsCOMPtr<nsIDOMText> startTextNode(do_QueryInterface(tStartContainer));
  nsCOMPtr<nsIDOMNodeList> tChildList;
  if (startTextNode) {
    referenceParentNode = tStartContainer->GetParentNode();
    if (!referenceParentNode) {
      aRv.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
      return;
    }

    nsCOMPtr<nsIDOMText> secondPart;
    aRv = startTextNode->SplitText(tStartOffset, getter_AddRefs(secondPart));
    if (aRv.Failed()) {
      return;
    }

    referenceNode = do_QueryInterface(secondPart);
  } else {
    aRv = tStartContainer->AsDOMNode()->GetChildNodes(getter_AddRefs(tChildList));
    if (aRv.Failed()) {
      return;
    }

    
    nsCOMPtr<nsIDOMNode> q;
    aRv = tChildList->Item(tStartOffset, getter_AddRefs(q));
    referenceNode = do_QueryInterface(q);
    if (aRv.Failed()) {
      return;
    }
  }

  
  
  
  int32_t newOffset;

  if (referenceNode) {
    newOffset = IndexOf(referenceNode);
  } else {
    uint32_t length;
    aRv = tChildList->GetLength(&length);
    if (aRv.Failed()) {
      return;
    }

    newOffset = length;
  }

  if (aNode.NodeType() == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    newOffset += aNode.GetChildCount();
  } else {
    newOffset++;
  }

  
  nsCOMPtr<nsINode> tResultNode;
  tResultNode = referenceParentNode->InsertBefore(aNode, referenceNode, aRv);
  if (aRv.Failed()) {
    return;
  }

  if (Collapsed()) {
    aRv = SetEnd(referenceParentNode, newOffset);
  }
}

NS_IMETHODIMP
nsRange::SurroundContents(nsIDOMNode* aNewParent)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNewParent);
  if (!node) {
    return NS_ERROR_DOM_NOT_OBJECT_ERR;
  }
  ErrorResult rv;
  SurroundContents(*node, rv);
  return rv.StealNSResult();
}

void
nsRange::SurroundContents(nsINode& aNewParent, ErrorResult& aRv)
{
  if (!nsContentUtils::CanCallerAccess(&aNewParent)) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  if (!mRoot) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  
  
  if (mStartParent != mEndParent) {
    bool startIsText = mStartParent->IsNodeOfType(nsINode::eTEXT);
    bool endIsText = mEndParent->IsNodeOfType(nsINode::eTEXT);
    nsINode* startGrandParent = mStartParent->GetParentNode();
    nsINode* endGrandParent = mEndParent->GetParentNode();
    if (!((startIsText && endIsText &&
           startGrandParent &&
           startGrandParent == endGrandParent) ||
          (startIsText &&
           startGrandParent &&
           startGrandParent == mEndParent) ||
          (endIsText &&
           endGrandParent &&
           endGrandParent == mStartParent))) {
      aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return;
    }
  }

  
  
  uint16_t nodeType = aNewParent.NodeType();
  if (nodeType == nsIDOMNode::DOCUMENT_NODE ||
      nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE ||
      nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    aRv.Throw(NS_ERROR_DOM_INVALID_NODE_TYPE_ERR);
    return;
  }

  

  nsRefPtr<DocumentFragment> docFrag = ExtractContents(aRv);

  if (aRv.Failed()) {
    return;
  }

  if (!docFrag) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  

  nsCOMPtr<nsINodeList> children = aNewParent.ChildNodes();
  if (!children) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  uint32_t numChildren = children->Length();

  while (numChildren)
  {
    nsCOMPtr<nsINode> child = children->Item(--numChildren);
    if (!child) {
      aRv.Throw(NS_ERROR_FAILURE);
      return;
    }

    aNewParent.RemoveChild(*child, aRv);
    if (aRv.Failed()) {
      return;
    }
  }

  

  InsertNode(aNewParent, aRv);
  if (aRv.Failed()) {
    return;
  }

  
  aNewParent.AppendChild(*docFrag, aRv);
  if (aRv.Failed()) {
    return;
  }

  

  SelectNode(aNewParent, aRv);
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

already_AddRefed<DocumentFragment>
nsRange::CreateContextualFragment(const nsAString& aFragment, ErrorResult& aRv)
{
  if (!mIsPositioned) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return nsContentUtils::CreateContextualFragment(mStartParent, aFragment,
                                                  false, aRv);
}

static void ExtractRectFromOffset(nsIFrame* aFrame,
                                  const int32_t aOffset, nsRect* aR, bool aKeepLeft,
                                  bool aClampToEdge)
{
  nsPoint point;
  aFrame->GetPointFromOffset(aOffset, &point);

  if (!aClampToEdge && !aR->Contains(point)) {
    aR->width = 0;
    aR->x = point.x;
    return;
  }

  if (aClampToEdge) {
    point = aR->ClampPoint(point);
  }

  if (aKeepLeft) {
    aR->width = point.x - aR->x;
  } else {
    aR->width = aR->XMost() - point.x;
    aR->x = point.x;
  }
}

static nsTextFrame*
GetTextFrameForContent(nsIContent* aContent, bool aFlushLayout)
{
  nsIPresShell* presShell = aContent->OwnerDoc()->GetShell();
  if (presShell) {
    presShell->FrameConstructor()->EnsureFrameForTextNode(
        static_cast<nsGenericDOMDataNode*>(aContent));

    if (aFlushLayout) {
      aContent->OwnerDoc()->FlushPendingNotifications(Flush_Layout);
    }

    nsIFrame* frame = aContent->GetPrimaryFrame();
    if (frame && frame->GetType() == nsGkAtoms::textFrame) {
      return static_cast<nsTextFrame*>(frame);
    }
  }
  return nullptr;
}

static nsresult GetPartialTextRect(nsLayoutUtils::RectCallback* aCallback,
                                   nsIContent* aContent, int32_t aStartOffset,
                                   int32_t aEndOffset, bool aClampToEdge,
                                   bool aFlushLayout)
{
  nsTextFrame* textFrame = GetTextFrameForContent(aContent, aFlushLayout);
  if (textFrame) {
    nsIFrame* relativeTo = nsLayoutUtils::GetContainingBlockForClientRect(textFrame);
    for (nsTextFrame* f = textFrame; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
      int32_t fstart = f->GetContentOffset(), fend = f->GetContentEnd();
      if (fend <= aStartOffset || fstart >= aEndOffset)
        continue;

      
      f->EnsureTextRun(nsTextFrame::eInflated);
      NS_ENSURE_TRUE(f->GetTextRun(nsTextFrame::eInflated), NS_ERROR_OUT_OF_MEMORY);
      bool rtl = f->GetTextRun(nsTextFrame::eInflated)->IsRightToLeft();
      nsRect r = f->GetRectRelativeToSelf();
      if (fstart < aStartOffset) {
        
        ExtractRectFromOffset(f, aStartOffset, &r, rtl, aClampToEdge);
      }
      if (fend > aEndOffset) {
        
        ExtractRectFromOffset(f, aEndOffset, &r, !rtl, aClampToEdge);
      }
      r = nsLayoutUtils::TransformFrameRectToAncestor(f, r, relativeTo);
      aCallback->AddRect(r);
    }
  }
  return NS_OK;
}

 void
nsRange::CollectClientRects(nsLayoutUtils::RectCallback* aCollector,
                            nsRange* aRange,
                            nsINode* aStartParent, int32_t aStartOffset,
                            nsINode* aEndParent, int32_t aEndOffset,
                            bool aClampToEdge, bool aFlushLayout)
{
  
  nsCOMPtr<nsINode> startContainer = aStartParent;
  nsCOMPtr<nsINode> endContainer = aEndParent;

  
  if (!aStartParent->IsInDoc()) {
    return;
  }

  if (aFlushLayout) {
    aStartParent->OwnerDoc()->FlushPendingNotifications(Flush_Layout);
    
    if (!aStartParent->IsInDoc()) {
      return;
    }
  }

  RangeSubtreeIterator iter;

  nsresult rv = iter.Init(aRange);
  if (NS_FAILED(rv)) return;

  if (iter.IsDone()) {
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(aStartParent);
    if (content && content->IsNodeOfType(nsINode::eTEXT)) {
      nsTextFrame* textFrame = GetTextFrameForContent(content, aFlushLayout);
      if (textFrame) {
        int32_t outOffset;
        nsIFrame* outFrame;
        textFrame->GetChildFrameContainingOffset(aStartOffset, false,
          &outOffset, &outFrame);
        if (outFrame) {
           nsIFrame* relativeTo =
             nsLayoutUtils::GetContainingBlockForClientRect(outFrame);
           nsRect r = outFrame->GetRectRelativeToSelf();
           ExtractRectFromOffset(outFrame, aStartOffset, &r, false, aClampToEdge);
           r.width = 0;
           r = nsLayoutUtils::TransformFrameRectToAncestor(outFrame, r, relativeTo);
           aCollector->AddRect(r);
        }
      }
    }
    return;
  }

  do {
    nsCOMPtr<nsINode> node = iter.GetCurrentNode();
    iter.Next();
    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    if (!content)
      continue;
    if (content->IsNodeOfType(nsINode::eTEXT)) {
       if (node == startContainer) {
         int32_t offset = startContainer == endContainer ?
           aEndOffset : content->GetText()->GetLength();
         GetPartialTextRect(aCollector, content, aStartOffset, offset,
                            aClampToEdge, aFlushLayout);
         continue;
       } else if (node == endContainer) {
         GetPartialTextRect(aCollector, content, 0, aEndOffset,
                            aClampToEdge, aFlushLayout);
         continue;
       }
    }

    nsIFrame* frame = content->GetPrimaryFrame();
    if (frame) {
      nsLayoutUtils::GetAllInFlowRects(frame,
        nsLayoutUtils::GetContainingBlockForClientRect(frame), aCollector,
        nsLayoutUtils::RECTS_ACCOUNT_FOR_TRANSFORMS);
    }
  } while (!iter.IsDone());
}

NS_IMETHODIMP
nsRange::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  *aResult = GetBoundingClientRect(true).take();
  return NS_OK;
}

already_AddRefed<DOMRect>
nsRange::GetBoundingClientRect(bool aClampToEdge, bool aFlushLayout)
{
  nsRefPtr<DOMRect> rect = new DOMRect(ToSupports(this));
  if (!mStartParent) {
    return rect.forget();
  }

  nsLayoutUtils::RectAccumulator accumulator;
  CollectClientRects(&accumulator, this, mStartParent, mStartOffset, 
    mEndParent, mEndOffset, aClampToEdge, aFlushLayout);

  nsRect r = accumulator.mResultRect.IsEmpty() ? accumulator.mFirstRect : 
    accumulator.mResultRect;
  rect->SetLayoutRect(r);
  return rect.forget();
}

NS_IMETHODIMP
nsRange::GetClientRects(nsIDOMClientRectList** aResult)
{
  *aResult = GetClientRects(true).take();
  return NS_OK;
}

already_AddRefed<DOMRectList>
nsRange::GetClientRects(bool aClampToEdge, bool aFlushLayout)
{
  if (!mStartParent) {
    return nullptr;
  }

  nsRefPtr<DOMRectList> rectList =
    new DOMRectList(static_cast<nsIDOMRange*>(this));

  nsLayoutUtils::RectListBuilder builder(rectList);

  CollectClientRects(&builder, this, mStartParent, mStartOffset, 
    mEndParent, mEndOffset, aClampToEdge, aFlushLayout);
  return rectList.forget();
}

NS_IMETHODIMP
nsRange::GetUsedFontFaces(nsIDOMFontFaceList** aResult)
{
  *aResult = nullptr;

  NS_ENSURE_TRUE(mStartParent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsINode> startContainer = do_QueryInterface(mStartParent);
  nsCOMPtr<nsINode> endContainer = do_QueryInterface(mEndParent);

  
  nsIDocument* doc = mStartParent->OwnerDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);
  doc->FlushPendingNotifications(Flush_Frames);

  
  NS_ENSURE_TRUE(mStartParent->IsInDoc(), NS_ERROR_UNEXPECTED);

  nsRefPtr<nsFontFaceList> fontFaceList = new nsFontFaceList();

  RangeSubtreeIterator iter;
  nsresult rv = iter.Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  while (!iter.IsDone()) {
    
    nsCOMPtr<nsINode> node = iter.GetCurrentNode();
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

 already_AddRefed<nsRange>
nsRange::Constructor(const GlobalObject& aGlobal,
                     ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window || !window->GetDoc()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return window->GetDoc()->CreateRange(aRv);
}

void
nsRange::ExcludeNonSelectableNodes(nsTArray<nsRefPtr<nsRange>>* aOutRanges)
{
  MOZ_ASSERT(mIsPositioned);
  MOZ_ASSERT(mEndParent);
  MOZ_ASSERT(mStartParent);

  nsRange* range = this;
  nsRefPtr<nsRange> newRange;
  while (range) {
    nsCOMPtr<nsIContentIterator> iter = NS_NewPreContentIterator();
    nsresult rv = iter->Init(range);
    if (NS_FAILED(rv)) {
      return;
    }

    bool added = false;
    bool seenSelectable = false;
    nsIContent* firstNonSelectableContent = nullptr;
    while (true) {
      ErrorResult err;
      nsINode* node = iter->GetCurrentNode();
      iter->Next();
      bool selectable = true;
      nsIContent* content =
        node && node->IsContent() ? node->AsContent() : nullptr;
      if (content) {
        nsIFrame* frame = content->GetPrimaryFrame();
        for (nsIContent* p = content; !frame && (p = p->GetParent()); ) {
          frame = p->GetPrimaryFrame();
        }
        if (frame) {
          frame->IsSelectable(&selectable, nullptr);
        }
      }

      if (!selectable) {
        if (!firstNonSelectableContent) {
          firstNonSelectableContent = content;
        }
        if (iter->IsDone() && seenSelectable) {
          
          
          range->SetEndBefore(*firstNonSelectableContent, err);
        }
      } else if (firstNonSelectableContent) {
        if (range == this && !seenSelectable) {
          
          
          range->SetStartBefore(*node, err);
          if (err.Failed()) {
            return;
          }
          break; 
        } else {
          
          nsINode* endParent = range->mEndParent;
          int32_t endOffset = range->mEndOffset;

          
          range->SetEndBefore(*firstNonSelectableContent, err);

          
          
          
          if (!added && !err.Failed()) {
            aOutRanges->AppendElement(range);
          }

          
          rv = CreateRange(node, 0, endParent, endOffset,
                           getter_AddRefs(newRange));
          if (NS_FAILED(rv) || newRange->Collapsed()) {
            newRange = nullptr;
          }
          range = newRange;
          break; 
        }
      } else {
        seenSelectable = true;
        if (!added) {
          added = true;
          aOutRanges->AppendElement(range);
        }
      }
      if (iter->IsDone()) {
        return;
      }
    }
  }
}
