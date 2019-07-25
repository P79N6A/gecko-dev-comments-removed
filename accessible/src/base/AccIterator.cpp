




































#include "AccIterator.h"

#include "nsAccessibilityService.h"
#include "nsAccessible.h"

#include "mozilla/dom/Element.h"





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
AccIterator::Next()
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
  mDocument(aDocument), mRelAttr(aRelAttr), mProviders(nsnull),
  mBindingParent(nsnull), mIndex(0)
{
  mBindingParent = aDependentContent->GetBindingParent();
  nsIAtom* IDAttr = mBindingParent ?
    nsAccessibilityAtoms::anonid : aDependentContent->GetIDAttributeName();

  nsAutoString id;
  if (aDependentContent->GetAttr(kNameSpaceID_None, IDAttr, id))
    mProviders = mDocument->mDependentIDsHash.Get(id);
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
      nsAccessible* related = mDocument->GetAccessible(provider->mContent);
      if (related)
        return related;

      
      
      if (provider->mContent == mDocument->GetContent())
        return mDocument;
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





IDRefsIterator::IDRefsIterator(nsIContent* aContent, nsIAtom* aIDRefsAttr) :
  mCurrIdx(0)
{
  if (!aContent->IsInDoc() ||
      !aContent->GetAttr(kNameSpaceID_None, aIDRefsAttr, mIDs))
    return;

  if (aContent->IsInAnonymousSubtree()) {
    mXBLDocument = do_QueryInterface(aContent->GetOwnerDoc());
    mBindingParent = do_QueryInterface(aContent->GetBindingParent());
  } else {
    mDocument = aContent->GetOwnerDoc();
  }
}

const nsDependentSubstring
IDRefsIterator::NextID()
{
  for (; mCurrIdx < mIDs.Length(); mCurrIdx++) {
    if (!NS_IsAsciiWhitespace(mIDs[mCurrIdx]))
      break;
  }

  if (mCurrIdx >= mIDs.Length())
    return nsDependentSubstring();

  nsAString::index_type idStartIdx = mCurrIdx;
  while (++mCurrIdx < mIDs.Length()) {
    if (NS_IsAsciiWhitespace(mIDs[mCurrIdx]))
      break;
  }

  return Substring(mIDs, idStartIdx, mCurrIdx++ - idStartIdx);
}

nsIContent*
IDRefsIterator::NextElem()
{
  while (true) {
    const nsDependentSubstring id = NextID();
    if (id.IsEmpty())
      break;

    nsIContent* refContent = GetElem(id);
    if (refContent)
      return refContent;
  }

  return nsnull;
}

nsIContent*
IDRefsIterator::GetElem(const nsDependentSubstring& aID)
{
  if (mXBLDocument) {
    
    

    nsCOMPtr<nsIDOMElement> refElm;
    mXBLDocument->GetAnonymousElementByAttribute(mBindingParent,
                                                 NS_LITERAL_STRING("anonid"),
                                                 aID,
                                                 getter_AddRefs(refElm));
    nsCOMPtr<nsIContent> refContent = do_QueryInterface(refElm);
    return refContent;
  }

  return mDocument->GetElementById(aID);
}

nsAccessible*
IDRefsIterator::Next()
{
  nsIContent* nextElm = NextElem();
  return nextElm ? GetAccService()->GetAccessible(nextElm) : nsnull;
}

nsAccessible*
SingleAccIterator::Next()
{
  nsRefPtr<nsAccessible> nextAcc;
  mAcc.swap(nextAcc);
  return (nextAcc && !nextAcc->IsDefunct()) ? nextAcc : nsnull;
}

