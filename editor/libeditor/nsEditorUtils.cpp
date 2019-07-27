




#include "nsEditorUtils.h"

#include "mozilla/dom/Selection.h"
#include "nsCOMArray.h"
#include "nsComponentManagerUtils.h"
#include "nsError.h"
#include "nsIClipboardDragDropHookList.h"

#include "nsIClipboardDragDropHooks.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMDocument.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsINode.h"
#include "nsISimpleEnumerator.h"

class nsISupports;
class nsRange;

using namespace mozilla;
using namespace mozilla::dom;





nsAutoSelectionReset::nsAutoSelectionReset(Selection* aSel, nsEditor* aEd)
  : mSel(nullptr), mEd(nullptr)
{ 
  if (!aSel || !aEd) return;    
  if (aEd->ArePreservingSelection()) return;   
  mSel = aSel;
  mEd = aEd;
  if (mSel)
  {
    mEd->PreserveSelectionAcrossActions(mSel);
  }
}

nsAutoSelectionReset::~nsAutoSelectionReset()
{
  NS_ASSERTION(!mSel || mEd, "mEd should be non-null when mSel is");
  if (mSel && mEd->ArePreservingSelection())   
  {
    mEd->RestorePreservedSelection(mSel);
  }
}

void
nsAutoSelectionReset::Abort()
{
  NS_ASSERTION(!mSel || mEd, "mEd should be non-null when mSel is");
  if (mSel)
    mEd->StopPreservingSelection();
}






nsDOMIterator::nsDOMIterator(nsRange& aRange)
{
  MOZ_ASSERT(aRange.GetStartParent(), "Invalid range");
  mIter = NS_NewContentIterator();
  DebugOnly<nsresult> res = mIter->Init(&aRange);
  MOZ_ASSERT(NS_SUCCEEDED(res));
}

nsDOMIterator::nsDOMIterator(nsIDOMNode& aNode)
{
  mIter = NS_NewContentIterator();
  nsCOMPtr<nsINode> node = do_QueryInterface(&aNode);
  NS_ENSURE_TRUE(node, );
  DebugOnly<nsresult> res = mIter->Init(node);
  MOZ_ASSERT(NS_SUCCEEDED(res));
}

nsDOMIterator::nsDOMIterator()
{
}

nsDOMIterator::~nsDOMIterator()
{
}

void
nsDOMIterator::AppendList(nsBoolDomIterFunctor& functor,
                          nsTArray<nsCOMPtr<nsINode>>& arrayOfNodes) const
{
  
  for (; !mIter->IsDone(); mIter->Next()) {
    nsCOMPtr<nsINode> node = mIter->GetCurrentNode();

    if (functor(node)) {
      arrayOfNodes.AppendElement(node);
    }
  }
}

void
nsDOMIterator::AppendList(nsBoolDomIterFunctor& functor,
                          nsCOMArray<nsIDOMNode>& arrayOfNodes) const
{
  
  for (; !mIter->IsDone(); mIter->Next()) {
    nsCOMPtr<nsIDOMNode> node = mIter->GetCurrentNode()->AsDOMNode();

    if (functor(node)) {
      arrayOfNodes.AppendObject(node);
    }
  }
}

nsDOMSubtreeIterator::nsDOMSubtreeIterator(nsRange& aRange)
{
  mIter = NS_NewContentSubtreeIterator();
  DebugOnly<nsresult> res = mIter->Init(&aRange);
  MOZ_ASSERT(NS_SUCCEEDED(res));
}

nsDOMSubtreeIterator::~nsDOMSubtreeIterator()
{
}





bool
nsEditorUtils::IsDescendantOf(nsINode* aNode, nsINode* aParent, int32_t* aOffset)
{
  MOZ_ASSERT(aNode && aParent);
  if (aNode == aParent) {
    return false;
  }

  for (nsCOMPtr<nsINode> node = aNode; node; node = node->GetParentNode()) {
    if (node->GetParentNode() == aParent) {
      if (aOffset) {
        *aOffset = aParent->IndexOf(node);
      }
      return true;
    }
  }

  return false;
}

bool
nsEditorUtils::IsDescendantOf(nsIDOMNode* aNode, nsIDOMNode* aParent, int32_t* aOffset)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  NS_ENSURE_TRUE(node && parent, false);
  return IsDescendantOf(node, parent, aOffset);
}

bool
nsEditorUtils::IsLeafNode(nsIDOMNode *aNode)
{
  bool hasChildren = false;
  if (aNode)
    aNode->HasChildNodes(&hasChildren);
  return !hasChildren;
}





nsresult
nsEditorHookUtils::GetHookEnumeratorFromDocument(nsIDOMDocument *aDoc,
                                                 nsISimpleEnumerator **aResult)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShell> docShell = doc->GetDocShell();
  nsCOMPtr<nsIClipboardDragDropHookList> hookObj = do_GetInterface(docShell);
  NS_ENSURE_TRUE(hookObj, NS_ERROR_FAILURE);

  return hookObj->GetHookEnumerator(aResult);
}

bool
nsEditorHookUtils::DoInsertionHook(nsIDOMDocument *aDoc, nsIDOMEvent *aDropEvent,  
                                   nsITransferable *aTrans)
{
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetHookEnumeratorFromDocument(aDoc, getter_AddRefs(enumerator));
  NS_ENSURE_TRUE(enumerator, true);

  bool hasMoreHooks = false;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks)) && hasMoreHooks)
  {
    nsCOMPtr<nsISupports> isupp;
    if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
      break;

    nsCOMPtr<nsIClipboardDragDropHooks> override = do_QueryInterface(isupp);
    if (override)
    {
      bool doInsert = true;
#ifdef DEBUG
      nsresult hookResult =
#endif
      override->OnPasteOrDrop(aDropEvent, aTrans, &doInsert);
      NS_ASSERTION(NS_SUCCEEDED(hookResult), "hook failure in OnPasteOrDrop");
      NS_ENSURE_TRUE(doInsert, false);
    }
  }

  return true;
}
