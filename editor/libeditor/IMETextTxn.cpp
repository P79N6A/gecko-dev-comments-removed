




#include "IMETextTxn.h"

#include "mozilla/dom/Selection.h"      
#include "mozilla/dom/Text.h"           
#include "nsAString.h"                  
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsIPresShell.h"               
#include "nsRange.h"                    
#include "nsQueryObject.h"              

using namespace mozilla;
using namespace mozilla::dom;

IMETextTxn::IMETextTxn(Text& aTextNode, uint32_t aOffset,
                       uint32_t aReplaceLength,
                       TextRangeArray* aTextRangeArray,
                       const nsAString& aStringToInsert,
                       nsEditor& aEditor)
  : EditTxn()
  , mTextNode(&aTextNode)
  , mOffset(aOffset)
  , mReplaceLength(aReplaceLength)
  , mRanges(aTextRangeArray)
  , mStringToInsert(aStringToInsert)
  , mEditor(aEditor)
  , mFixed(false)
{
}

IMETextTxn::~IMETextTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(IMETextTxn, EditTxn,
                                   mTextNode)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(IMETextTxn)
  if (aIID.Equals(NS_GET_IID(IMETextTxn))) {
    foundInterface = static_cast<nsITransaction*>(this);
  } else
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMPL_ADDREF_INHERITED(IMETextTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(IMETextTxn, EditTxn)

NS_IMETHODIMP
IMETextTxn::DoTransaction()
{
  
  nsCOMPtr<nsISelectionController> selCon;
  mEditor.GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);

  
  nsresult res;
  if (mReplaceLength == 0) {
    res = mTextNode->InsertData(mOffset, mStringToInsert);
  } else {
    res = mTextNode->ReplaceData(mOffset, mReplaceLength, mStringToInsert);
  }
  NS_ENSURE_SUCCESS(res, res);

  res = SetSelectionForRanges();
  NS_ENSURE_SUCCESS(res, res);

  return NS_OK;
}

NS_IMETHODIMP
IMETextTxn::UndoTransaction()
{
  
  
  nsRefPtr<Selection> selection = mEditor.GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);

  nsresult res = mTextNode->DeleteData(mOffset, mStringToInsert.Length());
  NS_ENSURE_SUCCESS(res, res);

  
  res = selection->Collapse(mTextNode, mOffset);
  NS_ASSERTION(NS_SUCCEEDED(res),
               "Selection could not be collapsed after undo of IME insert.");
  NS_ENSURE_SUCCESS(res, res);

  return NS_OK;
}

NS_IMETHODIMP
IMETextTxn::Merge(nsITransaction* aTransaction, bool* aDidMerge)
{
  NS_ENSURE_ARG_POINTER(aTransaction && aDidMerge);

  
  if (mFixed) {
    *aDidMerge = false;
    return NS_OK;
  }

  
  nsRefPtr<IMETextTxn> otherTxn = do_QueryObject(aTransaction);
  if (otherTxn) {
    
    mStringToInsert = otherTxn->mStringToInsert;
    mRanges = otherTxn->mRanges;
    *aDidMerge = true;
    return NS_OK;
  }

  *aDidMerge = false;
  return NS_OK;
}

void
IMETextTxn::MarkFixed()
{
  mFixed = true;
}

NS_IMETHODIMP
IMETextTxn::GetTxnDescription(nsAString& aString)
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
  nsRefPtr<Selection> selection = mEditor.GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = selection->StartBatchChanges();
  NS_ENSURE_SUCCESS(rv, rv);

  
  static const SelectionType kIMESelections[] = {
    nsISelectionController::SELECTION_IME_RAWINPUT,
    nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT,
    nsISelectionController::SELECTION_IME_CONVERTEDTEXT,
    nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT
  };

  nsCOMPtr<nsISelectionController> selCon;
  mEditor.GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);

  for (uint32_t i = 0; i < ArrayLength(kIMESelections); ++i) {
    nsCOMPtr<nsISelection> selectionOfIME;
    if (NS_FAILED(selCon->GetSelection(kIMESelections[i],
                                       getter_AddRefs(selectionOfIME)))) {
      continue;
    }
    rv = selectionOfIME->RemoveAllRanges();
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "Failed to remove all ranges of IME selection");
  }

  
  bool setCaret = false;
  uint32_t countOfRanges = mRanges ? mRanges->Length() : 0;

#ifdef DEBUG
  
  uint32_t maxOffset = mTextNode->Length();
#endif

  
  
  
  uint32_t insertedLength = mStringToInsert.Length();
  for (uint32_t i = 0; i < countOfRanges; ++i) {
    const TextRange& textRange = mRanges->ElementAt(i);

    
    
    if (textRange.mRangeType == NS_TEXTRANGE_CARETPOSITION) {
      NS_ASSERTION(!setCaret, "The ranges already has caret position");
      NS_ASSERTION(!textRange.Length(), "nsEditor doesn't support wide caret");
      int32_t caretOffset = static_cast<int32_t>(
        mOffset + std::min(textRange.mStartOffset, insertedLength));
      MOZ_ASSERT(caretOffset >= 0 &&
                 static_cast<uint32_t>(caretOffset) <= maxOffset);
      rv = selection->Collapse(mTextNode, caretOffset);
      setCaret = setCaret || NS_SUCCEEDED(rv);
      NS_ASSERTION(setCaret, "Failed to collapse normal selection");
      continue;
    }

    
    if (!textRange.Length()) {
      NS_WARNING("Any clauses must not be empty");
      continue;
    }

    nsRefPtr<nsRange> clauseRange;
    int32_t startOffset = static_cast<int32_t>(
      mOffset + std::min(textRange.mStartOffset, insertedLength));
    MOZ_ASSERT(startOffset >= 0 &&
               static_cast<uint32_t>(startOffset) <= maxOffset);
    int32_t endOffset = static_cast<int32_t>(
      mOffset + std::min(textRange.mEndOffset, insertedLength));
    MOZ_ASSERT(endOffset >= startOffset &&
               static_cast<uint32_t>(endOffset) <= maxOffset);
    rv = nsRange::CreateRange(mTextNode, startOffset,
                              mTextNode, endOffset,
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
    int32_t caretOffset = static_cast<int32_t>(mOffset + insertedLength);
    MOZ_ASSERT(caretOffset >= 0 &&
               static_cast<uint32_t>(caretOffset) <= maxOffset);
    rv = selection->Collapse(mTextNode, caretOffset);
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "Failed to set caret at the end of composition string");
  }

  rv = selection->EndBatchChanges();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to end batch changes");

  return rv;
}
