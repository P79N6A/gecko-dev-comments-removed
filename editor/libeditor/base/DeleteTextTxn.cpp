




































#include "DeleteTextTxn.h"
#include "nsIDOMCharacterData.h"
#include "nsISelection.h"
#include "nsSelectionState.h"

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#endif

DeleteTextTxn::DeleteTextTxn()
: EditTxn()
,mEditor(nsnull)
,mElement()
,mOffset(0)
,mNumCharsToDelete(0)
,mRangeUpdater(nsnull)
{
}

NS_IMETHODIMP DeleteTextTxn::Init(nsIEditor *aEditor,
                                  nsIDOMCharacterData *aElement,
                                  PRUint32 aOffset,
                                  PRUint32 aNumCharsToDelete,
                                  nsRangeUpdater *aRangeUpdater)
{
  NS_ASSERTION(aEditor&&aElement, "bad arg");
  if (!aEditor || !aElement) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mElement = do_QueryInterface(aElement);
  mOffset = aOffset;
  mNumCharsToDelete = aNumCharsToDelete;
  NS_ASSERTION(0!=aNumCharsToDelete, "bad arg, numCharsToDelete");
  PRUint32 count;
  aElement->GetLength(&count);
  NS_ASSERTION(count>=aNumCharsToDelete, "bad arg, numCharsToDelete.  Not enough characters in node");
  NS_ASSERTION(count>=aOffset+aNumCharsToDelete, "bad arg, numCharsToDelete.  Not enough characters in node");
  mDeletedText.Truncate();
  mRangeUpdater = aRangeUpdater;
  return NS_OK;
}

NS_IMETHODIMP DeleteTextTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Do Delete Text\n"); }
#endif

  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }
  
  nsresult result = mElement->SubstringData(mOffset, mNumCharsToDelete, mDeletedText);
  NS_ASSERTION(NS_SUCCEEDED(result), "could not get text to delete.");
  result = mElement->DeleteData(mOffset, mNumCharsToDelete);
  if (NS_FAILED(result)) return result;

  if (mRangeUpdater) 
    mRangeUpdater->SelAdjDeleteText(mElement, mOffset, mNumCharsToDelete);

  
  PRBool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) return result;
    if (!selection) return NS_ERROR_NULL_POINTER;
    result = selection->Collapse(mElement, mOffset);
    NS_ASSERTION((NS_SUCCEEDED(result)), "selection could not be collapsed after undo of deletetext.");
  }
  else
  {
    
  }
  return result;
}



NS_IMETHODIMP DeleteTextTxn::UndoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Undo Delete Text\n"); }
#endif

  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  return mElement->InsertData(mOffset, mDeletedText);
}

NS_IMETHODIMP DeleteTextTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("DeleteTextTxn: ");
  aString += mDeletedText;
  return NS_OK;
}
