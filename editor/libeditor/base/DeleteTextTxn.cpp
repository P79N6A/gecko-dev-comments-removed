




#include "DeleteTextTxn.h"
#include "mozilla/Selection.h"
#include "nsSelectionState.h"
#include "nsEditor.h"

using namespace mozilla;

DeleteTextTxn::DeleteTextTxn() :
  EditTxn(),
  mEditor(nsnull),
  mCharData(),
  mOffset(0),
  mNumCharsToDelete(0),
  mRangeUpdater(nsnull)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DeleteTextTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DeleteTextTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCharData)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DeleteTextTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCharData)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DeleteTextTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP
DeleteTextTxn::Init(nsEditor* aEditor,
                    nsIDOMCharacterData* aCharData,
                    PRUint32 aOffset,
                    PRUint32 aNumCharsToDelete,
                    nsRangeUpdater* aRangeUpdater)
{
  MOZ_ASSERT(aEditor && aCharData);

  mEditor = aEditor;
  mCharData = aCharData;

  
  if (!mEditor->IsModifiableNode(mCharData)) {
    return NS_ERROR_FAILURE;
  }

  mOffset = aOffset;
  mNumCharsToDelete = aNumCharsToDelete;
#ifdef DEBUG
  PRUint32 length;
  mCharData->GetLength(&length);
  NS_ASSERTION(length >= aOffset + aNumCharsToDelete,
               "Trying to delete more characters than in node");
#endif
  mDeletedText.Truncate();
  mRangeUpdater = aRangeUpdater;
  return NS_OK;
}

NS_IMETHODIMP
DeleteTextTxn::DoTransaction()
{
  MOZ_ASSERT(mEditor && mCharData);

  
  nsresult res = mCharData->SubstringData(mOffset, mNumCharsToDelete,
                                          mDeletedText);
  MOZ_ASSERT(NS_SUCCEEDED(res));
  res = mCharData->DeleteData(mOffset, mNumCharsToDelete);
  NS_ENSURE_SUCCESS(res, res);

  if (mRangeUpdater) {
    mRangeUpdater->SelAdjDeleteText(mCharData, mOffset, mNumCharsToDelete);
  }

  
  bool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection) {
    nsRefPtr<Selection> selection = mEditor->GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    res = selection->Collapse(mCharData, mOffset);
    NS_ASSERTION(NS_SUCCEEDED(res),
                 "selection could not be collapsed after undo of deletetext.");
    NS_ENSURE_SUCCESS(res, res);
  }
  
  return NS_OK;
}



NS_IMETHODIMP
DeleteTextTxn::UndoTransaction()
{
  MOZ_ASSERT(mEditor && mCharData);

  return mCharData->InsertData(mOffset, mDeletedText);
}

NS_IMETHODIMP
DeleteTextTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("DeleteTextTxn: ");
  aString += mDeletedText;
  return NS_OK;
}
