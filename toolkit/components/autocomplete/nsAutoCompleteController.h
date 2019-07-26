



#ifndef __nsAutoCompleteController__
#define __nsAutoCompleteController__

#include "nsIAutoCompleteController.h"

#include "nsCOMPtr.h"
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

  nsresult StartSearch(uint16_t aSearchType);

  nsresult BeforeSearches();
  nsresult StartSearches();
  void AfterSearches();
  nsresult ClearSearchTimer();

  nsresult ProcessResult(int32_t aSearchIndex, nsIAutoCompleteResult *aResult);
  nsresult PostSearchCleanup();

  nsresult EnterMatch(bool aIsPopupSelection);
  nsresult RevertTextValue();

  nsresult CompleteDefaultIndex(int32_t aResultIndex);
  nsresult CompleteValue(nsString &aValue);

  nsresult GetResultAt(int32_t aIndex, nsIAutoCompleteResult** aResult,
                       int32_t* aRowIndex);
  nsresult GetResultValueAt(int32_t aIndex, bool aValueOnly,
                            nsAString & _retval);
  nsresult GetResultLabelAt(int32_t aIndex, bool aValueOnly,
                            nsAString & _retval);
private:
  nsresult GetResultValueLabelAt(int32_t aIndex, bool aValueOnly,
                                 bool aGetValue, nsAString & _retval);
protected:

  











  nsresult GetDefaultCompleteResult(int32_t aResultIndex,
                                    nsIAutoCompleteResult** _result,
                                    int32_t* _defaultIndex);

  









  nsresult GetDefaultCompleteValue(int32_t aResultIndex, bool aPreserveCasing,
                                   nsAString &_retval);

  










  nsresult GetFinalDefaultCompleteValue(nsAString &_retval);

  nsresult ClearResults();
  
  nsresult RowIndexToSearch(int32_t aRowIndex,
                            int32_t *aSearchIndex, int32_t *aItemIndex);

  
  
  nsCOMPtr<nsIAutoCompleteInput> mInput;

  nsCOMArray<nsIAutoCompleteSearch> mSearches;
  nsCOMArray<nsIAutoCompleteResult> mResults;
  
  
  nsTArray<uint32_t> mMatchCounts;
  
  
  
  nsCOMArray<nsIAutoCompleteResult> mResultCache;

  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsITreeSelection> mSelection;
  nsCOMPtr<nsITreeBoxObject> mTree;

  nsString mSearchString;
  bool mDefaultIndexCompleted;
  bool mBackspaced;
  bool mPopupClosedByCompositionStart;
  enum CompositionState {
    eCompositionState_None,
    eCompositionState_Composing,
    eCompositionState_Committing
  };
  CompositionState mCompositionState;
  uint16_t mSearchStatus;
  uint32_t mRowCount;
  uint32_t mSearchesOngoing;
  uint32_t mSearchesFailed;
  bool mFirstSearchResult;
  uint32_t mImmediateSearchesCount;
};

#endif 
