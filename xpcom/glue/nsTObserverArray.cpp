




































#include "nsTObserverArray.h"

void
nsTObserverArray_base::AdjustIterators(PRInt32 aModPos,
                                       PRInt32 aAdjustment)
{
  NS_PRECONDITION(aAdjustment == -1 || aAdjustment == 1,
                  "invalid adjustment");
  Iterator_base* iter = mIterators;
  while (iter) {
    NS_ASSERTION(&(iter->mArray) == this, "wrong array");

    if (iter->mPosition > aModPos) {
      iter->mPosition += aAdjustment;
    }
    iter = iter->mNext;
  }
}

void
nsTObserverArray_base::Clear()
{
  mObservers.Clear();

  Iterator_base* iter = mIterators;
  while (iter) {
    NS_ASSERTION(&(iter->mArray) == this, "wrong array");

    iter->mPosition = 0;
    iter = iter->mNext;
  }
}
