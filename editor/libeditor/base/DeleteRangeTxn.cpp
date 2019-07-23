





































#include "DeleteRangeTxn.h"
#include "nsIDOMRange.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMNodeList.h"
#include "nsISelection.h"
#include "DeleteTextTxn.h"
#include "DeleteElementTxn.h"
#include "TransactionFactory.h"
#include "nsIContentIterator.h"
#include "nsIContent.h"
#include "nsComponentManagerUtils.h"

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#endif


DeleteRangeTxn::DeleteRangeTxn()
: EditAggregateTxn()
,mRange()
,mStartParent()
,mStartOffset(0)
,mEndParent()
,mCommonParent()
,mEndOffset(0)
,mEditor(nsnull)
,mRangeUpdater(nsnull)
{
}

NS_IMETHODIMP DeleteRangeTxn::Init(nsIEditor *aEditor, 
                                   nsIDOMRange *aRange,
                                   nsRangeUpdater *aRangeUpdater)
{
  NS_ASSERTION(aEditor && aRange, "bad state");
  if (!aEditor || !aRange) { return NS_ERROR_NOT_INITIALIZED; }

  mEditor = aEditor;
  mRange  = do_QueryInterface(aRange);
  mRangeUpdater = aRangeUpdater;
  
  nsresult result = aRange->GetStartContainer(getter_AddRefs(mStartParent));
  NS_ASSERTION((NS_SUCCEEDED(result)), "GetStartParent failed.");
  result = aRange->GetEndContainer(getter_AddRefs(mEndParent));
  NS_ASSERTION((NS_SUCCEEDED(result)), "GetEndParent failed.");
  result = aRange->GetStartOffset(&mStartOffset);
  NS_ASSERTION((NS_SUCCEEDED(result)), "GetStartOffset failed.");
  result = aRange->GetEndOffset(&mEndOffset);
  NS_ASSERTION((NS_SUCCEEDED(result)), "GetEndOffset failed.");
  result = aRange->GetCommonAncestorContainer(getter_AddRefs(mCommonParent));
  NS_ASSERTION((NS_SUCCEEDED(result)), "GetCommonParent failed.");

  if (!mEditor->IsModifiableNode(mStartParent)) {
    return NS_ERROR_FAILURE;
  }

  if (mStartParent!=mEndParent &&
      (!mEditor->IsModifiableNode(mEndParent) ||
       !mEditor->IsModifiableNode(mCommonParent)))
  {
      return NS_ERROR_FAILURE;
  }

#ifdef NS_DEBUG
  {
    PRUint32 count;
    nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(mStartParent);
    if (textNode)
      textNode->GetLength(&count);
    else
    {
      nsCOMPtr<nsIDOMNodeList> children;
      result = mStartParent->GetChildNodes(getter_AddRefs(children));
      NS_ASSERTION(((NS_SUCCEEDED(result)) && children), "bad start child list");
      children->GetLength(&count);
    }
    NS_ASSERTION(mStartOffset<=(PRInt32)count, "bad start offset");

    textNode = do_QueryInterface(mEndParent);
    if (textNode)
      textNode->GetLength(&count);
    else
    {
      nsCOMPtr<nsIDOMNodeList> children;
      result = mEndParent->GetChildNodes(getter_AddRefs(children));
      NS_ASSERTION(((NS_SUCCEEDED(result)) && children), "bad end child list");
      children->GetLength(&count);
    }
    NS_ASSERTION(mEndOffset<=(PRInt32)count, "bad end offset");

#ifdef NS_DEBUG
    if (gNoisy)
    {
      printf ("DeleteRange: %d of %p to %d of %p\n", 
               mStartOffset, (void *)mStartParent, mEndOffset, (void *)mEndParent);
    }         
#endif
  }
#endif
  return result;

}

NS_IMETHODIMP DeleteRangeTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Do Delete Range\n"); }
#endif

  if (!mStartParent || !mEndParent || !mCommonParent || !mEditor) 
    return NS_ERROR_NOT_INITIALIZED;

  nsresult result; 
  

  if (mStartParent==mEndParent)
  { 
    result = CreateTxnsToDeleteBetween(mStartParent, mStartOffset, mEndOffset);
  }
  else
  { 
    
    result = CreateTxnsToDeleteContent(mStartParent, mStartOffset, nsIEditor::eNext);
    if (NS_SUCCEEDED(result))
    {
      
      result = CreateTxnsToDeleteNodesBetween();
      if (NS_SUCCEEDED(result))
      {
        
        result = CreateTxnsToDeleteContent(mEndParent, mEndOffset, nsIEditor::ePrevious);
      }
    }
  }

  
  if (NS_SUCCEEDED(result)) {
    result = EditAggregateTxn::DoTransaction();
  }

  if (NS_FAILED(result)) return result;
  
  
  PRBool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) return result;
    if (!selection) return NS_ERROR_NULL_POINTER;
    result = selection->Collapse(mStartParent, mStartOffset);
  }
  else
  {
    
  }

  return result;
}

