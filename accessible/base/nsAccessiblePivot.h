





#ifndef _nsAccessiblePivot_H_
#define _nsAccessiblePivot_H_

#include "nsIAccessiblePivot.h"

#include "Accessible-inl.h"
#include "nsAutoPtr.h"
#include "nsTObserverArray.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class RuleCache;




class nsAccessiblePivot MOZ_FINAL : public nsIAccessiblePivot
{
public:
  typedef mozilla::a11y::Accessible Accessible;

  explicit nsAccessiblePivot(Accessible* aRoot);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsAccessiblePivot, nsIAccessiblePivot)

  NS_DECL_NSIACCESSIBLEPIVOT

  


  Accessible* Position() { return mPosition; }

private:
  ~nsAccessiblePivot();
  nsAccessiblePivot() MOZ_DELETE;
  nsAccessiblePivot(const nsAccessiblePivot&) MOZ_DELETE;
  void operator = (const nsAccessiblePivot&) MOZ_DELETE;

  



  bool NotifyOfPivotChange(Accessible* aOldAccessible,
                           int32_t aOldStart, int32_t aOldEnd,
                           PivotMoveReason aReason,
                           bool aIsFromUserInput);

  


  bool IsDescendantOf(Accessible* aAccessible, Accessible* aAncestor);


  


  Accessible* SearchForward(Accessible* aAccessible,
                            nsIAccessibleTraversalRule* aRule,
                            bool aSearchCurrent,
                            nsresult* aResult);

  


  Accessible* SearchBackward(Accessible* aAccessible,
                             nsIAccessibleTraversalRule* aRule,
                             bool aSearchCurrent,
                             nsresult* aResult);

  


  mozilla::a11y::HyperTextAccessible* SearchForText(Accessible* aAccessible,
                                                    bool aBackward);

  


  Accessible* GetActiveRoot() const
  {
    if (mModalRoot) {
      NS_ENSURE_FALSE(mModalRoot->IsDefunct(), mRoot);
      return mModalRoot;
    }

    return mRoot;
  }

  


  bool MovePivotInternal(Accessible* aPosition, PivotMoveReason aReason,
                         bool aIsFromUserInput);

  








  Accessible* AdjustStartPosition(Accessible* aAccessible, RuleCache& aCache,
                                  uint16_t* aFilterResult, nsresult* aResult);

  


  nsRefPtr<Accessible> mRoot;

  


  nsRefPtr<Accessible> mModalRoot;

  


  nsRefPtr<Accessible> mPosition;

  


  int32_t mStartOffset;

  


  int32_t mEndOffset;

  


  nsTObserverArray<nsCOMPtr<nsIAccessiblePivotObserver> > mObservers;
};

#endif
