





#ifndef _nsAccessiblePivot_H_
#define _nsAccessiblePivot_H_

#include "nsIAccessiblePivot.h"

#include "nsAutoPtr.h"
#include "nsTObserverArray.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class Accessible;
class nsIAccessibleTraversalRule;


#define NS_ERROR_NOT_IN_TREE \
NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL, 0x26)




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
                           PRInt32 aOldStart, PRInt32 aOldEnd,
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

  


  PRInt32 mStartOffset;

  


  PRInt32 mEndOffset;

  


  nsTObserverArray<nsCOMPtr<nsIAccessiblePivotObserver> > mObservers;
};

#endif
