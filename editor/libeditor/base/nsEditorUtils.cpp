





































#include "nsEditorUtils.h"
#include "nsIDOMDocument.h"
#include "nsIDOMRange.h"
#include "nsIContent.h"
#include "nsLayoutCID.h"


#include "nsIClipboardDragDropHooks.h"
#include "nsIClipboardDragDropHookList.h"
#include "nsISimpleEnumerator.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIInterfaceRequestorUtils.h"






nsAutoSelectionReset::nsAutoSelectionReset(nsISelection *aSel, nsEditor *aEd) : 
mSel(nsnull)
,mEd(nsnull)
{ 
  if (!aSel || !aEd) return;    
  if (aEd->ArePreservingSelection()) return;   
  mSel = do_QueryInterface(aSel);
  mEd = aEd;
  if (mSel)
  {
    mEd->PreserveSelectionAcrossActions(mSel);
  }
}

nsAutoSelectionReset::~nsAutoSelectionReset()
{
  if (mSel && mEd->ArePreservingSelection())   
  {
    mEd->RestorePreservedSelection(mSel);
  }
}

void
nsAutoSelectionReset::Abort()
{
  mEd->StopPreservingSelection();
}






nsDOMIterator::nsDOMIterator() :
mIter(nsnull)
{
}
    
nsDOMIterator::~nsDOMIterator()
{
}
    
nsresult
nsDOMIterator::Init(nsIDOMRange* aRange)
{
  nsresult res;
  mIter = do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &res);
  if (NS_FAILED(res)) return res;
  if (!mIter) return NS_ERROR_FAILURE;
  return mIter->Init(aRange);
}

nsresult
nsDOMIterator::Init(nsIDOMNode* aNode)
{
  nsresult res;
  mIter = do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &res);
  if (NS_FAILED(res)) return res;
  if (!mIter) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  return mIter->Init(content);
}

void
nsDOMIterator::ForEach(nsDomIterFunctor& functor) const
{
  nsCOMPtr<nsIDOMNode> node;
  
  
  while (!mIter->IsDone())
  {
    node = do_QueryInterface(mIter->GetCurrentNode());
    if (!node)
      return;

    functor(node);
    mIter->Next();
  }
}

nsresult
nsDOMIterator::AppendList(nsBoolDomIterFunctor& functor,
                          nsCOMArray<nsIDOMNode>& arrayOfNodes) const
{
  nsCOMPtr<nsIDOMNode> node;
  
  
  while (!mIter->IsDone())
  {
    node = do_QueryInterface(mIter->GetCurrentNode());
    if (!node)
      return NS_ERROR_NULL_POINTER;

    if (functor(node))
    {
      arrayOfNodes.AppendObject(node);
    }
    mIter->Next();
  }
  return NS_OK;
}

nsDOMSubtreeIterator::nsDOMSubtreeIterator()
{
}
    
nsDOMSubtreeIterator::~nsDOMSubtreeIterator()
{
}
    
nsresult
nsDOMSubtreeIterator::Init(nsIDOMRange* aRange)
{
  nsresult res;
  mIter = do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
  if (NS_FAILED(res)) return res;
  if (!mIter) return NS_ERROR_FAILURE;
  return mIter->Init(aRange);
}

nsresult
nsDOMSubtreeIterator::Init(nsIDOMNode* aNode)
{
  nsresult res;
  mIter = do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
  if (NS_FAILED(res)) return res;
  if (!mIter) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  return mIter->Init(content);
}





PRBool 
nsEditorUtils::IsDescendantOf(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 *aOffset) 
{
  if (!aNode && !aParent) return PR_FALSE;
  if (aNode == aParent) return PR_FALSE;
  
  nsCOMPtr<nsIDOMNode> parent, node = do_QueryInterface(aNode);
  nsresult res;
  
  do
  {
    res = node->GetParentNode(getter_AddRefs(parent));
    if (NS_FAILED(res)) return PR_FALSE;
    if (parent == aParent) 
    {
      if (aOffset)
      {
        nsCOMPtr<nsIContent> pCon(do_QueryInterface(parent));
        nsCOMPtr<nsIContent> cCon(do_QueryInterface(node));
        if (pCon)
        {
          *aOffset = pCon->IndexOf(cCon);
        }
      }
      return PR_TRUE;
    }
    node = parent;
  } while (parent);
  
  return PR_FALSE;
}

