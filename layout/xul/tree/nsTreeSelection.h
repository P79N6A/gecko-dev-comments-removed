





#ifndef nsTreeSelection_h__
#define nsTreeSelection_h__

#include "nsITreeSelection.h"
#include "nsITreeColumns.h"
#include "nsITimer.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class nsITreeBoxObject;
struct nsTreeRange;

class nsTreeSelection MOZ_FINAL : public nsINativeTreeSelection
{
public:
  explicit nsTreeSelection(nsITreeBoxObject* aTree);
   
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsTreeSelection)
  NS_DECL_NSITREESELECTION

  
  NS_IMETHOD EnsureNative() MOZ_OVERRIDE { return NS_OK; }

  friend struct nsTreeRange;

protected:
  ~nsTreeSelection();

  nsresult FireOnSelectHandler();
  static void SelectCallback(nsITimer *aTimer, void *aClosure);

protected:
  
  nsCOMPtr<nsITreeBoxObject> mTree; 

  bool mSuppressed; 
  int32_t mCurrentIndex; 
  nsCOMPtr<nsITreeColumn> mCurrentColumn;
  int32_t mShiftSelectPivot; 

  nsTreeRange* mFirstRange; 

  nsCOMPtr<nsITimer> mSelectTimer;
};

nsresult
NS_NewTreeSelection(nsITreeBoxObject* aTree, nsITreeSelection** aResult);

#endif
