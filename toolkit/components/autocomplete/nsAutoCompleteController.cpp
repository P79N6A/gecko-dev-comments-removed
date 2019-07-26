




#include "nsAutoCompleteController.h"
#include "nsAutoCompleteSimpleResult.h"

#include "nsAutoPtr.h"
#include "nsNetCID.h"
#include "nsIIOService.h"
#include "nsToolkitCompsCID.h"
#include "nsIServiceManager.h"
#include "nsIAtomService.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsITreeColumns.h"
#include "nsIObserverService.h"
#include "nsIDOMKeyEvent.h"
#include "mozilla/Services.h"
#include "mozilla/ModuleUtils.h"

static const char *kAutoCompleteSearchCID = "@mozilla.org/autocomplete/search;1?name=";

NS_IMPL_CYCLE_COLLECTION_CLASS(nsAutoCompleteController)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsAutoCompleteController)
  tmp->SetInput(nsnull);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsAutoCompleteController)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mInput)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mSearches)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mResults)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAutoCompleteController)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsAutoCompleteController)
NS_INTERFACE_TABLE_HEAD(nsAutoCompleteController)
  NS_INTERFACE_TABLE4(nsAutoCompleteController, nsIAutoCompleteController,
                      nsIAutoCompleteObserver, nsITimerCallback, nsITreeView)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsAutoCompleteController)
NS_INTERFACE_MAP_END

nsAutoCompleteController::nsAutoCompleteController() :
  mDefaultIndexCompleted(false),
  mBackspaced(false),
  mPopupClosedByCompositionStart(false),
  mCompositionState(eCompositionState_None),
  mSearchStatus(nsAutoCompleteController::STATUS_NONE),
  mRowCount(0),
  mSearchesOngoing(0),
  mSearchesFailed(0),
  mFirstSearchResult(false),
  mImmediateSearchesCount(0)
{
}

nsAutoCompleteController::~nsAutoCompleteController()
{
  SetInput(nsnull);
}