PRBool
nsEditorUtils::IsLeafNode(nsIDOMNode *aNode)
{
  PRBool hasChildren = PR_FALSE;
  if (aNode)
    aNode->HasChildNodes(&hasChildren);
  return !hasChildren;
}





nsresult
nsEditorHookUtils::GetHookEnumeratorFromDocument(nsIDOMDocument *aDoc,
                                                 nsISimpleEnumerator **aResult)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  if (!doc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> container = doc->GetContainer();
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
  nsCOMPtr<nsIClipboardDragDropHookList> hookObj = do_GetInterface(docShell);
  if (!hookObj) return NS_ERROR_FAILURE;

  return hookObj->GetHookEnumerator(aResult);
}

PRBool
nsEditorHookUtils::DoAllowDragHook(nsIDOMDocument *aDoc, nsIDOMEvent *aDragEvent)
{
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetHookEnumeratorFromDocument(aDoc, getter_AddRefs(enumerator));
  if (!enumerator)
    return PR_TRUE;

  PRBool hasMoreHooks = PR_FALSE;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks)) && hasMoreHooks)
  {
    nsCOMPtr<nsISupports> isupp;
    if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
      break;

    nsCOMPtr<nsIClipboardDragDropHooks> override = do_QueryInterface(isupp);
    if (override)
    {
      PRBool canDrag = PR_TRUE;
      nsresult hookres = override->AllowStartDrag(aDragEvent, &canDrag);
      NS_ASSERTION(NS_SUCCEEDED(hookres), "hook failure in AllowStartDrag");
      if (!canDrag)
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}

PRBool
nsEditorHookUtils::DoDragHook(nsIDOMDocument *aDoc, nsIDOMEvent *aEvent,
                              nsITransferable *aTrans)
{
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetHookEnumeratorFromDocument(aDoc, getter_AddRefs(enumerator));
  if (!enumerator)
    return PR_TRUE;

  PRBool hasMoreHooks = PR_FALSE;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks)) && hasMoreHooks)
  {
    nsCOMPtr<nsISupports> isupp;
    if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
      break;

    nsCOMPtr<nsIClipboardDragDropHooks> override = do_QueryInterface(isupp);
    if (override)
    {
      PRBool canInvokeDrag = PR_TRUE;
      nsresult hookResult = override->OnCopyOrDrag(aEvent, aTrans, &canInvokeDrag);
      NS_ASSERTION(NS_SUCCEEDED(hookResult), "hook failure in OnCopyOrDrag");
      if (!canInvokeDrag)
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}

PRBool
nsEditorHookUtils::DoAllowDropHook(nsIDOMDocument *aDoc, nsIDOMEvent *aEvent,   
                                   nsIDragSession *aSession)
{
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetHookEnumeratorFromDocument(aDoc, getter_AddRefs(enumerator));
  if (!enumerator)
    return PR_TRUE;

  PRBool hasMoreHooks = PR_FALSE;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks)) && hasMoreHooks)
  {
    nsCOMPtr<nsISupports> isupp;
    if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
      break;

    nsCOMPtr<nsIClipboardDragDropHooks> override = do_QueryInterface(isupp);
    if (override)
    {
      PRBool allowDrop = PR_TRUE;
      nsresult hookResult = override->AllowDrop(aEvent, aSession, &allowDrop);
      NS_ASSERTION(NS_SUCCEEDED(hookResult), "hook failure in AllowDrop");
      if (!allowDrop)
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}

PRBool
nsEditorHookUtils::DoInsertionHook(nsIDOMDocument *aDoc, nsIDOMEvent *aDropEvent,  
                                   nsITransferable *aTrans)
{
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetHookEnumeratorFromDocument(aDoc, getter_AddRefs(enumerator));
  if (!enumerator)
    return PR_TRUE;

  PRBool hasMoreHooks = PR_FALSE;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks)) && hasMoreHooks)
  {
    nsCOMPtr<nsISupports> isupp;
    if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
      break;

    nsCOMPtr<nsIClipboardDragDropHooks> override = do_QueryInterface(isupp);
    if (override)
    {
      PRBool doInsert = PR_TRUE;
      nsresult hookResult = override->OnPasteOrDrop(aDropEvent, aTrans, &doInsert);
      NS_ASSERTION(NS_SUCCEEDED(hookResult), "hook failure in OnPasteOrDrop");
      if (!doInsert)
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}
