




#include <stdio.h>                      

#include "InsertTextTxn.h"
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsIDOMCharacterData.h"        
#include "nsIEditor.h"                  
#include "nsISelection.h"               
#include "nsISupportsUtils.h"           
#include "nsITransaction.h"             

InsertTextTxn::InsertTextTxn()
  : EditTxn()
{
}

InsertTextTxn::~InsertTextTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(InsertTextTxn, EditTxn,
                                   mElement)

NS_IMPL_ADDREF_INHERITED(InsertTextTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(InsertTextTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(InsertTextTxn)
  if (aIID.Equals(InsertTextTxn::GetCID())) {
    *aInstancePtr = (void*)(InsertTextTxn*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  } else
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP InsertTextTxn::Init(nsIDOMCharacterData *aElement,
                                  uint32_t             aOffset,
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
  NS_ENSURE_TRUE(aElement && aEditor, NS_ERROR_NULL_POINTER);

  mElement = do_QueryInterface(aElement);
  mOffset = aOffset;
  mStringToInsert = aStringToInsert;
  mEditor = aEditor;
  return NS_OK;
}

NS_IMETHODIMP InsertTextTxn::DoTransaction(void)
{
  NS_ASSERTION(mElement && mEditor, "bad state");
  if (!mElement || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result = mElement->InsertData(mOffset, mStringToInsert);
  NS_ENSURE_SUCCESS(result, result);

  
  bool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(result, result);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
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
  NS_ASSERTION(mElement && mEditor, "bad state");
  if (!mElement || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  uint32_t length = mStringToInsert.Length();
  return mElement->DeleteData(mOffset, length);
}

NS_IMETHODIMP InsertTextTxn::Merge(nsITransaction *aTransaction, bool *aDidMerge)
{
  
  if (aDidMerge)
    *aDidMerge = false;
  nsresult result = NS_OK;
  if (aDidMerge && aTransaction)
  {
    
    
    InsertTextTxn *otherInsTxn = nullptr;
    aTransaction->QueryInterface(InsertTextTxn::GetCID(), (void **)&otherInsTxn);
    if (otherInsTxn)
    {
      if (IsSequentialInsert(otherInsTxn))
      {
        nsAutoString otherData;
        otherInsTxn->GetData(otherData);
        mStringToInsert += otherData;
        *aDidMerge = true;
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



NS_IMETHODIMP InsertTextTxn::GetData(nsString& aResult)
{
  aResult = mStringToInsert;
  return NS_OK;
}

bool InsertTextTxn::IsSequentialInsert(InsertTextTxn *aOtherTxn)
{
  NS_ASSERTION(aOtherTxn, "null param");
  if (aOtherTxn && aOtherTxn->mElement == mElement)
  {
    
    int32_t length = mStringToInsert.Length();
    if (aOtherTxn->mOffset == (mOffset + length))
      return true;
  }
  return false;
}
