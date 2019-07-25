




































#include "nsAccIterator.h"




nsAccIterator::nsAccIterator(nsAccessible *aAccessible,
                             AccIteratorFilterFuncPtr aFilterFunc,
                             IterationType aIterationType) :
  mFilterFunc(aFilterFunc), mIsDeep(aIterationType != eFlatNav)
{
  mState = new IteratorState(aAccessible);
}

nsAccIterator::~nsAccIterator()
{
  while (mState) {
    IteratorState *tmp = mState;
    mState = tmp->mParentState;
    delete tmp;
  }
}

nsAccessible*
nsAccIterator::GetNext()
{
  while (mState) {
    nsAccessible *child = mState->mParent->GetChildAt(mState->mIndex++);
    if (!child) {
      IteratorState *tmp = mState;
      mState = mState->mParentState;
      delete tmp;

      continue;
    }

    PRBool isComplying = mFilterFunc(child);
    if (isComplying)
      return child;

    if (mIsDeep) {
      IteratorState *childState = new IteratorState(child, mState);
      mState = childState;
    }
  }

  return nsnull;
}




nsAccIterator::IteratorState::IteratorState(nsAccessible *aParent,
                                            IteratorState *mParentState) :
  mParent(aParent), mIndex(0), mParentState(mParentState)
{
}
