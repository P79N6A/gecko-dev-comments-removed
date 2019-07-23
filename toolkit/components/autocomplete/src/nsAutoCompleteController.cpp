








































#include "nsAutoCompleteController.h"
#ifdef MOZ_MORK
#include "nsAutoCompleteMdbResult.h"
#endif
#include "nsAutoCompleteSimpleResult.h"

#include "nsNetCID.h"
#include "nsIIOService.h"
#include "nsToolkitCompsCID.h"
#include "nsIServiceManager.h"
#include "nsIAtomService.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsITreeColumns.h"
#include "nsIGenericFactory.h"
#include "nsIObserverService.h"
#include "nsIDOMKeyEvent.h"

static const char *kAutoCompleteSearchCID = "@mozilla.org/autocomplete/search;1?name=";

NS_IMPL_ISUPPORTS4(nsAutoCompleteController, nsIAutoCompleteController,
                                             nsIAutoCompleteObserver,
                                             nsITimerCallback,
                                             nsITreeView)

nsAutoCompleteController::nsAutoCompleteController() :
  mEnterAfterSearch(0),
  mDefaultIndexCompleted(PR_FALSE),
  mBackspaced(PR_FALSE),
  mPopupClosedByCompositionStart(PR_FALSE),
  mIsIMEComposing(PR_FALSE),
  mIgnoreHandleText(PR_FALSE),
  mIsOpen(PR_FALSE),
  mSearchStatus(0),
  mRowCount(0),
  mSearchesOngoing(0),
  mFirstSearchResult(PR_FALSE)
{
  mSearches = do_CreateInstance("@mozilla.org/supports-array;1");
  mResults = do_CreateInstance("@mozilla.org/supports-array;1");
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
    if (mIsOpen)
      ClosePopup();
    mSearches->Clear();
  }

  mInput = aInput;

  
  if (!aInput)
    return NS_OK;

  nsAutoString newValue;
  aInput->GetTextValue(newValue);

  
  mTree = nsnull;

  
  mSearchString = newValue;
  mEnterAfterSearch = 0;
  mDefaultIndexCompleted = PR_FALSE;
  mBackspaced = PR_FALSE;
  mSearchStatus = nsIAutoCompleteController::STATUS_NONE;
  mRowCount = 0;
  mSearchesOngoing = 0;

  
  PRUint32 searchCount;
  aInput->GetSearchCount(&searchCount);
  mResults->SizeTo(searchCount);
  mSearches->SizeTo(searchCount);
  mMatchCounts.SetLength(searchCount);

  const char *searchCID = kAutoCompleteSearchCID;

  for (PRUint32 i = 0; i < searchCount; ++i) {
    
    nsCAutoString searchName;
    aInput->GetSearchAt(i, searchName);
    nsCAutoString cid(searchCID);
    cid.Append(searchName);

    
    nsCOMPtr<nsIAutoCompleteSearch> search = do_GetService(cid.get());
    if (search)
      mSearches->AppendElement(search);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::StartSearch(const nsAString &aSearchString)
{
  mSearchString = aSearchString;
  StartSearchTimer();
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleText(PRBool aIgnoreSelection)
{
  if (!mInput) {
    
    StopSearch();
    
    
    
    NS_ERROR("Called before attaching to the control or after detaching from the control");
    return NS_OK;
  }

  nsAutoString newValue;
  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  input->GetTextValue(newValue);

  
  
  
  
  
  
  
  
  
  if (mIgnoreHandleText) {
    mIgnoreHandleText = PR_FALSE;
    if (newValue.Equals(mSearchString))
      return NS_OK;
    NS_ERROR("Now is after composition end event. But the value was changed.");
  }

  
  StopSearch();

  if (!mInput) {
    
    
    
    return NS_OK;
  }

  PRBool disabled;
  input->GetDisableAutoComplete(&disabled);
  NS_ENSURE_TRUE(!disabled, NS_OK);

  
  if (newValue.Length() > 0 && newValue.Equals(mSearchString))
    return NS_OK;

  
  if (newValue.Length() < mSearchString.Length() &&
      Substring(mSearchString, 0, newValue.Length()).Equals(newValue))
  {
    
    ClearResults();
    mBackspaced = PR_TRUE;
  } else
    mBackspaced = PR_FALSE;

  if (mRowCount == 0)
    
    
    
    ClearResults();

  mSearchString = newValue;

  
  if (newValue.Length() == 0) {
    ClosePopup();
    return NS_OK;
  }

  if (aIgnoreSelection) {
    StartSearchTimer();
  } else {
    
  PRInt32 selectionStart;
  input->GetSelectionStart(&selectionStart);
  PRInt32 selectionEnd;
  input->GetSelectionEnd(&selectionEnd);

  if (selectionStart == selectionEnd && selectionStart == (PRInt32) mSearchString.Length())
    StartSearchTimer();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleEnter(PRBool aIsPopupSelection, PRBool *_retval)
{
  *_retval = PR_FALSE;
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

  
  
  
  
  if (mSearchStatus != nsIAutoCompleteController::STATUS_SEARCHING)
    ClearSearchTimer();
  EnterMatch(aIsPopupSelection);

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleEscape(PRBool *_retval)
{
  *_retval = PR_FALSE;
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
  NS_ENSURE_TRUE(!mIsIMEComposing, NS_OK);

  mPopupClosedByCompositionStart = PR_FALSE;
  mIsIMEComposing = PR_TRUE;

  if (!mInput)
    return NS_OK;

  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  PRBool disabled;
  input->GetDisableAutoComplete(&disabled);
  if (disabled)
    return NS_OK;

  
  StopSearch();

  PRBool isOpen;
  input->GetPopupOpen(&isOpen);
  if (isOpen)
    ClosePopup();
  mPopupClosedByCompositionStart = isOpen;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleEndComposition()
{
  NS_ENSURE_TRUE(mIsIMEComposing, NS_OK);

  mIsIMEComposing = PR_FALSE;
  PRBool forceOpenPopup = mPopupClosedByCompositionStart;
  mPopupClosedByCompositionStart = PR_FALSE;

  if (!mInput)
    return NS_OK;

  nsAutoString value;
  mInput->GetTextValue(value);
  SetSearchString(EmptyString());
  if (!value.IsEmpty()) {
    
    HandleText(PR_TRUE);
  } else if (forceOpenPopup) {
    PRBool cancel;
    HandleKeyNavigation(nsIDOMKeyEvent::DOM_VK_DOWN, &cancel);
  }
  
  
  mIgnoreHandleText = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::HandleTab()
{
  PRBool cancel;
  return HandleEnter(PR_FALSE, &cancel);
}

NS_IMETHODIMP
nsAutoCompleteController::HandleKeyNavigation(PRUint32 aKey, PRBool *_retval)
{
  
  *_retval = PR_FALSE;

  if (!mInput) {
    
    
    
    NS_ERROR("Called before attaching to the control or after detaching from the control");
    return NS_OK;
  }

  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  nsCOMPtr<nsIAutoCompletePopup> popup;
  input->GetPopup(getter_AddRefs(popup));
  NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);

  PRBool disabled;
  input->GetDisableAutoComplete(&disabled);
  NS_ENSURE_TRUE(!disabled, NS_OK);

  if (aKey == nsIDOMKeyEvent::DOM_VK_UP ||
      aKey == nsIDOMKeyEvent::DOM_VK_DOWN ||
      aKey == nsIDOMKeyEvent::DOM_VK_PAGE_UP ||
      aKey == nsIDOMKeyEvent::DOM_VK_PAGE_DOWN)
  {
    
    
    *_retval = PR_TRUE;

    PRBool isOpen;
    input->GetPopupOpen(&isOpen);
    if (isOpen) {
      PRBool reverse = aKey == nsIDOMKeyEvent::DOM_VK_UP ||
                      aKey == nsIDOMKeyEvent::DOM_VK_PAGE_UP ? PR_TRUE : PR_FALSE;
      PRBool page = aKey == nsIDOMKeyEvent::DOM_VK_PAGE_UP ||
                    aKey == nsIDOMKeyEvent::DOM_VK_PAGE_DOWN ? PR_TRUE : PR_FALSE;

      
      
      
      PRBool completeSelection;
      input->GetCompleteSelectedIndex(&completeSelection);

      
      popup->SelectBy(reverse, page);

      if (completeSelection)
      {
        PRInt32 selectedIndex;
        popup->GetSelectedIndex(&selectedIndex);
        if (selectedIndex >= 0) {
          
          nsAutoString value;
          if (NS_SUCCEEDED(GetResultValueAt(selectedIndex, PR_TRUE, value))) {
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
          *_retval = PR_FALSE;
      }
      else if (aKey == nsIDOMKeyEvent::DOM_VK_DOWN) {
        nsAutoString text;
        input->GetTextValue(text);
        input->GetSelectionStart(&start);
        input->GetSelectionEnd(&end);
        if (start != end || end < (PRInt32)text.Length())
          *_retval = PR_FALSE;
      }
#endif
      if (*_retval) {
        
        PRUint32 resultCount;
        mResults->Count(&resultCount);
        if (resultCount) {
          if (mRowCount) {
            OpenPopup();
          }
        } else
          StartSearchTimer();
      }
    }
  } else if (   aKey == nsIDOMKeyEvent::DOM_VK_LEFT
             || aKey == nsIDOMKeyEvent::DOM_VK_RIGHT
#ifndef XP_MACOSX
             || aKey == nsIDOMKeyEvent::DOM_VK_HOME
#endif
            )
  {
    
    PRBool isOpen;
    input->GetPopupOpen(&isOpen);
    if (isOpen) {
      PRInt32 selectedIndex;
      popup->GetSelectedIndex(&selectedIndex);
      if (selectedIndex >= 0) {
        
        nsAutoString value;
        if (NS_SUCCEEDED(GetResultValueAt(selectedIndex, PR_TRUE, value))) {
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
nsAutoCompleteController::HandleDelete(PRBool *_retval)
{
  *_retval = PR_FALSE;
  if (!mInput)
    return NS_OK;

  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  PRBool isOpen = PR_FALSE;
  input->GetPopupOpen(&isOpen);
  if (!isOpen || mRowCount <= 0) {
    
    HandleText(PR_FALSE);
    return NS_OK;
  }

  nsCOMPtr<nsIAutoCompletePopup> popup;
  input->GetPopup(getter_AddRefs(popup));

  PRInt32 index, searchIndex, rowIndex;
  popup->GetSelectedIndex(&index);
  RowIndexToSearch(index, &searchIndex, &rowIndex);
  NS_ENSURE_TRUE(searchIndex >= 0 && rowIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAutoCompleteResult> result;
  mResults->GetElementAt(searchIndex, getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  nsAutoString search;
  input->GetSearchParam(search);

  
  result->RemoveValueAt(rowIndex, PR_TRUE);
  --mRowCount;

  
  popup->SetSelectedIndex(-1);

  
  if (mTree)
    mTree->RowCountChanged(mRowCount, -1);

  
  if (index >= (PRInt32)mRowCount)
    index = mRowCount - 1;

  if (mRowCount > 0) {
    
    popup->SetSelectedIndex(index);

    
    nsAutoString value;
    if (NS_SUCCEEDED(GetResultValueAt(index, PR_TRUE, value))) {
      CompleteValue(value, PR_FALSE);

      
      *_retval = PR_TRUE;
    }

    
    popup->Invalidate();
  } else {
    
    
    ClearSearchTimer();
    ClosePopup();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetValueAt(PRInt32 aIndex, nsAString & _retval)
{
  GetResultValueAt(aIndex, PR_FALSE, _retval);

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetCommentAt(PRInt32 aIndex, nsAString & _retval)
{
  PRInt32 searchIndex;
  PRInt32 rowIndex;
  RowIndexToSearch(aIndex, &searchIndex, &rowIndex);
  NS_ENSURE_TRUE(searchIndex >= 0 && rowIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAutoCompleteResult> result;
  mResults->GetElementAt(searchIndex, getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  return result->GetCommentAt(rowIndex, _retval);
}

NS_IMETHODIMP
nsAutoCompleteController::GetStyleAt(PRInt32 aIndex, nsAString & _retval)
{
  PRInt32 searchIndex;
  PRInt32 rowIndex;
  RowIndexToSearch(aIndex, &searchIndex, &rowIndex);
  NS_ENSURE_TRUE(searchIndex >= 0 && rowIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAutoCompleteResult> result;
  mResults->GetElementAt(searchIndex, getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  return result->GetStyleAt(rowIndex, _retval);
}

NS_IMETHODIMP
nsAutoCompleteController::GetImageAt(PRInt32 aIndex, nsAString & _retval)
{
  PRInt32 searchIndex;
  PRInt32 rowIndex;
  RowIndexToSearch(aIndex, &searchIndex, &rowIndex);
  NS_ENSURE_TRUE(searchIndex >= 0 && rowIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAutoCompleteResult> result;
  mResults->GetElementAt(searchIndex, getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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
nsAutoCompleteController::OnSearchResult(nsIAutoCompleteSearch *aSearch, nsIAutoCompleteResult* aResult)
{
  
  PRUint32 count;
  mSearches->Count(&count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsIAutoCompleteSearch> search;
    mSearches->GetElementAt(i, getter_AddRefs(search));
    if (search == aSearch) {
      ProcessResult(i, aResult);
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsAutoCompleteController::Notify(nsITimer *timer)
{
  mTimer = nsnull;
  StartSearch();
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
  
  
  PRInt32 currentIndex;
  mSelection->GetCurrentIndex(&currentIndex);

  








  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::GetCellProperties(PRInt32 row, nsITreeColumn* col, nsISupportsArray* properties)
{
  GetRowProperties(row, properties);

  if (row >= 0) {
    nsAutoString className;
    GetStyleAt(row, className);
    if (!className.IsEmpty()) {
      nsCOMPtr<nsIAtomService> atomSvc = do_GetService("@mozilla.org/atom-service;1");
      nsCOMPtr<nsIAtom> atom;
      atomSvc->GetAtom(className.get(), getter_AddRefs(atom));
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
nsAutoCompleteController::IsContainer(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  NS_NOTREACHED("no container cells");
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsContainerEmpty(PRInt32 index, PRBool *_retval)
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
nsAutoCompleteController::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, PRBool *_retval)
{
  *_retval = PR_FALSE;
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
nsAutoCompleteController::IsEditable(PRInt32 row, nsITreeColumn* col, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsSelectable(PRInt32 row, nsITreeColumn* col, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsSeparator(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::IsSorted(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::CanDrop(PRInt32 index, PRInt32 orientation, PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::Drop(PRInt32 row, PRInt32 orientation)
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
    mIsOpen = PR_TRUE;
    return mInput->SetPopupOpen(PR_TRUE);
  }

  return NS_OK;
}

nsresult
nsAutoCompleteController::ClosePopup()
{
  if (!mInput) {
    return NS_OK;
  }

  PRBool isOpen;
  mInput->GetPopupOpen(&isOpen);
  if (!isOpen)
    return NS_OK;

  nsCOMPtr<nsIAutoCompletePopup> popup;
  mInput->GetPopup(getter_AddRefs(popup));
  NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);
  popup->SetSelectedIndex(-1);
  mIsOpen = PR_FALSE;
  return mInput->SetPopupOpen(PR_FALSE);
}

nsresult
nsAutoCompleteController::StartSearch()
{
  NS_ENSURE_STATE(mInput);
  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  mSearchStatus = nsIAutoCompleteController::STATUS_SEARCHING;
  mDefaultIndexCompleted = PR_FALSE;

  PRUint32 count;
  mSearches->Count(&count);
  mSearchesOngoing = count;
  mFirstSearchResult = PR_TRUE;

  
  input->OnSearchBegin();

  PRUint32 searchesFailed = 0;
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsIAutoCompleteSearch> search;
    mSearches->GetElementAt(i, getter_AddRefs(search));
    nsCOMPtr<nsIAutoCompleteResult> result;
    mResults->GetElementAt(i, getter_AddRefs(result));

    if (result) {
      PRUint16 searchResult;
      result->GetSearchResult(&searchResult);
      if (searchResult != nsIAutoCompleteResult::RESULT_SUCCESS &&
          searchResult != nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING)
        result = nsnull;
    }

    nsAutoString searchParam;
    nsresult rv = input->GetSearchParam(searchParam);
    if (NS_FAILED(rv))
        return rv;

    rv = search->StartSearch(mSearchString, searchParam, result, static_cast<nsIAutoCompleteObserver *>(this));
    if (NS_FAILED(rv)) {
      ++searchesFailed;
      --mSearchesOngoing;
    }
  }

  if (searchesFailed == count)
    PostSearchCleanup();

  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteController::StopSearch()
{
  
  ClearSearchTimer();

  
  if (mSearchStatus == nsIAutoCompleteController::STATUS_SEARCHING) {
    PRUint32 count;
    mSearches->Count(&count);

    for (PRUint32 i = 0; i < count; ++i) {
      nsCOMPtr<nsIAutoCompleteSearch> search;
      mSearches->GetElementAt(i, getter_AddRefs(search));
      search->StopSearch();
    }
    mSearchesOngoing = 0;
    
    
    PostSearchCleanup();
  }
  return NS_OK;
}

nsresult
nsAutoCompleteController::StartSearchTimer()
{
  
  
  
  if (mTimer || !mInput)
    return NS_OK;

  PRUint32 timeout;
  mInput->GetTimeout(&timeout);

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
nsAutoCompleteController::EnterMatch(PRBool aIsPopupSelection)
{
  
  
  if (mSearchStatus == nsIAutoCompleteController::STATUS_SEARCHING) {
    mEnterAfterSearch = aIsPopupSelection ? 2 : 1;
    return NS_OK;
  }
  mEnterAfterSearch = 0;

  nsCOMPtr<nsIAutoCompleteInput> input(mInput);
  nsCOMPtr<nsIAutoCompletePopup> popup;
  input->GetPopup(getter_AddRefs(popup));
  NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);

  PRBool forceComplete;
  input->GetForceComplete(&forceComplete);

  
  nsAutoString value;
  popup->GetOverrideValue(value);
  if (value.IsEmpty()) {
    PRBool completeSelection;
    input->GetCompleteSelectedIndex(&completeSelection);

    
    
    
    
    PRInt32 selectedIndex;
    popup->GetSelectedIndex(&selectedIndex);
    if (selectedIndex >= 0 && (!completeSelection || aIsPopupSelection))
      GetResultValueAt(selectedIndex, PR_TRUE, value);

    if (forceComplete && value.IsEmpty()) {
      
      
      PRUint32 count;
      mResults->Count(&count);
      for (PRUint32 i = 0; i < count; ++i) {
        nsCOMPtr<nsIAutoCompleteResult> result;
        mResults->GetElementAt(i, getter_AddRefs(result));

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
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_STATE(obsSvc);
  obsSvc->NotifyObservers(input, "autocomplete-will-enter-text", nsnull);

  if (!value.IsEmpty()) {
    input->SetTextValue(value);
    input->SelectTextRange(value.Length(), value.Length());
    mSearchString = value;
  }

  obsSvc->NotifyObservers(input, "autocomplete-did-enter-text", nsnull);
  ClosePopup();

  PRBool cancel;
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

  PRBool cancel = PR_FALSE;
  input->OnTextReverted(&cancel);

  if (!cancel) {
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService("@mozilla.org/observer-service;1");
    NS_ENSURE_STATE(obsSvc);
    obsSvc->NotifyObservers(input, "autocomplete-will-revert-text", nsnull);

    input->SetTextValue(oldValue);

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
    mFirstSearchResult = PR_FALSE;
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

  PRInt32 oldIndex = mResults->IndexOf(aResult);
  if (oldIndex == -1) {
    
    mResults->AppendElement(aResult);
    mMatchCounts.AppendElement(matchCount);
  }
  else {
    
    mResults->ReplaceElementAt(aResult, oldIndex);
    oldMatchCount = mMatchCounts[aSearchIndex];
    mMatchCounts[oldIndex] = matchCount;
  }

  PRUint32 oldRowCount = mRowCount;
  
  
  if (result == nsIAutoCompleteResult::RESULT_FAILURE) {
    nsAutoString error;
    aResult->GetErrorDescription(error);
    if (!error.IsEmpty()) {
      ++mRowCount;
      if (mTree)
        mTree->RowCountChanged(oldRowCount, 1);
    }
  } else if (result == nsIAutoCompleteResult::RESULT_SUCCESS ||
             result == nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING) {
    
    mRowCount += matchCount - oldMatchCount;

    if (mTree)
      mTree->RowCountChanged(oldRowCount, matchCount - oldMatchCount);

    
    CompleteDefaultIndex(aSearchIndex);
  }

  
  nsCOMPtr<nsIAutoCompletePopup> popup;
  input->GetPopup(getter_AddRefs(popup));
  NS_ENSURE_TRUE(popup != nsnull, NS_ERROR_FAILURE);
  popup->Invalidate();

  
  
  
  if (mRowCount)
    OpenPopup();
  else if (result != nsIAutoCompleteResult::RESULT_NOMATCH_ONGOING)
    ClosePopup();

  if (mSearchesOngoing == 0) {
    
    PostSearchCleanup();
  }
  else if (mEnterAfterSearch) {
    
    
    StopSearch();
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

  
  
  if (mEnterAfterSearch)
    EnterMatch(mEnterAfterSearch == 2);

  return NS_OK;
}

nsresult
nsAutoCompleteController::ClearResults()
{
  PRInt32 oldRowCount = mRowCount;
  mRowCount = 0;
  mResults->Clear();
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
nsAutoCompleteController::CompleteDefaultIndex(PRInt32 aSearchIndex)
{
  if (mDefaultIndexCompleted || mEnterAfterSearch || mBackspaced || mRowCount == 0 || mSearchString.Length() == 0)
    return NS_OK;

  PRBool shouldComplete;
  mInput->GetCompleteDefaultIndex(&shouldComplete);
  if (!shouldComplete)
    return NS_OK;

  nsCOMPtr<nsIAutoCompleteSearch> search;
  mSearches->GetElementAt(aSearchIndex, getter_AddRefs(search));
  nsCOMPtr<nsIAutoCompleteResult> result;
  mResults->GetElementAt(aSearchIndex, getter_AddRefs(result));
  NS_ENSURE_TRUE(result != nsnull, NS_ERROR_FAILURE);

  
  
  PRInt32 defaultIndex;
  result->GetDefaultIndex(&defaultIndex);
  NS_ENSURE_TRUE(defaultIndex >= 0, NS_OK);

  nsAutoString resultValue;
  result->GetValueAt(defaultIndex, resultValue);
  CompleteValue(resultValue, PR_TRUE);

  mDefaultIndexCompleted = PR_TRUE;

  return NS_OK;
}

nsresult
nsAutoCompleteController::CompleteValue(nsString &aValue,
                                        PRBool selectDifference)



{
  const PRInt32 mSearchStringLength = mSearchString.Length();
  PRInt32 endSelect = aValue.Length();  

  if (aValue.IsEmpty() ||
      StringBeginsWith(aValue, mSearchString,
                       nsCaseInsensitiveStringComparator())) {
    
    
    
    mInput->SetTextValue(aValue);
  } else {
    PRInt32 findIndex;  

    nsresult rv;
    nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString scheme;
    if (NS_SUCCEEDED(ios->ExtractScheme(NS_ConvertUTF16toUTF8(aValue), scheme))) {
      
      
      
      
      findIndex = 7; 

      if ((endSelect < findIndex + mSearchStringLength) ||
          !scheme.LowerCaseEqualsLiteral("http") ||
          !Substring(aValue, findIndex, mSearchStringLength).Equals(
            mSearchString, nsCaseInsensitiveStringComparator())) {
        return NS_OK;
      }
    } else {
      
      
      
      nsString::const_iterator iter, end;
      aValue.BeginReading(iter);
      aValue.EndReading(end);
      const nsString::const_iterator::pointer start = iter.get();
      ++iter;  

      FindInReadable(mSearchString, iter, end,
                     nsCaseInsensitiveStringComparator());

      findIndex = iter.get() - start;
    }

    mInput->SetTextValue(mSearchString +
                         Substring(aValue, mSearchStringLength + findIndex,
                                   endSelect));

    endSelect -= findIndex; 
  }

  mInput->SelectTextRange(selectDifference ?
                          mSearchStringLength : endSelect, endSelect);

  return NS_OK;
}

nsresult
nsAutoCompleteController::GetResultValueAt(PRInt32 aIndex, PRBool aValueOnly, nsAString & _retval)
{
  NS_ENSURE_TRUE(aIndex >= 0 && (PRUint32) aIndex < mRowCount, NS_ERROR_ILLEGAL_VALUE);

  PRInt32 searchIndex;
  PRInt32 rowIndex;
  RowIndexToSearch(aIndex, &searchIndex, &rowIndex);
  NS_ENSURE_TRUE(searchIndex >= 0 && rowIndex >= 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAutoCompleteResult> result;
  mResults->GetElementAt(searchIndex, getter_AddRefs(result));
  NS_ENSURE_TRUE(result != nsnull, NS_ERROR_FAILURE);

  PRUint16 searchResult;
  result->GetSearchResult(&searchResult);

  if (searchResult == nsIAutoCompleteResult::RESULT_FAILURE) {
    if (aValueOnly)
      return NS_ERROR_FAILURE;
    result->GetErrorDescription(_retval);
  } else if (searchResult == nsIAutoCompleteResult::RESULT_SUCCESS ||
             searchResult == nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING) {
    result->GetValueAt(rowIndex, _retval);
  }

  return NS_OK;
}






nsresult
nsAutoCompleteController::RowIndexToSearch(PRInt32 aRowIndex, PRInt32 *aSearchIndex, PRInt32 *aItemIndex)
{
  *aSearchIndex = -1;
  *aItemIndex = -1;

  PRUint32 count;
  mSearches->Count(&count);
  PRUint32 index = 0;

  
  
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsIAutoCompleteResult> result;
    mResults->GetElementAt(i, getter_AddRefs(result));
    if (!result)
      continue;

    PRUint16 searchResult;
    result->GetSearchResult(&searchResult);

    
    
    PRUint32 rowCount = 0;
    if (searchResult == nsIAutoCompleteResult::RESULT_SUCCESS ||
        searchResult == nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING) {
      result->GetMatchCount(&rowCount);
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
#ifdef MOZ_MORK
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteMdbResult)
#endif

static const nsModuleComponentInfo components[] =
{
  { "AutoComplete Controller",
    NS_AUTOCOMPLETECONTROLLER_CID,
    NS_AUTOCOMPLETECONTROLLER_CONTRACTID,
    nsAutoCompleteControllerConstructor },

  { "AutoComplete Simple Result",
    NS_AUTOCOMPLETESIMPLERESULT_CID,
    NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID,
    nsAutoCompleteSimpleResultConstructor },

#ifdef MOZ_MORK
  { "AutoComplete Mdb Result",
    NS_AUTOCOMPLETEMDBRESULT_CID,
    NS_AUTOCOMPLETEMDBRESULT_CONTRACTID,
    nsAutoCompleteMdbResultConstructor },
#endif
};

NS_IMPL_NSGETMODULE(tkAutoCompleteModule, components)
