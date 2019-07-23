








































#ifndef nsTreeSelection_h__
#define nsTreeSelection_h__

#include "nsITreeSelection.h"
#include "nsITreeColumns.h"
#include "nsITimer.h"
#include "nsCycleCollectionParticipant.h"

class nsITreeBoxObject;
struct nsTreeRange;

class nsTreeSelection : public nsITreeSelection
{
public:
  nsTreeSelection(nsITreeBoxObject* aTree);
  ~nsTreeSelection();
   
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsTreeSelection)
  NS_DECL_NSITREESELECTION

  friend struct nsTreeRange;

protected:
  nsresult FireOnSelectHandler();
  static void SelectCallback(nsITimer *aTimer, void *aClosure);

protected:
  
  nsCOMPtr<nsITreeBoxObject> mTree; 

  PRBool mSuppressed; 
  PRInt32 mCurrentIndex; 
  nsCOMPtr<nsITreeColumn> mCurrentColumn;
  PRInt32 mShiftSelectPivot; 

  nsTreeRange* mFirstRange; 

  nsCOMPtr<nsITimer> mSelectTimer;
};

nsresult
NS_NewTreeSelection(nsITreeBoxObject* aTree, nsITreeSelection** aResult);

#endif
