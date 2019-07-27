



#include "AccIterator.h"

#include "AccGroupInfo.h"
#ifdef MOZ_XUL
#include "XULTreeAccessible.h"
#endif

#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;





AccIterator::AccIterator(Accessible* aAccessible,
                         filters::FilterFuncPtr aFilterFunc) :
  mFilterFunc(aFilterFunc)
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
      IteratorState* tmp = mState;
      mState = mState->mParentState;
      delete tmp;

      continue;
    }

    uint32_t result = mFilterFunc(child);
    if (result & filters::eMatch)
      return child;

    if (!(result & filters::eSkipSubtree)) {
      IteratorState* childState = new IteratorState(child, mState);
      mState = childState;
    }
  }

  return nullptr;
}




AccIterator::IteratorState::IteratorState(Accessible* aParent,
                                          IteratorState *mParentState) :
  mParent(aParent), mIndex(0), mParentState(mParentState)
{
}






RelatedAccIterator::
  RelatedAccIterator(DocAccessible* aDocument, nsIContent* aDependentContent,
                     nsIAtom* aRelAttr) :
  mDocument(aDocument), mRelAttr(aRelAttr), mProviders(nullptr),
  mBindingParent(nullptr), mIndex(0)
{
  mBindingParent = aDependentContent->GetBindingParent();
  nsIAtom* IDAttr = mBindingParent ?
    nsGkAtoms::anonid : nsGkAtoms::id;

  nsAutoString id;
  if (aDependentContent->GetAttr(kNameSpaceID_None, IDAttr, id))
    mProviders = mDocument->mDependentIDsHash.Get(id);
}

Accessible*
RelatedAccIterator::Next()
{
  if (!mProviders)
    return nullptr;

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

  return nullptr;
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
  
  
  Accessible* label = nullptr;
  while ((label = mRelIter.Next())) {
    if (label->GetContent()->Tag() == nsGkAtoms::label)
      return label;
  }

  
  if (mLabelFilter == eSkipAncestorLabel || !mAcc->IsWidget())
    return nullptr;

  
  
  
  Accessible* walkUp = mAcc->Parent();
  while (walkUp && !walkUp->IsDoc()) {
    nsIContent* walkUpElm = walkUp->GetContent();
    if (walkUpElm->IsHTMLElement()) {
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

  return nullptr;
}






HTMLOutputIterator::
HTMLOutputIterator(DocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsGkAtoms::_for)
{
}

Accessible*
HTMLOutputIterator::Next()
{
  Accessible* output = nullptr;
  while ((output = mRelIter.Next())) {
    if (output->GetContent()->Tag() == nsGkAtoms::output)
      return output;
  }

  return nullptr;
}






XULLabelIterator::
  XULLabelIterator(DocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsGkAtoms::control)
{
}

Accessible*
XULLabelIterator::Next()
{
  Accessible* label = nullptr;
  while ((label = mRelIter.Next())) {
    if (label->GetContent()->Tag() == nsGkAtoms::label)
      return label;
  }

  return nullptr;
}






XULDescriptionIterator::
  XULDescriptionIterator(DocAccessible* aDocument, nsIContent* aElement) :
  mRelIter(aDocument, aElement, nsGkAtoms::control)
{
}

Accessible*
XULDescriptionIterator::Next()
{
  Accessible* descr = nullptr;
  while ((descr = mRelIter.Next())) {
    if (descr->GetContent()->Tag() == nsGkAtoms::description)
      return descr;
  }

  return nullptr;
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

  return nullptr;
}

nsIContent*
IDRefsIterator::GetElem(const nsDependentSubstring& aID)
{
  
  
  if (!mContent->IsInAnonymousSubtree()) {
    dom::Element* refElm = mContent->OwnerDoc()->GetElementById(aID);
    if (refElm || !mContent->GetXBLBinding())
      return refElm;
  }

  
  

  
  nsIContent* bindingParent = mContent->GetBindingParent();
  if (bindingParent) {
    nsIContent* refElm = bindingParent->OwnerDoc()->
      GetAnonymousElementByAttribute(bindingParent, nsGkAtoms::anonid, aID);

    if (refElm)
      return refElm;
  }

  
  if (mContent->GetXBLBinding()) {
    return mContent->OwnerDoc()->
      GetAnonymousElementByAttribute(mContent, nsGkAtoms::anonid, aID);
  }

  return nullptr;
}

Accessible*
IDRefsIterator::Next()
{
  nsIContent* nextElm = NextElem();
  return nextElm ? mDoc->GetAccessible(nextElm) : nullptr;
}






ARIAOwnedByIterator::ARIAOwnedByIterator(const Accessible* aDependent) :
  RelatedAccIterator(aDependent->Document(), aDependent->GetContent(),
                     nsGkAtoms::aria_owns), mDependent(aDependent)
{
}

Accessible*
ARIAOwnedByIterator::Next()
{
  Accessible* owner = RelatedAccIterator::Next();
  Accessible* cur = owner;
  while (cur) {
    if (cur == mDependent)
      return Next(); 

    if (cur->IsDoc())
      break; 

    cur = cur->Parent();
  }

  return owner;
}






ARIAOwnsIterator::ARIAOwnsIterator(const Accessible* aOwner) :
  mIter(aOwner->Document(), aOwner->GetContent(), nsGkAtoms::aria_owns),
  mOwner(aOwner)
{
}

Accessible*
ARIAOwnsIterator::Next()
{
  Accessible* child = mIter.Next();
  const Accessible* cur = mOwner;
  while (cur) {
    if (cur == child)
      return Next(); 

    if (cur->IsDoc())
      break; 

    cur = cur->Parent();
  }

  return child;
}






Accessible*
SingleAccIterator::Next()
{
  nsRefPtr<Accessible> nextAcc;
  mAcc.swap(nextAcc);
  return (nextAcc && !nextAcc->IsDefunct()) ? nextAcc : nullptr;
}






Accessible*
ItemIterator::Next()
{
  if (mContainer) {
    mAnchor = AccGroupInfo::FirstItemOf(mContainer);
    mContainer = nullptr;
    return mAnchor;
  }

  return mAnchor ? (mAnchor = AccGroupInfo::NextItemTo(mAnchor)) : nullptr;
}






XULTreeItemIterator::XULTreeItemIterator(XULTreeAccessible* aXULTree,
                                         nsITreeView* aTreeView,
                                         int32_t aRowIdx) :
  mXULTree(aXULTree), mTreeView(aTreeView), mRowCount(-1),
  mContainerLevel(-1), mCurrRowIdx(aRowIdx + 1)
{
  mTreeView->GetRowCount(&mRowCount);
  if (aRowIdx != -1)
    mTreeView->GetLevel(aRowIdx, &mContainerLevel);
}

Accessible*
XULTreeItemIterator::Next()
{
  while (mCurrRowIdx < mRowCount) {
    int32_t level = 0;
    mTreeView->GetLevel(mCurrRowIdx, &level);

    if (level == mContainerLevel + 1)
      return mXULTree->GetTreeItemAccessible(mCurrRowIdx++);

    if (level <= mContainerLevel) { 
      mCurrRowIdx = mRowCount;
      break;
    }

    mCurrRowIdx++;
  }

  return nullptr;
}
