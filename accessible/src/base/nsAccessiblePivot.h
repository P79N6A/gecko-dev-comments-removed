





#ifndef _nsAccessiblePivot_H_
#define _nsAccessiblePivot_H_

#include "nsIAccessiblePivot.h"

#include "nsAutoPtr.h"
#include "nsTObserverArray.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class Accessible;
class nsIAccessibleTraversalRule;




class nsAccessiblePivot MOZ_FINAL : public nsIAccessiblePivot
{
public:
  nsAccessiblePivot(Accessible* aRoot);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsAccessiblePivot, nsIAccessiblePivot)

  NS_DECL_NSIACCESSIBLEPIVOT

  


  Accessible* Position() { return mPosition; }

private:
  nsAccessiblePivot() MOZ_DELETE;
  nsAccessiblePivot(const nsAccessiblePivot&) MOZ_DELETE;
  void operator = (const nsAccessiblePivot&) MOZ_DELETE;

  



  bool NotifyOfPivotChange(Accessible* aOldAccessible,
                           int32_t aOldStart, int32_t aOldEnd,
                           PivotMoveReason aReason);

  


  bool IsRootDescendant(Accessible* aAccessible);


  


  Accessible* SearchForward(Accessible* aAccessible,
                            nsIAccessibleTraversalRule* aRule,
                            bool aSearchCurrent,
                            nsresult* aResult);

  


  Accessible* SearchBackward(Accessible* aAccessible,
                             nsIAccessibleTraversalRule* aRule,
                             bool aSearchCurrent,
                             nsresult* aResult);

  


  bool MovePivotInternal(Accessible* aPosition, PivotMoveReason aReason);

  


  nsRefPtr<Accessible> mRoot;

  


  nsRefPtr<Accessible> mPosition;

  


  int32_t mStartOffset;

  


  int32_t mEndOffset;

  


  nsTObserverArray<nsCOMPtr<nsIAccessiblePivotObserver> > mObservers;
};

#endif
