




#include "DeleteRangeTxn.h"
#include "nsIDOMRange.h"
#include "nsIDOMNodeList.h"
#include "DeleteTextTxn.h"
#include "DeleteElementTxn.h"
#include "nsIContentIterator.h"
#include "nsIContent.h"
#include "nsComponentManagerUtils.h"

#include "mozilla/Util.h"

using namespace mozilla;

#ifdef NS_DEBUG
static bool gNoisy = false;
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

NS_IMPL_CYCLE_COLLECTION_CLASS(DeleteRangeTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DeleteRangeTxn,
                                                EditAggregateTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRange)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mStartParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEndParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCommonParent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DeleteRangeTxn,
                                                  EditAggregateTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRange)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mStartParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mEndParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCommonParent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DeleteRangeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditAggregateTxn)

NS_IMETHODIMP DeleteRangeTxn::Init(nsEditor *aEditor,
                                   nsIDOMRange *aRange,
                                   nsRangeUpdater *aRangeUpdater)
{
  NS_ASSERTION(aEditor && aRange, "bad state");
  if (!aEditor || !aRange) { return NS_ERROR_NOT_INITIALIZED; }

  mEditor = aEditor;
  mRange  = do_QueryInterface(aRange);
  mRangeUpdater = aRangeUpdater;
  
  DebugOnly<nsresult> result = aRange->GetStartContainer(getter_AddRefs(mStartParent));
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

#ifdef DEBUG
  {
    nsCOMPtr<nsINode> start = do_QueryInterface(mStartParent);
    MOZ_ASSERT(start);
    NS_ASSERTION(mStartOffset <= PRInt32(start->Length()), "bad start offset");

    nsCOMPtr<nsINode> end = do_QueryInterface(mEndParent);
    MOZ_ASSERT(end);
    NS_ASSERTION(mEndOffset <= PRInt32(end->Length()), "bad end offset");

    if (gNoisy) {
      printf ("DeleteRange: %d of %p to %d of %p\n", 
               mStartOffset, (void *)mStartParent, mEndOffset, (void *)mEndParent);
    }         
  }
#endif 

  return NS_OK;
}

NS_IMETHODIMP DeleteRangeTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Do Delete Range\n"); }
#endif

  NS_ENSURE_TRUE(mStartParent && mEndParent && mCommonParent && mEditor, NS_ERROR_NOT_INITIALIZED);

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

  NS_ENSURE_SUCCESS(result, result);
  
  
  bool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    
    
    
    
    NS_ENSURE_SUCCESS(result, NS_OK);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
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

  NS_ENSURE_TRUE(mStartParent && mEndParent && mCommonParent && mEditor, NS_ERROR_NOT_INITIALIZED);

  return EditAggregateTxn::UndoTransaction();
}

NS_IMETHODIMP DeleteRangeTxn::RedoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Redo Delete Range\n"); }
#endif

  NS_ENSURE_TRUE(mStartParent && mEndParent && mCommonParent && mEditor, NS_ERROR_NOT_INITIALIZED);

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
  
  nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(aStartParent);
  if (textNode) {
    
    nsRefPtr<DeleteTextTxn> txn = new DeleteTextTxn();

    PRInt32 numToDel;
    if (aStartOffset==aEndOffset)
      numToDel = 1;
    else
      numToDel = aEndOffset-aStartOffset;

    nsresult rv = txn->Init(mEditor, textNode, aStartOffset, numToDel, mRangeUpdater);
    NS_ENSURE_SUCCESS(rv, rv);

    AppendChild(txn);
    return NS_OK;
  }

  nsCOMPtr<nsINode> startParent = do_QueryInterface(aStartParent);
  NS_ENSURE_STATE(startParent);
  NS_ASSERTION(aEndOffset <= startParent->GetChildCount(), "bad aEndOffset");

  nsCOMPtr<nsIContent> child = startParent->GetChildAt(aStartOffset);
  NS_ENSURE_STATE(child);

  nsresult result = NS_OK;
  for (PRUint32 i = aStartOffset; i < aEndOffset; ++i) {
    nsRefPtr<DeleteElementTxn> txn = new DeleteElementTxn();
    result = txn->Init(mEditor, child->AsDOMNode(), mRangeUpdater);
    if (NS_SUCCEEDED(result))
      AppendChild(txn);

    child = child->GetNextSibling();
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
      nsRefPtr<DeleteTextTxn> txn = new DeleteTextTxn();
      NS_ENSURE_TRUE(txn, NS_ERROR_OUT_OF_MEMORY);

      result = txn->Init(mEditor, textNode, start, numToDelete, mRangeUpdater);
      if (NS_SUCCEEDED(result))
        AppendChild(txn);
    }
  }

  return result;
}

NS_IMETHODIMP DeleteRangeTxn::CreateTxnsToDeleteNodesBetween()
{
  nsCOMPtr<nsIContentIterator> iter = do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1");
  NS_ENSURE_TRUE(iter, NS_ERROR_NULL_POINTER);

  nsresult result = iter->Init(mRange);
  NS_ENSURE_SUCCESS(result, result);

  while (!iter->IsDone() && NS_SUCCEEDED(result))
  {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(iter->GetCurrentNode());
    NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

    nsRefPtr<DeleteElementTxn> txn = new DeleteElementTxn();
    NS_ENSURE_TRUE(txn, NS_ERROR_OUT_OF_MEMORY);

    result = txn->Init(mEditor, node, mRangeUpdater);
    if (NS_SUCCEEDED(result))
      AppendChild(txn);
    iter->Next();
  }
  return result;
}

