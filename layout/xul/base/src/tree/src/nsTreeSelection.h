








































#ifndef nsTreeSelection_h__
#define nsTreeSelection_h__

#include "nsITreeSelection.h"
#include "nsITreeColumns.h"
#include "nsITimer.h"
#include "nsCycleCollectionParticipant.h"

class nsITreeBoxObject;
struct nsTreeRange;

class nsTreeSelection : public nsINativeTreeSelection
{
public:
  nsTreeSelection(nsITreeBoxObject* aTree);
  ~nsTreeSelection();
   
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsTreeSelection)
  NS_DECL_NSITREESELECTION

  
  NS_IMETHOD EnsureNative() { return NS_OK; }

  friend struct nsTreeRange;

protected:
  nsresult FireOnSelectHandler();
  static void SelectCallback(nsITimer *aTimer, void *aClosure);

protected:
  
  nsCOMPtr<nsITreeBoxObject> mTree; 

  bool mSuppressed; 
  PRInt32 mCurrentIndex; 
  nsCOMPtr<nsITreeColumn> mCurrentColumn;
  PRInt32 mShiftSelectPivot; 

  nsTreeRange* mFirstRange; 

  nsCOMPtr<nsITimer> mSelectTimer;
};

nsresult
NS_NewTreeSelection(nsITreeBoxObject* aTree, nsITreeSelection** aResult);

#endif
