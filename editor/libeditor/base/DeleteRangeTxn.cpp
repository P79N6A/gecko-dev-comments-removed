




#include "DeleteRangeTxn.h"
#include "DeleteTextTxn.h"
#include "DeleteElementTxn.h"
#include "nsIContentIterator.h"
#include "nsComponentManagerUtils.h"

#include "mozilla/Util.h"

using namespace mozilla;


DeleteRangeTxn::DeleteRangeTxn()
  : EditAggregateTxn(),
    mRange(),
    mEditor(nsnull),
    mRangeUpdater(nsnull)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DeleteRangeTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DeleteRangeTxn,
                                                EditAggregateTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRange)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DeleteRangeTxn,
                                                  EditAggregateTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mRange, nsIDOMRange)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DeleteRangeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditAggregateTxn)

nsresult
DeleteRangeTxn::Init(nsEditor* aEditor,
                     nsRange* aRange,
                     nsRangeUpdater* aRangeUpdater)
{
  MOZ_ASSERT(aEditor && aRange);

  mEditor = aEditor;
  mRange = aRange->CloneRange();
  mRangeUpdater = aRangeUpdater;

  NS_ENSURE_TRUE(mEditor->IsModifiableNode(mRange->GetStartParent()),
                 NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mEditor->IsModifiableNode(mRange->GetEndParent()),
                 NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mEditor->IsModifiableNode(mRange->GetCommonAncestor()),
                 NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
DeleteRangeTxn::DoTransaction()
{
  MOZ_ASSERT(mRange && mEditor);
  nsresult res;

  
  nsCOMPtr<nsINode> startParent = mRange->GetStartParent();
  PRInt32 startOffset = mRange->StartOffset();
  nsCOMPtr<nsINode> endParent = mRange->GetEndParent();
  PRInt32 endOffset = mRange->EndOffset();
  MOZ_ASSERT(startParent && endParent);

  if (startParent == endParent) {
    
    res = CreateTxnsToDeleteBetween(startParent, startOffset, endOffset);
    NS_ENSURE_SUCCESS(res, res);
  } else {
    
    
    res = CreateTxnsToDeleteContent(startParent, startOffset, nsIEditor::eNext);
    NS_ENSURE_SUCCESS(res, res);
    
    res = CreateTxnsToDeleteNodesBetween();
    NS_ENSURE_SUCCESS(res, res);
    
    res = CreateTxnsToDeleteContent(endParent, endOffset, nsIEditor::ePrevious);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  res = EditAggregateTxn::DoTransaction();
  NS_ENSURE_SUCCESS(res, res);

  
  bool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection) {
    nsRefPtr<Selection> selection = mEditor->GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    res = selection->Collapse(startParent, startOffset);
    NS_ENSURE_SUCCESS(res, res);
  }
  

  return NS_OK;
}

NS_IMETHODIMP
DeleteRangeTxn::UndoTransaction()
{
  MOZ_ASSERT(mRange && mEditor);

  return EditAggregateTxn::UndoTransaction();
}

NS_IMETHODIMP
DeleteRangeTxn::RedoTransaction()
{
  MOZ_ASSERT(mRange && mEditor);

  return EditAggregateTxn::RedoTransaction();
}

NS_IMETHODIMP
DeleteRangeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("DeleteRangeTxn");
  return NS_OK;
}

nsresult
DeleteRangeTxn::CreateTxnsToDeleteBetween(nsINode* aNode,
                                          PRInt32 aStartOffset,
                                          PRInt32 aEndOffset)
{
  
  if (aNode->IsNodeOfType(nsINode::eDATA_NODE)) {
    
    nsRefPtr<DeleteTextTxn> txn = new DeleteTextTxn();

    PRInt32 numToDel;
    if (aStartOffset == aEndOffset) {
      numToDel = 1;
    } else {
      numToDel = aEndOffset - aStartOffset;
    }

    nsCOMPtr<nsIDOMCharacterData> charDataNode = do_QueryInterface(aNode);
    nsresult res = txn->Init(mEditor, charDataNode, aStartOffset, numToDel,
                             mRangeUpdater);
    NS_ENSURE_SUCCESS(res, res);

    AppendChild(txn);
    return NS_OK;
  }

  nsCOMPtr<nsIContent> child = aNode->GetChildAt(aStartOffset);
  NS_ENSURE_STATE(child);

  nsresult res = NS_OK;
  for (PRInt32 i = aStartOffset; i < aEndOffset; ++i) {
    nsRefPtr<DeleteElementTxn> txn = new DeleteElementTxn();
    res = txn->Init(mEditor, child->AsDOMNode(), mRangeUpdater);
    if (NS_SUCCEEDED(res)) {
      AppendChild(txn);
    }

    child = child->GetNextSibling();
  }

  NS_ENSURE_SUCCESS(res, res);
  return NS_OK;
}

nsresult
DeleteRangeTxn::CreateTxnsToDeleteContent(nsINode* aNode,
                                          PRInt32 aOffset,
                                          nsIEditor::EDirection aAction)
{
  
  if (aNode->IsNodeOfType(nsINode::eDATA_NODE)) {
    
    PRUint32 start, numToDelete;
    if (nsIEditor::eNext == aAction) {
      start = aOffset;
      numToDelete = aNode->Length() - aOffset;
    } else {
      start = 0;
      numToDelete = aOffset;
    }

    if (numToDelete) {
      nsRefPtr<DeleteTextTxn> txn = new DeleteTextTxn();

      nsCOMPtr<nsIDOMCharacterData> charDataNode = do_QueryInterface(aNode);
      nsresult res = txn->Init(mEditor, charDataNode, start, numToDelete,
                               mRangeUpdater);
      NS_ENSURE_SUCCESS(res, res);

      AppendChild(txn);
    }
  }

  return NS_OK;
}

nsresult
DeleteRangeTxn::CreateTxnsToDeleteNodesBetween()
{
  nsCOMPtr<nsIContentIterator> iter = NS_NewContentSubtreeIterator();

  nsresult res = iter->Init(mRange);
  NS_ENSURE_SUCCESS(res, res);

  while (!iter->IsDone()) {
    nsCOMPtr<nsINode> node = iter->GetCurrentNode();
    NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

    nsRefPtr<DeleteElementTxn> txn = new DeleteElementTxn();

    res = txn->Init(mEditor, node->AsDOMNode(), mRangeUpdater);
    NS_ENSURE_SUCCESS(res, res);
    AppendChild(txn);

    iter->Next();
  }
  return NS_OK;
}
