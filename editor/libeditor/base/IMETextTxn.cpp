




#include "IMETextTxn.h"
#include "mozilla/DebugOnly.h"          
#include "mozilla/mozalloc.h"           
#include "mozilla/TextEvents.h"         
#include "nsAString.h"                  
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsIDOMCharacterData.h"        
#include "nsIDOMRange.h"                
#include "nsIContent.h"                 
#include "nsIEditor.h"                  
#include "nsIPresShell.h"               
#include "nsISelection.h"               
#include "nsISelectionController.h"     
#include "nsISelectionPrivate.h"        
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsITransaction.h"             
#include "nsRange.h"                    
#include "nsString.h"                   

using namespace mozilla;



IMETextTxn::IMETextTxn()
  : EditTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(IMETextTxn, EditTxn,
                                   mElement)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(IMETextTxn)
  if (aIID.Equals(IMETextTxn::GetCID())) {
    *aInstancePtr = (void*)(IMETextTxn*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  } else
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP IMETextTxn::Init(nsIDOMCharacterData     *aElement,
                               uint32_t                 aOffset,
                               uint32_t                 aReplaceLength,
                               TextRangeArray          *aTextRangeArray,
                               const nsAString         &aStringToInsert,
                               nsIEditor               *aEditor)
{
  NS_ENSURE_ARG_POINTER(aElement);
  mElement = aElement;
  mOffset = aOffset;
  mReplaceLength = aReplaceLength;
  mStringToInsert = aStringToInsert;
  mEditor = aEditor;
  mRanges = aTextRangeArray;
  mFixed = false;
  return NS_OK;
}

NS_IMETHODIMP IMETextTxn::DoTransaction(void)
{

#ifdef DEBUG_IMETXN
  printf("Do IME Text element = %p replace = %d len = %d\n", mElement.get(), mReplaceLength, mStringToInsert.Length());
#endif

  nsCOMPtr<nsISelectionController> selCon;
  mEditor->GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);

  
  nsresult result;
  if (mReplaceLength == 0) {
    result = mElement->InsertData(mOffset, mStringToInsert);
  } else {
    result = mElement->ReplaceData(mOffset, mReplaceLength, mStringToInsert);
  }
  if (NS_SUCCEEDED(result)) {
    result = SetSelectionForRanges();
  }

  return result;
}

NS_IMETHODIMP IMETextTxn::UndoTransaction(void)
{
#ifdef DEBUG_IMETXN
  printf("Undo IME Text element = %p\n", mElement.get());
#endif

  nsCOMPtr<nsISelectionController> selCon;
  mEditor->GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);

  nsresult result = mElement->DeleteData(mOffset, mStringToInsert.Length());
  if (NS_SUCCEEDED(result))
  { 
    nsCOMPtr<nsISelection> selection;
    result = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
    if (NS_SUCCEEDED(result) && selection) {
      result = selection->Collapse(mElement, mOffset);
      NS_ASSERTION((NS_SUCCEEDED(result)), "selection could not be collapsed after undo of IME insert.");
    }
  }
  return result;
}

NS_IMETHODIMP IMETextTxn::Merge(nsITransaction *aTransaction, bool *aDidMerge)
{
  NS_ASSERTION(aDidMerge, "illegal vaule- null ptr- aDidMerge");
  NS_ASSERTION(aTransaction, "illegal vaule- null ptr- aTransaction");
  NS_ENSURE_TRUE(aDidMerge && aTransaction, NS_ERROR_NULL_POINTER);
    
#ifdef DEBUG_IMETXN
  printf("Merge IME Text element = %p\n", mElement.get());
#endif

  
  
  
  if (mFixed) {
    *aDidMerge = false;
    return NS_OK;
  }

  
  
  
  IMETextTxn*  otherTxn = nullptr;
  nsresult result = aTransaction->QueryInterface(IMETextTxn::GetCID(),(void**)&otherTxn);
  if (otherTxn && NS_SUCCEEDED(result))
  {
    
    
    
    mStringToInsert = otherTxn->mStringToInsert;
    mRanges = otherTxn->mRanges;
    *aDidMerge = true;
#ifdef DEBUG_IMETXN
    printf("IMETextTxn assimilated IMETextTxn:%p\n", aTransaction);
#endif
    NS_RELEASE(otherTxn);
    return NS_OK;
  }

  *aDidMerge = false;
  return NS_OK;
}

