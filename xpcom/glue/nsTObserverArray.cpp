





#include "nsTObserverArray.h"

void
nsTObserverArray_base::AdjustIterators(index_type aModPos,
                                       diff_type aAdjustment)
{
  NS_PRECONDITION(aAdjustment == -1 || aAdjustment == 1, "invalid adjustment");
  Iterator_base* iter = mIterators;
  while (iter) {
    if (iter->mPosition > aModPos) {
      iter->mPosition += aAdjustment;
    }
    iter = iter->mNext;
  }
}

void
nsTObserverArray_base::ClearIterators()
{
  Iterator_base* iter = mIterators;
  while (iter) {
    iter->mPosition = 0;
    iter = iter->mNext;
  }
}
