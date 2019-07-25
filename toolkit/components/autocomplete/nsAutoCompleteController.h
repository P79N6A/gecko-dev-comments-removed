





































#ifndef __nsAutoCompleteController__
#define __nsAutoCompleteController__

#include "nsIAutoCompleteController.h"

#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompletePopup.h"
#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteSearch.h"
#include "nsString.h"
#include "nsITreeView.h"
#include "nsITreeSelection.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"

class nsAutoCompleteController : public nsIAutoCompleteController,
                                 public nsIAutoCompleteObserver,
                                 public nsITimerCallback,
                                 public nsITreeView
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsAutoCompleteController,
                                           nsIAutoCompleteController)
  NS_DECL_NSIAUTOCOMPLETECONTROLLER
  NS_DECL_NSIAUTOCOMPLETEOBSERVER
  NS_DECL_NSITREEVIEW
  NS_DECL_NSITIMERCALLBACK
   
  nsAutoCompleteController();
  virtual ~nsAutoCompleteController();
  
protected:
  nsresult OpenPopup();
  nsresult ClosePopup();

  nsresult StartSearch();
  
  nsresult StartSearchTimer();
  nsresult ClearSearchTimer();

  nsresult ProcessResult(PRInt32 aSearchIndex, nsIAutoCompleteResult *aResult);
  nsresult PostSearchCleanup();

  nsresult EnterMatch(bool aIsPopupSelection);
  nsresult RevertTextValue();

  nsresult CompleteDefaultIndex(PRInt32 aResultIndex);
  nsresult CompleteValue(nsString &aValue);

  nsresult GetResultAt(PRInt32 aIndex, nsIAutoCompleteResult** aResult,
                       PRInt32* aRowIndex);
  nsresult GetResultValueAt(PRInt32 aIndex, bool aValueOnly,
                            nsAString & _retval);
  nsresult GetResultLabelAt(PRInt32 aIndex, bool aValueOnly,
                            nsAString & _retval);
private:
  nsresult GetResultValueLabelAt(PRInt32 aIndex, bool aValueOnly,
                                 bool aGetValue, nsAString & _retval);
protected:
  nsresult GetDefaultCompleteValue(PRInt32 aResultIndex, bool aPreserveCasing,
                                   nsAString &_retval);
  nsresult ClearResults();
  
  nsresult RowIndexToSearch(PRInt32 aRowIndex,
                            PRInt32 *aSearchIndex, PRInt32 *aItemIndex);

  
  
  nsCOMPtr<nsIAutoCompleteInput> mInput;

  nsCOMArray<nsIAutoCompleteSearch> mSearches;
  nsCOMArray<nsIAutoCompleteResult> mResults;
  nsTArray<PRUint32> mMatchCounts;
  
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsITreeSelection> mSelection;
  nsCOMPtr<nsITreeBoxObject> mTree;

  nsString mSearchString;
  bool mDefaultIndexCompleted;
  bool mBackspaced;
  bool mPopupClosedByCompositionStart;
  bool mIsIMEComposing;
  bool mIgnoreHandleText;
  PRUint16 mSearchStatus;
  PRUint32 mRowCount;
  PRUint32 mSearchesOngoing;
  bool mFirstSearchResult;
};

#endif 
