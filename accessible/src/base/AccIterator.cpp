




































#include "AccIterator.h"

#include "nsAccessibilityService.h"
#include "nsAccessible.h"





AccIterator::AccIterator(nsAccessible *aAccessible,
                         filters::FilterFuncPtr aFilterFunc,
                         IterationType aIterationType) :
  mFilterFunc(aFilterFunc), mIsDeep(aIterationType != eFlatNav)
{
  mState = new IteratorState(aAccessible);
}

AccIterator::~AccIterator()
{
  while (mState) {
    IteratorState *tmp = mState;
    mState = tmp->mParentState;
    delete tmp;
  }
}

nsAccessible*
AccIterator::GetNext()
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




AccIterator::IteratorState::IteratorState(nsAccessible *aParent,
                                          IteratorState *mParentState) :
  mParent(aParent), mIndex(0), mParentState(mParentState)
{
}






RelatedAccIterator::
  RelatedAccIterator(nsDocAccessible* aDocument, nsIContent* aDependentContent,
                     nsIAtom* aRelAttr) :
  mRelAttr(aRelAttr), mProviders(nsnull), mBindingParent(nsnull), mIndex(0)
{
  mBindingParent = aDependentContent->GetBindingParent();
  nsIAtom* IDAttr = mBindingParent ?
    nsAccessibilityAtoms::anonid : aDependentContent->GetIDAttributeName();

  nsAutoString id;
  if (aDependentContent->GetAttr(kNameSpaceID_None, IDAttr, id))
    mProviders = aDocument->mDependentIDsHash.Get(id);
}

nsAccessible*
RelatedAccIterator::Next()
{
  if (!mProviders)
    return nsnull;

  while (mIndex < mProviders->Length()) {
    nsDocAccessible::AttrRelProvider* provider = (*mProviders)[mIndex++];

    
    
    if (provider->mRelAttr == mRelAttr &&
        (!mBindingParent ||
         mBindingParent == provider->mContent->GetBindingParent())) {
      nsAccessible* related = GetAccService()->GetAccessible(provider->mContent);
      if (related)
        return related;
    }
  }

  return nsnull;
}
