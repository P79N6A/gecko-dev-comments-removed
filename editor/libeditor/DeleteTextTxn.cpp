




#include "DeleteTextTxn.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/Selection.h"
#include "nsAutoPtr.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsError.h"
#include "nsIEditor.h"
#include "nsISelection.h"
#include "nsISupportsImpl.h"
#include "nsSelectionState.h"
#include "nsAString.h"

using namespace mozilla;
using namespace mozilla::dom;

DeleteTextTxn::DeleteTextTxn() :
  EditTxn(),
  mEditor(nullptr),
  mCharData(),
  mOffset(0),
  mNumCharsToDelete(0),
  mRangeUpdater(nullptr)
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(DeleteTextTxn, EditTxn,
                                   mCharData)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DeleteTextTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP
DeleteTextTxn::Init(nsEditor* aEditor,
                    nsIDOMCharacterData* aCharData,
                    uint32_t aOffset,
                    uint32_t aNumCharsToDelete,
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
  uint32_t length;
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
