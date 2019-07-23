





































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
#include "nsISupportsArray.h"
#include "nsITimer.h"
#include "nsIRollupListener.h"
#include "nsIWidget.h"
#include "nsTArray.h"

class nsAutoCompleteController : public nsIAutoCompleteController,
                                 public nsIAutoCompleteObserver,
                                 public nsIRollupListener,
                                 public nsITimerCallback,
                                 public nsITreeView
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETECONTROLLER
  NS_DECL_NSIAUTOCOMPLETEOBSERVER
  NS_DECL_NSIROLLUPLISTENER
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

  nsresult EnterMatch();
  nsresult RevertTextValue();

  nsresult CompleteDefaultIndex(PRInt32 aSearchIndex);
  nsresult CompleteValue(nsString &aValue, PRBool selectDifference);
  nsresult GetResultValueAt(PRInt32 aIndex, PRBool aValueOnly, nsAString & _retval);

  nsresult ClearResults();
  
  nsresult RowIndexToSearch(PRInt32 aRowIndex, PRInt32 *aSearchIndex, PRInt32 *aItemIndex);

  nsIWidget* GetPopupWidget();

  
  
  nsCOMPtr<nsIAutoCompleteInput> mInput;
  
  nsCOMPtr<nsISupportsArray> mSearches;
  nsCOMPtr<nsISupportsArray> mResults;
  nsTArray<PRUint32> mMatchCounts;
  
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsITreeSelection> mSelection;
  nsCOMPtr<nsITreeBoxObject> mTree;

  nsString mSearchString;
  PRPackedBool mEnterAfterSearch;
  PRPackedBool mDefaultIndexCompleted;
  PRPackedBool mBackspaced;
  PRPackedBool mPopupClosedByCompositionStart;
  PRPackedBool mIsIMEComposing;
  PRPackedBool mIgnoreHandleText;
  PRBool mIsOpen;
  PRUint16 mSearchStatus;
  PRUint32 mRowCount;
  PRUint32 mSearchesOngoing;
  PRBool mFirstSearchResult;
};

#endif 