NS_IMETHODIMP
nsAutoCompleteController::GetSearchStatus(PRUint16 *aSearchStatus)
{
  *aSearchStatus = mSearchStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetMatchCount(PRUint32 *aMatchCount)
{
  *aMatchCount = mRowCount;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetInput(nsIAutoCompleteInput **aInput)
{
  *aInput = mInput;
  NS_IF_ADDREF(*aInput);
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::SetInput(nsIAutoCompleteInput *aInput)
{
  
  if (mInput == aInput)
    return NS_OK;

  
  if (mInput) {
    
    StopSearch();
    ClearResults();
    ClosePopup();
    mSearches.Clear();
  }

  mInput = aInput;

  
  if (!aInput)
    return NS_OK;

  nsAutoString newValue;
  aInput->GetTextValue(newValue);

  
  mTree = nsnull;

  
  mSearchString = newValue;
  mDefaultIndexCompleted = false;
  mBackspaced = false;
  mSearchStatus = nsIAutoCompleteController::STATUS_NONE;
  mRowCount = 0;
  mSearchesOngoing = 0;

  
  PRUint32 searchCount;
  aInput->GetSearchCount(&searchCount);
  mResults.SetCapacity(searchCount);
  mSearches.SetCapacity(searchCount);
  mMatchCounts.SetLength(searchCount);
  mImmediateSearchesCount = 0;

  const char *searchCID = kAutoCompleteSearchCID;

  for (PRUint32 i = 0; i < searchCount; ++i) {
    
    nsCAutoString searchName;
    aInput->GetSearchAt(i, searchName);
    nsCAutoString cid(searchCID);
    cid.Append(searchName);

    
    nsCOMPtr<nsIAutoCompleteSearch> search = do_GetService(cid.get());
    if (search) {
      mSearches.AppendObject(search);

      
      PRUint16 searchType = nsIAutoCompleteSearchDescriptor::SEARCH_TYPE_DELAYED;
      nsCOMPtr<nsIAutoCompleteSearchDescriptor> searchDesc =
        do_QueryInterface(search);
      if (searchDesc && NS_SUCCEEDED(searchDesc->GetSearchType(&searchType)) &&
          searchType == nsIAutoCompleteSearchDescriptor::SEARCH_TYPE_IMMEDIATE)
        mImmediateSearchesCount++;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::StartSearch(const nsAString &aSearchString)
{
  mSearchString = aSearchString;
  StartSearches();
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleText()
{
  
  
  
  
  
  
  if (mCompositionState == eCompositionState_Composing) {
    return NS_OK;
  }

  bool handlingCompositionCommit =
    (mCompositionState == eCompositionState_Committing);
  bool popupClosedByCompositionStart = mPopupClosedByCompositionStart;
  if (handlingCompositionCommit) {
    mCompositionState = eCompositionState_None;
    mPopupClosedByCompositionStart = false;
  }

  if (!mInput) {
    
    StopSearch();
    
    
    
    NS_ERROR("Called before attaching to the control or after detaching from the control");
    return NS_OK;
  }

  nsAutoString newValue;
  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  input->GetTextValue(newValue);

  
  StopSearch();

  if (!mInput) {
    
    
    
    return NS_OK;
  }

  bool disabled;
  input->GetDisableAutoComplete(&disabled);
  NS_ENSURE_TRUE(!disabled, NS_OK);

  
  
  
  
  if (!handlingCompositionCommit && newValue.Length() > 0 &&
      newValue.Equals(mSearchString)) {
    return NS_OK;
  }

  
  if (newValue.Length() < mSearchString.Length() &&
      Substring(mSearchString, 0, newValue.Length()).Equals(newValue))
  {
    
    ClearResults();
    mBackspaced = true;
  } else
    mBackspaced = false;

  mSearchString = newValue;

  
  if (newValue.Length() == 0) {
    
    
    if (popupClosedByCompositionStart && handlingCompositionCommit) {
      bool cancel;
      HandleKeyNavigation(nsIDOMKeyEvent::DOM_VK_DOWN, &cancel);
      return NS_OK;
    }
    ClosePopup();
    return NS_OK;
  }

  StartSearches();

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleEnter(bool aIsPopupSelection, bool *_retval)
{
  *_retval = false;
  if (!mInput)
    return NS_OK;

  
  mInput->GetPopupOpen(_retval);
  if (*_retval) {
    nsCOMPtr<nsIAutoCompletePopup> popup;
    mInput->GetPopup(getter_AddRefs(popup));

    if (popup) {
      PRInt32 selectedIndex;
      popup->GetSelectedIndex(&selectedIndex);
      *_retval = selectedIndex >= 0;
    }
  }

  
  StopSearch();
  EnterMatch(aIsPopupSelection);

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleEscape(bool *_retval)
{
  *_retval = false;
  if (!mInput)
    return NS_OK;

  
  mInput->GetPopupOpen(_retval);

  
  StopSearch();
  ClearResults();
  RevertTextValue();
  ClosePopup();

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleStartComposition()
{
  NS_ENSURE_TRUE(mCompositionState != eCompositionState_Composing, NS_OK);

  mPopupClosedByCompositionStart = false;
  mCompositionState = eCompositionState_Composing;

  if (!mInput)
    return NS_OK;

  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  bool disabled;
  input->GetDisableAutoComplete(&disabled);
  if (disabled)
    return NS_OK;

  
  StopSearch();

  bool isOpen = false;
  input->GetPopupOpen(&isOpen);
  if (isOpen) {
    ClosePopup();

    bool stillOpen = false;
    input->GetPopupOpen(&stillOpen);
    mPopupClosedByCompositionStart = !stillOpen;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleEndComposition()
{
  NS_ENSURE_TRUE(mCompositionState == eCompositionState_Composing, NS_OK);

  
  
  
  
  
  mCompositionState = eCompositionState_Committing;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleTab()
{
  bool cancel;
  return HandleEnter(false, &cancel);
}

NS_IMETHODIMP
nsAutoCompleteController::HandleKeyNavigation(PRUint32 aKey, bool *_retval)
{
  
  *_retval = false;

  if (!mInput) {
    
    StopSearch();
    
    
    
    NS_ERROR("Called before attaching to the control or after detaching from the control");
    return NS_OK;
  }

  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  nsCOMPtr<nsIAutoCompletePopup> popup;
  input->GetPopup(getter_AddRefs(popup));
  NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);

  bool disabled;
  input->GetDisableAutoComplete(&disabled);
  NS_ENSURE_TRUE(!disabled, NS_OK);

  if (aKey == nsIDOMKeyEvent::DOM_VK_UP ||
      aKey == nsIDOMKeyEvent::DOM_VK_DOWN ||
      aKey == nsIDOMKeyEvent::DOM_VK_PAGE_UP ||
      aKey == nsIDOMKeyEvent::DOM_VK_PAGE_DOWN)
  {
    
    
    *_retval = true;

    bool isOpen = false;
    input->GetPopupOpen(&isOpen);
    if (isOpen) {
      bool reverse = aKey == nsIDOMKeyEvent::DOM_VK_UP ||
                      aKey == nsIDOMKeyEvent::DOM_VK_PAGE_UP ? true : false;
      bool page = aKey == nsIDOMKeyEvent::DOM_VK_PAGE_UP ||
                    aKey == nsIDOMKeyEvent::DOM_VK_PAGE_DOWN ? true : false;

      
      
      
      bool completeSelection;
      input->GetCompleteSelectedIndex(&completeSelection);

      
      popup->SelectBy(reverse, page);

      if (completeSelection)
      {
        PRInt32 selectedIndex;
        popup->GetSelectedIndex(&selectedIndex);
        if (selectedIndex >= 0) {
          
          nsAutoString value;
          if (NS_SUCCEEDED(GetResultValueAt(selectedIndex, true, value))) {
            input->SetTextValue(value);
            input->SelectTextRange(value.Length(), value.Length());
          }
        } else {
          
          input->SetTextValue(mSearchString);
          input->SelectTextRange(mSearchString.Length(), mSearchString.Length());
        }
      }
    } else {
#ifdef XP_MACOSX
      
      
      
      
      PRInt32 start, end;
      if (aKey == nsIDOMKeyEvent::DOM_VK_UP) {
        input->GetSelectionStart(&start);
        input->GetSelectionEnd(&end);
        if (start > 0 || start != end)
          *_retval = false;
      }
      else if (aKey == nsIDOMKeyEvent::DOM_VK_DOWN) {
        nsAutoString text;
        input->GetTextValue(text);
        input->GetSelectionStart(&start);
        input->GetSelectionEnd(&end);
        if (start != end || end < (PRInt32)text.Length())
          *_retval = false;
      }
#endif
      if (*_retval) {
        
        if (mResults.Count() > 0) {
          if (mRowCount) {
            OpenPopup();
          }
        } else {
          
          StopSearch();

          if (!mInput) {
            
            
            
            return NS_OK;
          }

          StartSearches();
        }
      }
    }
  } else if (   aKey == nsIDOMKeyEvent::DOM_VK_LEFT
             || aKey == nsIDOMKeyEvent::DOM_VK_RIGHT
#ifndef XP_MACOSX
             || aKey == nsIDOMKeyEvent::DOM_VK_HOME
#endif
            )
  {
    
    bool isOpen = false;
    input->GetPopupOpen(&isOpen);
    if (isOpen) {
      PRInt32 selectedIndex;
      popup->GetSelectedIndex(&selectedIndex);
      bool shouldComplete;
      input->GetCompleteDefaultIndex(&shouldComplete);
      if (selectedIndex >= 0) {
        
        nsAutoString value;
        if (NS_SUCCEEDED(GetResultValueAt(selectedIndex, true, value))) {
          input->SetTextValue(value);
          input->SelectTextRange(value.Length(), value.Length());
        }
      }
      else if (shouldComplete) {
        
        
        
        
        
        nsAutoString value;
        nsAutoString inputValue;
        input->GetTextValue(inputValue);
        if (NS_SUCCEEDED(GetDefaultCompleteValue(-1, false, value)) &&
            value.Equals(inputValue, nsCaseInsensitiveStringComparator())) {
          input->SetTextValue(value);
          input->SelectTextRange(value.Length(), value.Length());
        }
      }
      
      ClearSearchTimer();
      ClosePopup();
    }
    
    
    
    nsAutoString value;
    input->GetTextValue(value);
    mSearchString = value;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleDelete(bool *_retval)
{
  *_retval = false;
  if (!mInput)
    return NS_OK;

  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  bool isOpen = false;
  input->GetPopupOpen(&isOpen);
  if (!isOpen || mRowCount <= 0) {
    
    HandleText();
    return NS_OK;
  }

  nsCOMPtr<nsIAutoCompletePopup> popup;
  input->GetPopup(getter_AddRefs(popup));

  PRInt32 index, searchIndex, rowIndex;
  popup->GetSelectedIndex(&index);
  RowIndexToSearch(index, &searchIndex, &rowIndex);
  NS_ENSURE_TRUE(searchIndex >= 0 && rowIndex >= 0, NS_ERROR_FAILURE);

  nsIAutoCompleteResult *result = mResults[searchIndex];
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  nsAutoString search;
  input->GetSearchParam(search);

  
  result->RemoveValueAt(rowIndex, true);
  --mRowCount;

  
  *_retval = true;

  
  popup->SetSelectedIndex(-1);

  
  if (mTree)
    mTree->RowCountChanged(mRowCount, -1);

  
  if (index >= (PRInt32)mRowCount)
    index = mRowCount - 1;

  if (mRowCount > 0) {
    
    popup->SetSelectedIndex(index);

    
    bool shouldComplete = false;
    mInput->GetCompleteDefaultIndex(&shouldComplete);
    if (shouldComplete) {
      nsAutoString value;
      if (NS_SUCCEEDED(GetResultValueAt(index, true, value))) {
        CompleteValue(value);
      }
    }

    
    popup->Invalidate();
  } else {
    
    
    ClearSearchTimer();
    ClosePopup();
  }

  return NS_OK;
}

nsresult 
nsAutoCompleteController::GetResultAt(PRInt32 aIndex, nsIAutoCompleteResult** aResult,
                                      PRInt32* aRowIndex)
{
  PRInt32 searchIndex;
  RowIndexToSearch(aIndex, &searchIndex, aRowIndex);
  NS_ENSURE_TRUE(searchIndex >= 0 && *aRowIndex >= 0, NS_ERROR_FAILURE);

  *aResult = mResults[searchIndex];
  NS_ENSURE_TRUE(*aResult, NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetValueAt(PRInt32 aIndex, nsAString & _retval)
{
  GetResultLabelAt(aIndex, false, _retval);

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetLabelAt(PRInt32 aIndex, nsAString & _retval)
{
  GetResultLabelAt(aIndex, false, _retval);

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetCommentAt(PRInt32 aIndex, nsAString & _retval)
{
  PRInt32 rowIndex;
  nsIAutoCompleteResult* result;
  nsresult rv = GetResultAt(aIndex, &result, &rowIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return result->GetCommentAt(rowIndex, _retval);
}

NS_IMETHODIMP
nsAutoCompleteController::GetStyleAt(PRInt32 aIndex, nsAString & _retval)
{
  PRInt32 rowIndex;
  nsIAutoCompleteResult* result;
  nsresult rv = GetResultAt(aIndex, &result, &rowIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return result->GetStyleAt(rowIndex, _retval);
}

NS_IMETHODIMP
nsAutoCompleteController::GetImageAt(PRInt32 aIndex, nsAString & _retval)
{
  PRInt32 rowIndex;
  nsIAutoCompleteResult* result;
  nsresult rv = GetResultAt(aIndex, &result, &rowIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return result->GetImageAt(rowIndex, _retval);
}

NS_IMETHODIMP
nsAutoCompleteController::SetSearchString(const nsAString &aSearchString)
{
  mSearchString = aSearchString;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetSearchString(nsAString &aSearchString)
{
  aSearchString = mSearchString;
  return NS_OK;
}





NS_IMETHODIMP
nsAutoCompleteController::OnUpdateSearchResult(nsIAutoCompleteSearch *aSearch, nsIAutoCompleteResult* aResult)
{
  ClearResults();
  return OnSearchResult(aSearch, aResult);
}

NS_IMETHODIMP
nsAutoCompleteController::OnSearchResult(nsIAutoCompleteSearch *aSearch, nsIAutoCompleteResult* aResult)
{
  
  PRUint32 count = mSearches.Count();
  for (PRUint32 i = 0; i < count; ++i) {
    if (mSearches[i] == aSearch) {
      ProcessResult(i, aResult);
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsAutoCompleteController::Notify(nsITimer *timer)
{
  mTimer = nsnull;

  if (mImmediateSearchesCount == 0) {
    
    
    nsresult rv = BeforeSearches();
    if (NS_FAILED(rv))
      return rv;
  }
  StartSearch(nsIAutoCompleteSearchDescriptor::SEARCH_TYPE_DELAYED);
  AfterSearches();
  return NS_OK;
}




NS_IMETHODIMP
nsAutoCompleteController::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = mRowCount;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetCellProperties(PRInt32 row, nsITreeColumn* col, nsISupportsArray* properties)
{
  if (row >= 0) {
    nsAutoString className;
    GetStyleAt(row, className);
    if (!className.IsEmpty()) {
      nsCOMPtr<nsIAtom> atom(do_GetAtom(className));
      properties->AppendElement(atom);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetColumnProperties(nsITreeColumn* col, nsISupportsArray* properties)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetImageSrc(PRInt32 row, nsITreeColumn* col, nsAString& _retval)
{
  const PRUnichar* colID;
  col->GetIdConst(&colID);

  if (NS_LITERAL_STRING("treecolAutoCompleteValue").Equals(colID))
    return GetImageAt(row, _retval);

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetProgressMode(PRInt32 row, nsITreeColumn* col, PRInt32* _retval)
{
  NS_NOTREACHED("tree has no progress cells");
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetCellValue(PRInt32 row, nsITreeColumn* col, nsAString& _retval)
{
  NS_NOTREACHED("all of our cells are text");
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetCellText(PRInt32 row, nsITreeColumn* col, nsAString& _retval)
{
  const PRUnichar* colID;
  col->GetIdConst(&colID);

  if (NS_LITERAL_STRING("treecolAutoCompleteValue").Equals(colID))
    GetValueAt(row, _retval);
  else if (NS_LITERAL_STRING("treecolAutoCompleteComment").Equals(colID))
    GetCommentAt(row, _retval);

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsContainer(PRInt32 index, bool *_retval)
{
  *_retval = false;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsContainerOpen(PRInt32 index, bool *_retval)
{
  NS_NOTREACHED("no container cells");
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsContainerEmpty(PRInt32 index, bool *_retval)
{
  NS_NOTREACHED("no container cells");
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  *_retval = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, bool *_retval)
{
  *_retval = false;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::ToggleOpenState(PRInt32 index)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::SetTree(nsITreeBoxObject *tree)
{
  mTree = tree;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetSelection(nsITreeSelection * *aSelection)
{
  *aSelection = mSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteController::SetSelection(nsITreeSelection * aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::SelectionChanged()
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::SetCellValue(PRInt32 row, nsITreeColumn* col, const nsAString& value)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::SetCellText(PRInt32 row, nsITreeColumn* col, const nsAString& value)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::CycleHeader(nsITreeColumn* col)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::CycleCell(PRInt32 row, nsITreeColumn* col)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsEditable(PRInt32 row, nsITreeColumn* col, bool *_retval)
{
  *_retval = false;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsSelectable(PRInt32 row, nsITreeColumn* col, bool *_retval)
{
  *_retval = false;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsSeparator(PRInt32 index, bool *_retval)
{
  *_retval = false;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsSorted(bool *_retval)
{
  *_retval = false;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::CanDrop(PRInt32 index, PRInt32 orientation,
                                  nsIDOMDataTransfer* dataTransfer, bool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::Drop(PRInt32 row, PRInt32 orientation, nsIDOMDataTransfer* dataTransfer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::PerformAction(const PRUnichar *action)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::PerformActionOnCell(const PRUnichar* action, PRInt32 row, nsITreeColumn* col)
{
  return NS_OK;
}




nsresult
nsAutoCompleteController::OpenPopup()
{
  PRUint32 minResults;
  mInput->GetMinResultsForPopup(&minResults);

  if (mRowCount >= minResults) {
    return mInput->SetPopupOpen(true);
  }

  return NS_OK;
}

nsresult
nsAutoCompleteController::ClosePopup()
{
  if (!mInput) {
    return NS_OK;
  }

  bool isOpen = false;
  mInput->GetPopupOpen(&isOpen);
  if (!isOpen)
    return NS_OK;

  nsCOMPtr<nsIAutoCompletePopup> popup;
  mInput->GetPopup(getter_AddRefs(popup));
  NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);
  popup->SetSelectedIndex(-1);
  return mInput->SetPopupOpen(false);
}

nsresult
nsAutoCompleteController::BeforeSearches()
{
  NS_ENSURE_STATE(mInput);

  mSearchStatus = nsIAutoCompleteController::STATUS_SEARCHING;
  mDefaultIndexCompleted = false;

  
  
  
  if (!mResultCache.AppendObjects(mResults)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mSearchesOngoing = mSearches.Count();
  mSearchesFailed = 0;
  mFirstSearchResult = true;

  
  mInput->OnSearchBegin();

  return NS_OK;
}

nsresult
nsAutoCompleteController::StartSearch(PRUint16 aSearchType)
{
  NS_ENSURE_STATE(mInput);
  nsCOMPtr<nsIAutoCompleteInput> input = mInput;

  for (PRInt32 i = 0; i < mSearches.Count(); ++i) {
    nsCOMPtr<nsIAutoCompleteSearch> search = mSearches[i];

    
    
    PRUint16 searchType = nsIAutoCompleteSearchDescriptor::SEARCH_TYPE_DELAYED;
    nsCOMPtr<nsIAutoCompleteSearchDescriptor> searchDesc =
      do_QueryInterface(search);
    if (searchDesc)
      searchDesc->GetSearchType(&searchType);
    if (searchType != aSearchType)
      continue;

    nsIAutoCompleteResult *result = mResultCache.SafeObjectAt(i);

    if (result) {
      PRUint16 searchResult;
      result->GetSearchResult(&searchResult);
      if (searchResult != nsIAutoCompleteResult::RESULT_SUCCESS &&
          searchResult != nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING &&
          searchResult != nsIAutoCompleteResult::RESULT_NOMATCH)
        result = nsnull;
    }

    nsAutoString searchParam;
    nsresult rv = input->GetSearchParam(searchParam);
    if (NS_FAILED(rv))
        return rv;

    rv = search->StartSearch(mSearchString, searchParam, result, static_cast<nsIAutoCompleteObserver *>(this));
    if (NS_FAILED(rv)) {
      ++mSearchesFailed;
      --mSearchesOngoing;
    }
    
    
    
    
    
    if (!mInput) {
      
      return NS_OK;
    }
  }

  return NS_OK;
}

void
nsAutoCompleteController::AfterSearches()
{
  mResultCache.Clear();
  if (mSearchesFailed == mSearches.Count())
    PostSearchCleanup();
}

NS_IMETHODIMP
nsAutoCompleteController::StopSearch()
{
  
  ClearSearchTimer();

  
  if (mSearchStatus == nsIAutoCompleteController::STATUS_SEARCHING) {
    PRUint32 count = mSearches.Count();

    for (PRUint32 i = 0; i < count; ++i) {
      nsCOMPtr<nsIAutoCompleteSearch> search = mSearches[i];
      search->StopSearch();
    }
    mSearchesOngoing = 0;
    
    
    PostSearchCleanup();
  }
  return NS_OK;
}

nsresult
nsAutoCompleteController::StartSearches()
{
  
  
  
  if (mTimer || !mInput)
    return NS_OK;

  
  PRUint32 timeout;
  mInput->GetTimeout(&timeout);

  PRUint32 immediateSearchesCount = mImmediateSearchesCount;
  if (timeout == 0) {
    
    immediateSearchesCount = mSearches.Count();
  }

  if (immediateSearchesCount > 0) {
    nsresult rv = BeforeSearches();
    if (NS_FAILED(rv))
      return rv;
    StartSearch(nsIAutoCompleteSearchDescriptor::SEARCH_TYPE_IMMEDIATE);

    if (mSearches.Count() == immediateSearchesCount) {
      
      
      
      StartSearch(nsIAutoCompleteSearchDescriptor::SEARCH_TYPE_DELAYED);

      
      AfterSearches();
      return NS_OK;
    }
  }

  MOZ_ASSERT(timeout > 0, "Trying to delay searches with a 0 timeout!");

  
  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_FAILED(rv))
      return rv;
  rv = mTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv))
      mTimer = nsnull;

  return rv;
}

nsresult
nsAutoCompleteController::ClearSearchTimer()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }
  return NS_OK;
}

nsresult
nsAutoCompleteController::EnterMatch(bool aIsPopupSelection)
{
  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  nsCOMPtr<nsIAutoCompletePopup> popup;
  input->GetPopup(getter_AddRefs(popup));
  NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);

  bool forceComplete;
  input->GetForceComplete(&forceComplete);

  
  nsAutoString value;
  popup->GetOverrideValue(value);
  if (value.IsEmpty()) {
    bool shouldComplete;
    mInput->GetCompleteDefaultIndex(&shouldComplete);
    bool completeSelection;
    input->GetCompleteSelectedIndex(&completeSelection);

    PRInt32 selectedIndex;
    popup->GetSelectedIndex(&selectedIndex);
    if (selectedIndex >= 0) {
      
      
      
      
      
      if (!completeSelection || aIsPopupSelection)
        GetResultValueAt(selectedIndex, true, value);
    }
    else if (shouldComplete) {
      
      
      
      
      
      nsAutoString defaultIndexValue;
      if (NS_SUCCEEDED(GetFinalDefaultCompleteValue(defaultIndexValue)))
        value = defaultIndexValue;
    }

    if (forceComplete && value.IsEmpty()) {
      
      
      PRUint32 count = mResults.Count();
      for (PRUint32 i = 0; i < count; ++i) {
        nsIAutoCompleteResult *result = mResults[i];

        if (result) {
          PRInt32 defaultIndex;
          result->GetDefaultIndex(&defaultIndex);
          if (defaultIndex >= 0) {
            result->GetValueAt(defaultIndex, value);
            break;
          }
        }
      }
    }
  }

  nsCOMPtr<nsIObserverService> obsSvc =
    mozilla::services::GetObserverService();
  NS_ENSURE_STATE(obsSvc);
  obsSvc->NotifyObservers(input, "autocomplete-will-enter-text", nsnull);

  if (!value.IsEmpty()) {
    input->SetTextValue(value);
    input->SelectTextRange(value.Length(), value.Length());
    mSearchString = value;
  }

  obsSvc->NotifyObservers(input, "autocomplete-did-enter-text", nsnull);
  ClosePopup();

  bool cancel;
  input->OnTextEntered(&cancel);

  return NS_OK;
}

nsresult
nsAutoCompleteController::RevertTextValue()
{
  
  
  
  if (!mInput)
    return NS_OK;

  nsAutoString oldValue(mSearchString);
  nsCOMPtr<nsIAutoCompleteInput> input(mInput);

  bool cancel = false;
  input->OnTextReverted(&cancel);

  if (!cancel) {
    nsCOMPtr<nsIObserverService> obsSvc =
      mozilla::services::GetObserverService();
    NS_ENSURE_STATE(obsSvc);
    obsSvc->NotifyObservers(input, "autocomplete-will-revert-text", nsnull);

    nsAutoString inputValue;
    input->GetTextValue(inputValue);
    
    
    if (!oldValue.Equals(inputValue)) {
      input->SetTextValue(oldValue);
    }

    obsSvc->NotifyObservers(input, "autocomplete-did-revert-text", nsnull);
  }

  return NS_OK;
}

nsresult
nsAutoCompleteController::ProcessResult(PRInt32 aSearchIndex, nsIAutoCompleteResult *aResult)
{
  NS_ENSURE_STATE(mInput);
  nsCOMPtr<nsIAutoCompleteInput> input(mInput);

  
  
  if (mFirstSearchResult) {
    ClearResults();
    mFirstSearchResult = false;
  }

  PRUint16 result = 0;
  if (aResult)
    aResult->GetSearchResult(&result);

  
  if (result != nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING &&
      result != nsIAutoCompleteResult::RESULT_NOMATCH_ONGOING) {
    --mSearchesOngoing;
  }

  PRUint32 oldMatchCount = 0;
  PRUint32 matchCount = 0;
  if (aResult)
    aResult->GetMatchCount(&matchCount);

  PRInt32 resultIndex = mResults.IndexOf(aResult);
  if (resultIndex == -1) {
    
    mResults.AppendObject(aResult);
    mMatchCounts.AppendElement(matchCount);
    resultIndex = mResults.Count() - 1;
  }
  else {
    oldMatchCount = mMatchCounts[aSearchIndex];
    mMatchCounts[resultIndex] = matchCount;
  }

  bool isTypeAheadResult = false;
  if (aResult) {
    aResult->GetTypeAheadResult(&isTypeAheadResult);
  }

  if (!isTypeAheadResult) {
    PRUint32 oldRowCount = mRowCount;
    
    
    if (result == nsIAutoCompleteResult::RESULT_FAILURE) {
      nsAutoString error;
      aResult->GetErrorDescription(error);
      if (!error.IsEmpty()) {
        ++mRowCount;
        if (mTree) {
          mTree->RowCountChanged(oldRowCount, 1);
        }
      }
    } else if (result == nsIAutoCompleteResult::RESULT_SUCCESS ||
               result == nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING) {
      
      mRowCount += matchCount - oldMatchCount;

      if (mTree) {
        mTree->RowCountChanged(oldRowCount, matchCount - oldMatchCount);
      }
    }

    
    nsCOMPtr<nsIAutoCompletePopup> popup;
    input->GetPopup(getter_AddRefs(popup));
    NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);
    popup->Invalidate();

    
    
    
    if (mRowCount) {
      OpenPopup();
    } else if (result != nsIAutoCompleteResult::RESULT_NOMATCH_ONGOING) {
      ClosePopup();
    }
  }

  if (result == nsIAutoCompleteResult::RESULT_SUCCESS ||
      result == nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING) {
    
    CompleteDefaultIndex(resultIndex);
  }

  if (mSearchesOngoing == 0) {
    
    PostSearchCleanup();
  }

  return NS_OK;
}

nsresult
nsAutoCompleteController::PostSearchCleanup()
{
  NS_ENSURE_STATE(mInput);
  nsCOMPtr<nsIAutoCompleteInput> input(mInput);

  PRUint32 minResults;
  mInput->GetMinResultsForPopup(&minResults);

  if (mRowCount || minResults == 0) {
    OpenPopup();
    if (mRowCount)
      mSearchStatus = nsIAutoCompleteController::STATUS_COMPLETE_MATCH;
    else
      mSearchStatus = nsIAutoCompleteController::STATUS_COMPLETE_NO_MATCH;
  } else {
    mSearchStatus = nsIAutoCompleteController::STATUS_COMPLETE_NO_MATCH;
    ClosePopup();
  }

  
  input->OnSearchComplete();

  return NS_OK;
}

nsresult
nsAutoCompleteController::ClearResults()
{
  PRInt32 oldRowCount = mRowCount;
  mRowCount = 0;
  mResults.Clear();
  mMatchCounts.Clear();
  if (oldRowCount != 0) {
    if (mTree)
      mTree->RowCountChanged(0, -oldRowCount);
    else if (mInput) {
      nsCOMPtr<nsIAutoCompletePopup> popup;
      mInput->GetPopup(getter_AddRefs(popup));
      NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);
      
      
      
      popup->SetSelectedIndex(-1);
    }
  }
  return NS_OK;
}

nsresult
nsAutoCompleteController::CompleteDefaultIndex(PRInt32 aResultIndex)
{
  if (mDefaultIndexCompleted || mBackspaced || mSearchString.Length() == 0 || !mInput)
    return NS_OK;

  PRInt32 selectionStart;
  mInput->GetSelectionStart(&selectionStart);
  PRInt32 selectionEnd;
  mInput->GetSelectionEnd(&selectionEnd);

  
  
  if (selectionEnd != selectionStart ||
      selectionEnd != (PRInt32)mSearchString.Length())
    return NS_OK;

  bool shouldComplete;
  mInput->GetCompleteDefaultIndex(&shouldComplete);
  if (!shouldComplete)
    return NS_OK;

  nsAutoString resultValue;
  if (NS_SUCCEEDED(GetDefaultCompleteValue(aResultIndex, true, resultValue)))
    CompleteValue(resultValue);

  mDefaultIndexCompleted = true;

  return NS_OK;
}

nsresult
nsAutoCompleteController::GetDefaultCompleteResult(PRInt32 aResultIndex,
                                                   nsIAutoCompleteResult** _result,
                                                   PRInt32* _defaultIndex)
{
  *_defaultIndex = -1;
  PRInt32 resultIndex = aResultIndex;

  
  for (PRInt32 i = 0; resultIndex < 0 && i < mResults.Count(); ++i) {
    nsIAutoCompleteResult *result = mResults[i];
    if (result &&
        NS_SUCCEEDED(result->GetDefaultIndex(_defaultIndex)) &&
        *_defaultIndex >= 0) {
      resultIndex = i;
    }
  }
  NS_ENSURE_TRUE(resultIndex >= 0, NS_ERROR_FAILURE);

  *_result = mResults.SafeObjectAt(resultIndex);
  NS_ENSURE_TRUE(*_result, NS_ERROR_FAILURE);

  if (*_defaultIndex < 0) {
    
    
    (*_result)->GetDefaultIndex(_defaultIndex);
  }

  if (*_defaultIndex < 0) {
    
    
    return NS_ERROR_FAILURE;
  }

  
  
  
  PRUint32 matchCount = 0;
  (*_result)->GetMatchCount(&matchCount);
  
  if ((PRUint32)(*_defaultIndex) >= matchCount) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsAutoCompleteController::GetDefaultCompleteValue(PRInt32 aResultIndex,
                                                  bool aPreserveCasing,
                                                  nsAString &_retval)
{
  nsIAutoCompleteResult *result;
  PRInt32 defaultIndex = -1;
  nsresult rv = GetDefaultCompleteResult(aResultIndex, &result, &defaultIndex);
  if (NS_FAILED(rv)) return rv;

  nsAutoString resultValue;
  result->GetValueAt(defaultIndex, resultValue);
  if (aPreserveCasing &&
      StringBeginsWith(resultValue, mSearchString,
                       nsCaseInsensitiveStringComparator())) {
    
    
    
    
    
    
    
    nsAutoString casedResultValue;
    casedResultValue.Assign(mSearchString);
    
    casedResultValue.Append(Substring(resultValue,
                                      mSearchString.Length(),
                                      resultValue.Length()));
    _retval = casedResultValue;
  }
  else
    _retval = resultValue;

  return NS_OK;
}

nsresult
nsAutoCompleteController::GetFinalDefaultCompleteValue(nsAString &_retval)
{
  nsIAutoCompleteResult *result;
  PRInt32 defaultIndex = -1;
  nsresult rv = GetDefaultCompleteResult(-1, &result, &defaultIndex);
  if (NS_FAILED(rv)) return rv;

  result->GetValueAt(defaultIndex, _retval);
  nsAutoString inputValue;
  mInput->GetTextValue(inputValue);
  if (!_retval.Equals(inputValue, nsCaseInsensitiveStringComparator())) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  
  bool isTypeAheadResult = false;
  nsAutoString commentValue;
  if (NS_SUCCEEDED(result->GetTypeAheadResult(&isTypeAheadResult)) &&
      isTypeAheadResult &&
      NS_SUCCEEDED(result->GetCommentAt(defaultIndex, commentValue)) &&
      !commentValue.IsEmpty()) {
    _retval = commentValue;
  }

  return NS_OK;
}

nsresult
nsAutoCompleteController::CompleteValue(nsString &aValue)



{
  const PRInt32 mSearchStringLength = mSearchString.Length();
  PRInt32 endSelect = aValue.Length();  

  if (aValue.IsEmpty() ||
      StringBeginsWith(aValue, mSearchString,
                       nsCaseInsensitiveStringComparator())) {
    
    
    
    mInput->SetTextValue(aValue);
  } else {
    nsresult rv;
    nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString scheme;
    if (NS_SUCCEEDED(ios->ExtractScheme(NS_ConvertUTF16toUTF8(aValue), scheme))) {
      
      
      
      
      const PRInt32 findIndex = 7; 

      if ((endSelect < findIndex + mSearchStringLength) ||
          !scheme.LowerCaseEqualsLiteral("http") ||
          !Substring(aValue, findIndex, mSearchStringLength).Equals(
            mSearchString, nsCaseInsensitiveStringComparator())) {
        return NS_OK;
      }

      mInput->SetTextValue(mSearchString +
                           Substring(aValue, mSearchStringLength + findIndex,
                                     endSelect));

      endSelect -= findIndex; 
    } else {
      
      
      
      mInput->SetTextValue(mSearchString + NS_LITERAL_STRING(" >> ") + aValue);

      endSelect = mSearchString.Length() + 4 + aValue.Length();
    }
  }

  mInput->SelectTextRange(mSearchStringLength, endSelect);

  return NS_OK;
}

nsresult
nsAutoCompleteController::GetResultLabelAt(PRInt32 aIndex, bool aValueOnly, nsAString & _retval)
{
  return GetResultValueLabelAt(aIndex, aValueOnly, false, _retval);
}

nsresult
nsAutoCompleteController::GetResultValueAt(PRInt32 aIndex, bool aValueOnly, nsAString & _retval)
{
  return GetResultValueLabelAt(aIndex, aValueOnly, true, _retval);
}

nsresult
nsAutoCompleteController::GetResultValueLabelAt(PRInt32 aIndex, bool aValueOnly,
                                                bool aGetValue, nsAString & _retval)
{
  NS_ENSURE_TRUE(aIndex >= 0 && (PRUint32) aIndex < mRowCount, NS_ERROR_ILLEGAL_VALUE);

  PRInt32 rowIndex;
  nsIAutoCompleteResult *result;
  nsresult rv = GetResultAt(aIndex, &result, &rowIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint16 searchResult;
  result->GetSearchResult(&searchResult);

  if (searchResult == nsIAutoCompleteResult::RESULT_FAILURE) {
    if (aValueOnly)
      return NS_ERROR_FAILURE;
    result->GetErrorDescription(_retval);
  } else if (searchResult == nsIAutoCompleteResult::RESULT_SUCCESS ||
             searchResult == nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING) {
    if (aGetValue)
      result->GetValueAt(rowIndex, _retval);
    else
      result->GetLabelAt(rowIndex, _retval);
  }

  return NS_OK;
}






nsresult
nsAutoCompleteController::RowIndexToSearch(PRInt32 aRowIndex, PRInt32 *aSearchIndex, PRInt32 *aItemIndex)
{
  *aSearchIndex = -1;
  *aItemIndex = -1;

  PRUint32 count = mSearches.Count();
  PRUint32 index = 0;

  
  
  for (PRUint32 i = 0; i < count; ++i) {
    nsIAutoCompleteResult *result = mResults.SafeObjectAt(i);
    if (!result)
      continue;

    PRUint32 rowCount = 0;

    
    bool isTypeAheadResult = false;
    result->GetTypeAheadResult(&isTypeAheadResult);

    if (!isTypeAheadResult) {
      PRUint16 searchResult;
      result->GetSearchResult(&searchResult);

      
      
      if (searchResult == nsIAutoCompleteResult::RESULT_SUCCESS ||
          searchResult == nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING) {
        result->GetMatchCount(&rowCount);
      }
    }

    
    
    
    if ((rowCount != 0) && (index + rowCount-1 >= (PRUint32) aRowIndex)) {
      *aSearchIndex = i;
      *aItemIndex = aRowIndex - index;
      return NS_OK;
    }

    
    
    index += rowCount;
  }

  return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteController)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteSimpleResult)

NS_DEFINE_NAMED_CID(NS_AUTOCOMPLETECONTROLLER_CID);
NS_DEFINE_NAMED_CID(NS_AUTOCOMPLETESIMPLERESULT_CID);

static const mozilla::Module::CIDEntry kAutoCompleteCIDs[] = {
  { &kNS_AUTOCOMPLETECONTROLLER_CID, false, NULL, nsAutoCompleteControllerConstructor },
  { &kNS_AUTOCOMPLETESIMPLERESULT_CID, false, NULL, nsAutoCompleteSimpleResultConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kAutoCompleteContracts[] = {
  { NS_AUTOCOMPLETECONTROLLER_CONTRACTID, &kNS_AUTOCOMPLETECONTROLLER_CID },
  { NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID, &kNS_AUTOCOMPLETESIMPLERESULT_CID },
  { NULL }
};

static const mozilla::Module kAutoCompleteModule = {
  mozilla::Module::kVersion,
  kAutoCompleteCIDs,
  kAutoCompleteContracts
};

NSMODULE_DEFN(tkAutoCompleteModule) = &kAutoCompleteModule;
