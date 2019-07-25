




































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






HTMLLabelIterator::
  HTMLLabelIterator(nsDocAccessible* aDocument, nsIContent* aElement,
                    LabelFilter aFilter) :
  mRelIter(aDocument, aElement, nsAccessibilityAtoms::_for),
  mElement(aElement), mLabelFilter(aFilter)
{
}

nsAccessible*
HTMLLabelIterator::Next()
{
  
  
  nsAccessible* label = nsnull;
  while ((label = mRelIter.Next())) {
    if (label->GetContent()->Tag() == nsAccessibilityAtoms::label)
      return label;
  }

  if (mLabelFilter == eSkipAncestorLabel)
    return nsnull;

  
  
  nsIContent* walkUpContent = mElement;
  while ((walkUpContent = walkUpContent->GetParent()) &&
         walkUpContent->Tag() != nsAccessibilityAtoms::form &&
         walkUpContent->Tag() != nsAccessibilityAtoms::body) {
    if (walkUpContent->Tag() == nsAccessibilityAtoms::label) {
      
      mLabelFilter = eSkipAncestorLabel;
      return GetAccService()->GetAccessible(walkUpContent);
    }
  }

  return nsnull;
}






HTMLOutputIterator::
HTMLOutputIterator(nsDocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsAccessibilityAtoms::_for)
{
}

nsAccessible*
HTMLOutputIterator::Next()
{
  nsAccessible* output = nsnull;
  while ((output = mRelIter.Next())) {
    if (output->GetContent()->Tag() == nsAccessibilityAtoms::output)
      return output;
  }

  return nsnull;
}






XULLabelIterator::
  XULLabelIterator(nsDocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsAccessibilityAtoms::control)
{
}

nsAccessible*
XULLabelIterator::Next()
{
  nsAccessible* label = nsnull;
  while ((label = mRelIter.Next())) {
    if (label->GetContent()->Tag() == nsAccessibilityAtoms::label)
      return label;
  }

  return nsnull;
}






XULDescriptionIterator::
  XULDescriptionIterator(nsDocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsAccessibilityAtoms::control)
{
}

nsAccessible*
XULDescriptionIterator::Next()
{
  nsAccessible* descr = nsnull;
  while ((descr = mRelIter.Next())) {
    if (descr->GetContent()->Tag() == nsAccessibilityAtoms::description)
      return descr;
  }

  return nsnull;
}
