




































#include "InsertTextTxn.h"
#include "nsIDOMCharacterData.h"
#include "nsISelection.h"
#include "EditAggregateTxn.h"

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#endif

InsertTextTxn::InsertTextTxn()
  : EditTxn()
{
}

NS_IMETHODIMP InsertTextTxn::Init(nsIDOMCharacterData *aElement,
                                  PRUint32             aOffset,
                                  const nsAString     &aStringToInsert,
                                  nsIEditor           *aEditor)
{
#if 0
      nsAutoString text;
      aElement->GetData(text);
      printf("InsertTextTxn: Offset to insert at = %d. Text of the node to insert into:\n", aOffset);
      wprintf(text.get());
      printf("\n");
#endif

  NS_ASSERTION(aElement && aEditor, "bad args");
  if (!aElement || !aEditor) return NS_ERROR_NULL_POINTER;

  mElement = do_QueryInterface(aElement);
  mOffset = aOffset;
  mStringToInsert = aStringToInsert;
  mEditor = aEditor;
  return NS_OK;
}

NS_IMETHODIMP InsertTextTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Do Insert Text element = %p\n", mElement.get()); }
#endif

  NS_ASSERTION(mElement && mEditor, "bad state");
  if (!mElement || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result = mElement->InsertData(mOffset, mStringToInsert);
  if (NS_FAILED(result)) return result;

  
  PRBool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) return result;
    if (!selection) return NS_ERROR_NULL_POINTER;
    result = selection->Collapse(mElement, mOffset+mStringToInsert.Length());
    NS_ASSERTION((NS_SUCCEEDED(result)), "selection could not be collapsed after insert.");
  }
  else
  {
    
  }

  return result;
}

NS_IMETHODIMP InsertTextTxn::UndoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("Undo Insert Text element = %p\n", mElement.get()); }
#endif

  NS_ASSERTION(mElement && mEditor, "bad state");
  if (!mElement || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  PRUint32 length = mStringToInsert.Length();
  return mElement->DeleteData(mOffset, length);
}

NS_IMETHODIMP InsertTextTxn::Merge(nsITransaction *aTransaction, PRBool *aDidMerge)
{
  
  if (aDidMerge)
    *aDidMerge = PR_FALSE;
  nsresult result = NS_OK;
  if (aDidMerge && aTransaction)
  {
    
    
    InsertTextTxn *otherInsTxn = nsnull;
    aTransaction->QueryInterface(InsertTextTxn::GetCID(), (void **)&otherInsTxn);
    if (otherInsTxn)
    {
      if (IsSequentialInsert(otherInsTxn))
      {
        nsAutoString otherData;
        otherInsTxn->GetData(otherData);
        mStringToInsert += otherData;
        *aDidMerge = PR_TRUE;
#ifdef NS_DEBUG
        if (gNoisy) { printf("InsertTextTxn assimilated %p\n", aTransaction); }
#endif
      }
      NS_RELEASE(otherInsTxn);
    }
  }
  return result;
}

NS_IMETHODIMP InsertTextTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("InsertTextTxn: ");
  aString += mStringToInsert;
  return NS_OK;
}



NS_IMETHODIMP
InsertTextTxn::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (!aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(InsertTextTxn::GetCID())) {
    *aInstancePtr = (void*)(InsertTextTxn*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return (EditTxn::QueryInterface(aIID, aInstancePtr));
}



NS_IMETHODIMP InsertTextTxn::GetData(nsString& aResult)
{
  aResult = mStringToInsert;
  return NS_OK;
}

PRBool InsertTextTxn::IsSequentialInsert(InsertTextTxn *aOtherTxn)
{
  NS_ASSERTION(aOtherTxn, "null param");
  if (aOtherTxn && aOtherTxn->mElement == mElement)
  {
    
    PRInt32 length = mStringToInsert.Length();
    if (aOtherTxn->mOffset == (mOffset + length))
      return PR_TRUE;
  }
  return PR_FALSE;
}
