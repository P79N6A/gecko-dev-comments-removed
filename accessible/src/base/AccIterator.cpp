



#include "AccIterator.h"

#include "nsAccessibilityService.h"
#include "Accessible.h"

#include "mozilla/dom/Element.h"
#include "nsBindingManager.h"

using namespace mozilla;
using namespace mozilla::a11y;





AccIterator::AccIterator(Accessible* aAccessible,
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

Accessible*
AccIterator::Next()
{
  while (mState) {
    Accessible* child = mState->mParent->GetChildAt(mState->mIndex++);
    if (!child) {
      IteratorState *tmp = mState;
      mState = mState->mParentState;
      delete tmp;

      continue;
    }

    bool isComplying = mFilterFunc(child);
    if (isComplying)
      return child;

    if (mIsDeep) {
      IteratorState *childState = new IteratorState(child, mState);
      mState = childState;
    }
  }

  return nsnull;
}




AccIterator::IteratorState::IteratorState(Accessible* aParent,
                                          IteratorState *mParentState) :
  mParent(aParent), mIndex(0), mParentState(mParentState)
{
}






RelatedAccIterator::
  RelatedAccIterator(DocAccessible* aDocument, nsIContent* aDependentContent,
                     nsIAtom* aRelAttr) :
  mDocument(aDocument), mRelAttr(aRelAttr), mProviders(nsnull),
  mBindingParent(nsnull), mIndex(0)
{
  mBindingParent = aDependentContent->GetBindingParent();
  nsIAtom* IDAttr = mBindingParent ?
    nsGkAtoms::anonid : aDependentContent->GetIDAttributeName();

  nsAutoString id;
  if (aDependentContent->GetAttr(kNameSpaceID_None, IDAttr, id))
    mProviders = mDocument->mDependentIDsHash.Get(id);
}

Accessible*
RelatedAccIterator::Next()
{
  if (!mProviders)
    return nsnull;

  while (mIndex < mProviders->Length()) {
    DocAccessible::AttrRelProvider* provider = (*mProviders)[mIndex++];

    
    
    if (provider->mRelAttr == mRelAttr) {
      nsIContent* bindingParent = provider->mContent->GetBindingParent();
      bool inScope = mBindingParent == bindingParent ||
        mBindingParent == provider->mContent;

      if (inScope) {
        Accessible* related = mDocument->GetAccessible(provider->mContent);
        if (related)
          return related;

        
        
        if (provider->mContent == mDocument->GetContent())
          return mDocument;
      }
    }
  }

  return nsnull;
}






HTMLLabelIterator::
  HTMLLabelIterator(DocAccessible* aDocument, const Accessible* aAccessible,
                    LabelFilter aFilter) :
  mRelIter(aDocument, aAccessible->GetContent(), nsGkAtoms::_for),
  mAcc(aAccessible), mLabelFilter(aFilter)
{
}

Accessible*
HTMLLabelIterator::Next()
{
  
  
  Accessible* label = nsnull;
  while ((label = mRelIter.Next())) {
    if (label->GetContent()->Tag() == nsGkAtoms::label)
      return label;
  }

  
  if (mLabelFilter == eSkipAncestorLabel || !mAcc->IsWidget())
    return nsnull;

  
  
  
  Accessible* walkUp = mAcc->Parent();
  while (walkUp && !walkUp->IsDoc()) {
    nsIContent* walkUpElm = walkUp->GetContent();
    if (walkUpElm->IsHTML()) {
      if (walkUpElm->Tag() == nsGkAtoms::label &&
          !walkUpElm->HasAttr(kNameSpaceID_None, nsGkAtoms::_for)) {
        mLabelFilter = eSkipAncestorLabel; 
        return walkUp;
      }

      if (walkUpElm->Tag() == nsGkAtoms::form)
        break;
    }

    walkUp = walkUp->Parent();
  }

  return nsnull;
}






HTMLOutputIterator::
HTMLOutputIterator(DocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsGkAtoms::_for)
{
}

Accessible*
HTMLOutputIterator::Next()
{
  Accessible* output = nsnull;
  while ((output = mRelIter.Next())) {
    if (output->GetContent()->Tag() == nsGkAtoms::output)
      return output;
  }

  return nsnull;
}






XULLabelIterator::
  XULLabelIterator(DocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsGkAtoms::control)
{
}

Accessible*
XULLabelIterator::Next()
{
  Accessible* label = nsnull;
  while ((label = mRelIter.Next())) {
    if (label->GetContent()->Tag() == nsGkAtoms::label)
      return label;
  }

  return nsnull;
}






XULDescriptionIterator::
  XULDescriptionIterator(DocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsGkAtoms::control)
{
}

Accessible*
XULDescriptionIterator::Next()
{
  Accessible* descr = nsnull;
  while ((descr = mRelIter.Next())) {
    if (descr->GetContent()->Tag() == nsGkAtoms::description)
      return descr;
  }

  return nsnull;
}





IDRefsIterator::
  IDRefsIterator(DocAccessible* aDoc, nsIContent* aContent,
                 nsIAtom* aIDRefsAttr) :
  mContent(aContent), mDoc(aDoc), mCurrIdx(0)
{
  if (mContent->IsInDoc())
    mContent->GetAttr(kNameSpaceID_None, aIDRefsAttr, mIDs);
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
  
  
  if (!mContent->IsInAnonymousSubtree()) {
    dom::Element* refElm = mContent->OwnerDoc()->GetElementById(aID);
    if (refElm || !mContent->OwnerDoc()->BindingManager()->GetBinding(mContent))
      return refElm;
  }

  
  

  
  nsIContent* bindingParent = mContent->GetBindingParent();
  if (bindingParent) {
    nsIContent* refElm = bindingParent->OwnerDoc()->
      GetAnonymousElementByAttribute(bindingParent, nsGkAtoms::anonid, aID);

    if (refElm)
      return refElm;
  }

  
  if (mContent->OwnerDoc()->BindingManager()->GetBinding(mContent)) {
    return mContent->OwnerDoc()->
      GetAnonymousElementByAttribute(mContent, nsGkAtoms::anonid, aID);
  }

  return nsnull;
}

Accessible*
IDRefsIterator::Next()
{
  nsIContent* nextElm = NextElem();
  return nextElm ? mDoc->GetAccessible(nextElm) : nsnull;
}

Accessible*
SingleAccIterator::Next()
{
  nsRefPtr<Accessible> nextAcc;
  mAcc.swap(nextAcc);
  return (nextAcc && !nextAcc->IsDefunct()) ? nextAcc : nsnull;
}