NS_IMETHODIMP IMETextTxn::MarkFixed(void)
{
  mFixed = true;
  return NS_OK;
}

NS_IMETHODIMP IMETextTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("IMETextTxn: ");
  aString += mStringToInsert;
  return NS_OK;
}


static SelectionType
ToSelectionType(uint32_t aTextRangeType)
{
  switch(aTextRangeType) {
    case NS_TEXTRANGE_RAWINPUT:
      return nsISelectionController::SELECTION_IME_RAWINPUT;
    case NS_TEXTRANGE_SELECTEDRAWTEXT:
      return nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT;
    case NS_TEXTRANGE_CONVERTEDTEXT:
      return nsISelectionController::SELECTION_IME_CONVERTEDTEXT;
    case NS_TEXTRANGE_SELECTEDCONVERTEDTEXT:
      return nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT;
    default:
      MOZ_CRASH("Selection type is invalid");
      return nsISelectionController::SELECTION_NORMAL;
  }
}

nsresult
IMETextTxn::SetSelectionForRanges()
{
  nsCOMPtr<nsISelectionController> selCon;
  mEditor->GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsISelection> selection;
  nsresult rv =
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                         getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
  rv = selPriv->StartBatchChanges();
  NS_ENSURE_SUCCESS(rv, rv);

  
  static const SelectionType kIMESelections[] = {
    nsISelectionController::SELECTION_IME_RAWINPUT,
    nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT,
    nsISelectionController::SELECTION_IME_CONVERTEDTEXT,
    nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT
  };
  for (uint32_t i = 0; i < ArrayLength(kIMESelections); ++i) {
    nsCOMPtr<nsISelection> selectionOfIME;
    if (NS_FAILED(selCon->GetSelection(kIMESelections[i],
                                       getter_AddRefs(selectionOfIME)))) {
      continue;
    }
    DebugOnly<nsresult> rv = selectionOfIME->RemoveAllRanges();
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "Failed to remove all ranges of IME selection");
  }

  
  bool setCaret = false;
  uint32_t countOfRanges = mRanges ? mRanges->Length() : 0;
  for (uint32_t i = 0; i < countOfRanges; ++i) {
    const TextRange& textRange = mRanges->ElementAt(i);

    
    
    if (textRange.mRangeType == NS_TEXTRANGE_CARETPOSITION) {
      NS_ASSERTION(!setCaret, "The ranges already has caret position");
      NS_ASSERTION(!textRange.Length(), "nsEditor doesn't support wide caret");
      
      
      rv = selection->Collapse(mElement, mOffset + textRange.mStartOffset);
      setCaret = setCaret || NS_SUCCEEDED(rv);
      NS_ASSERTION(setCaret, "Failed to collapse normal selection");
      continue;
    }

    
    if (!textRange.Length()) {
      NS_WARNING("Any clauses must not be empty");
      continue;
    }

    nsRefPtr<nsRange> clauseRange;
    rv = nsRange::CreateRange(mElement, mOffset + textRange.mStartOffset,
                              mElement, mOffset + textRange.mEndOffset,
                              getter_AddRefs(clauseRange));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create a DOM range for a clause of composition");
      break;
    }

    
    nsCOMPtr<nsISelection> selectionOfIME;
    rv = selCon->GetSelection(ToSelectionType(textRange.mRangeType),
                              getter_AddRefs(selectionOfIME));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to get IME selection");
      break;
    }

    rv = selectionOfIME->AddRange(clauseRange);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to add selection range for a clause of composition");
      break;
    }

    
    nsCOMPtr<nsISelectionPrivate> selectionOfIMEPriv =
                                    do_QueryInterface(selectionOfIME);
    if (!selectionOfIMEPriv) {
      NS_WARNING("Failed to get nsISelectionPrivate interface from selection");
      continue; 
    }
    rv = selectionOfIMEPriv->SetTextRangeStyle(clauseRange,
                                               textRange.mRangeStyle);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to set selection style");
      break; 
    }
  }

  
  
  if (!setCaret) {
    rv = selection->Collapse(mElement, mOffset + mStringToInsert.Length());
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "Failed to set caret at the end of composition string");
  }

  rv = selPriv->EndBatchChanges();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to end batch changes");

  return rv;
}