NS_IMETHODIMP DeleteRangeTxn::UndoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Undo Delete Range\n"); }
#endif

  if (!mStartParent || !mEndParent || !mCommonParent || !mEditor) 
    return NS_ERROR_NOT_INITIALIZED;

  return EditAggregateTxn::UndoTransaction();
}

NS_IMETHODIMP DeleteRangeTxn::RedoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Redo Delete Range\n"); }
#endif

  if (!mStartParent || !mEndParent || !mCommonParent || !mEditor) 
    return NS_ERROR_NOT_INITIALIZED;

  return EditAggregateTxn::RedoTransaction();
}

NS_IMETHODIMP DeleteRangeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("DeleteRangeTxn");
  return NS_OK;
}

NS_IMETHODIMP 
DeleteRangeTxn::CreateTxnsToDeleteBetween(nsIDOMNode *aStartParent, 
                                          PRUint32    aStartOffset, 
                                          PRUint32    aEndOffset)
{
  nsresult result;
  
  nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(aStartParent);
  if (textNode)
  { 
    DeleteTextTxn *txn;
    result = TransactionFactory::GetNewTransaction(DeleteTextTxn::GetCID(), (EditTxn **)&txn);
    if (NS_FAILED(result)) return result;
    if (!txn) return NS_ERROR_NULL_POINTER;

    PRInt32 numToDel;
    if (aStartOffset==aEndOffset)
      numToDel = 1;
    else
      numToDel = aEndOffset-aStartOffset;
    result = txn->Init(mEditor, textNode, aStartOffset, numToDel, mRangeUpdater);
    if (NS_SUCCEEDED(result))
      AppendChild(txn);
    NS_RELEASE(txn);
  }
  else
  {
    nsCOMPtr<nsIDOMNodeList> children;
    result = aStartParent->GetChildNodes(getter_AddRefs(children));
    if (NS_FAILED(result)) return result;
    if (!children) return NS_ERROR_NULL_POINTER;

#ifdef DEBUG
    PRUint32 childCount;
    children->GetLength(&childCount);
    NS_ASSERTION(aEndOffset<=childCount, "bad aEndOffset");
#endif
    PRUint32 i;
    for (i=aStartOffset; i<aEndOffset; i++)
    {
      nsCOMPtr<nsIDOMNode> child;
      result = children->Item(i, getter_AddRefs(child));
      if (NS_FAILED(result)) return result;
      if (!child) return NS_ERROR_NULL_POINTER;

      DeleteElementTxn *txn;
      result = TransactionFactory::GetNewTransaction(DeleteElementTxn::GetCID(), (EditTxn **)&txn);
      if (NS_FAILED(result)) return result;
      if (!txn) return NS_ERROR_NULL_POINTER;

      result = txn->Init(mEditor, child, mRangeUpdater);
      if (NS_SUCCEEDED(result))
        AppendChild(txn);
      NS_RELEASE(txn);
    }
  }
  return result;
}

NS_IMETHODIMP DeleteRangeTxn::CreateTxnsToDeleteContent(nsIDOMNode *aParent, 
                                                        PRUint32    aOffset, 
                                                        nsIEditor::EDirection aAction)
{
  nsresult result = NS_OK;
  
  nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(aParent);
  if (textNode)
  { 
    PRUint32 start, numToDelete;
    if (nsIEditor::eNext == aAction)
    {
      start=aOffset;
      textNode->GetLength(&numToDelete);
      numToDelete -= aOffset;
    }
    else
    {
      start=0;
      numToDelete=aOffset;
    }
    
    if (numToDelete)
    {
      DeleteTextTxn *txn;
      result = TransactionFactory::GetNewTransaction(DeleteTextTxn::GetCID(), (EditTxn **)&txn);
      if (NS_FAILED(result)) return result;
      if (!txn) return NS_ERROR_NULL_POINTER;

      result = txn->Init(mEditor, textNode, start, numToDelete, mRangeUpdater);
      if (NS_SUCCEEDED(result))
        AppendChild(txn);
      NS_RELEASE(txn);
    }
  }

  return result;
}

NS_IMETHODIMP DeleteRangeTxn::CreateTxnsToDeleteNodesBetween()
{
  nsCOMPtr<nsIContentIterator> iter = do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1");
  if (!iter) return NS_ERROR_NULL_POINTER;

  nsresult result = iter->Init(mRange);
  if (NS_FAILED(result)) return result;

  while (!iter->IsDone() && NS_SUCCEEDED(result))
  {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(iter->GetCurrentNode());
    if (!node)
      return NS_ERROR_NULL_POINTER;

    DeleteElementTxn *txn;
    result = TransactionFactory::GetNewTransaction(DeleteElementTxn::GetCID(), (EditTxn **)&txn);
    if (NS_FAILED(result)) return result;
    if (!txn) return NS_ERROR_NULL_POINTER;

    result = txn->Init(mEditor, node, mRangeUpdater);
    if (NS_SUCCEEDED(result))
      AppendChild(txn);
    NS_RELEASE(txn);
    iter->Next();
  }
  return result;
}

