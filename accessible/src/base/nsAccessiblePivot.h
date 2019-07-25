






































#ifndef _nsAccessiblePivot_H_
#define _nsAccessiblePivot_H_

#include "nsIAccessiblePivot.h"

#include "nsAutoPtr.h"
#include "nsTObserverArray.h"
#include "nsCycleCollectionParticipant.h"

class nsAccessible;
class nsIAccessibleTraversalRule;




class nsAccessiblePivot: public nsIAccessiblePivot
{
public:
  nsAccessiblePivot(nsAccessible* aRoot);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsAccessiblePivot, nsIAccessiblePivot)

  NS_DECL_NSIACCESSIBLEPIVOT

  


  nsAccessible* Position() { return mPosition; }

private:
  nsAccessiblePivot() MOZ_DELETE;
  nsAccessiblePivot(const nsAccessiblePivot&) MOZ_DELETE;
  void operator = (const nsAccessiblePivot&) MOZ_DELETE;

  


  void NotifyPivotChanged(nsAccessible* aOldAccessible,
                          PRInt32 aOldStart, PRInt32 aOldEnd);

  


  bool IsRootDescendant(nsAccessible* aAccessible);


  


  nsAccessible* SearchForward(nsAccessible* aAccessible,
                              nsIAccessibleTraversalRule* aRule,
                              bool searchCurrent,
                              nsresult* rv);

  


  nsAccessible* SearchBackward(nsAccessible* aAccessible,
                               nsIAccessibleTraversalRule* aRule,
                               bool searchCurrent,
                               nsresult* rv);

  


  void MovePivotInternal(nsAccessible* aPosition);

  


  nsRefPtr<nsAccessible> mRoot;

  


  nsRefPtr<nsAccessible> mPosition;

  


  PRInt32 mStartOffset;

  


  PRInt32 mEndOffset;

  


  nsTObserverArray<nsCOMPtr<nsIAccessiblePivotObserver> > mObservers;
};

#endif
