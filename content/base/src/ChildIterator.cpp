





#include "ChildIterator.h"
#include "mozilla/dom/XBLChildrenElement.h"

namespace mozilla {
namespace dom {

nsIContent*
ExplicitChildIterator::GetNextChild()
{
  
  if (mIndexInInserted) {
    MOZ_ASSERT(mChild);
    MOZ_ASSERT(mChild->IsActiveChildrenElement());
    MOZ_ASSERT(!mDefaultChild);

    XBLChildrenElement* point = static_cast<XBLChildrenElement*>(mChild);
    if (mIndexInInserted < point->mInsertedChildren.Length()) {
      return point->mInsertedChildren[mIndexInInserted++];
    }
    mIndexInInserted = 0;
    mChild = mChild->GetNextSibling();
  } else if (mDefaultChild) {
    
    MOZ_ASSERT(mChild);
    MOZ_ASSERT(mChild->IsActiveChildrenElement());

    mDefaultChild = mDefaultChild->GetNextSibling();
    if (mDefaultChild) {
      return mDefaultChild;
    }

    mChild = mChild->GetNextSibling();
  } else if (mIsFirst) {  
    mChild = mParent->GetFirstChild();
    mIsFirst = false;
  } else if (mChild) { 
    mChild = mChild->GetNextSibling();
  }

  
  while (mChild && mChild->IsActiveChildrenElement()) {
    XBLChildrenElement* point = static_cast<XBLChildrenElement*>(mChild);
    if (!point->mInsertedChildren.IsEmpty()) {
      mIndexInInserted = 1;
      return point->mInsertedChildren[0];
    }

    mDefaultChild = mChild->GetFirstChild();
    if (mDefaultChild) {
      return mDefaultChild;
    }

    mChild = mChild->GetNextSibling();
  }

  return mChild;
}

FlattenedChildIterator::FlattenedChildIterator(nsIContent* aParent)
  : ExplicitChildIterator(aParent), mXBLInvolved(false)
{
  nsXBLBinding* binding =
    aParent->OwnerDoc()->BindingManager()->GetBindingWithContent(aParent);

  if (binding) {
    nsIContent* anon = binding->GetAnonymousContent();
    if (anon) {
      mParent = anon;
      mXBLInvolved = true;
    }
  }

  
  
  
  if (!mXBLInvolved && aParent->GetBindingParent()) {
    for (nsIContent* child = aParent->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      if (child->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
        MOZ_ASSERT(child->GetBindingParent());
        mXBLInvolved = true;
        break;
      }
    }
  }
}

nsIContent* FlattenedChildIterator::Get()
{
  MOZ_ASSERT(!mIsFirst);

  if (mIndexInInserted) {
    XBLChildrenElement* point = static_cast<XBLChildrenElement*>(mChild);
    return point->mInsertedChildren[mIndexInInserted - 1];
  }
  return mDefaultChild ? mDefaultChild : mChild;
}

nsIContent* FlattenedChildIterator::GetPreviousChild()
{
  
  if (mIndexInInserted) {
    
    
    XBLChildrenElement* point = static_cast<XBLChildrenElement*>(mChild);
    if (--mIndexInInserted) {
      return point->mInsertedChildren[mIndexInInserted - 1];
    }
    mChild = mChild->GetPreviousSibling();
  } else if (mDefaultChild) {
    
    mDefaultChild = mDefaultChild->GetPreviousSibling();
    if (mDefaultChild) {
      return mDefaultChild;
    }

    mChild = mChild->GetPreviousSibling();
  } else if (mIsFirst) { 
    return nullptr;
  } else if (mChild) { 
    mChild = mChild->GetPreviousSibling();
  } else { 
    mChild = mParent->GetLastChild();
  }

  
  while (mChild && mChild->IsActiveChildrenElement()) {
    XBLChildrenElement* point = static_cast<XBLChildrenElement*>(mChild);
    if (!point->mInsertedChildren.IsEmpty()) {
      mIndexInInserted = point->InsertedChildrenLength();
      return point->mInsertedChildren[mIndexInInserted - 1];
    }

    mDefaultChild = mChild->GetLastChild();
    if (mDefaultChild) {
      return mDefaultChild;
    }

    mChild = mChild->GetPreviousSibling();
  }

  if (!mChild) {
    mIsFirst = true;
  }

  return mChild;
}

} 
} 
