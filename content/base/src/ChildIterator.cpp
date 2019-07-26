





#include "ChildIterator.h"
#include "nsContentUtils.h"
#include "mozilla/dom/XBLChildrenElement.h"
#include "mozilla/dom/HTMLContentElement.h"
#include "mozilla/dom/HTMLShadowElement.h"
#include "mozilla/dom/ShadowRoot.h"

namespace mozilla {
namespace dom {

class MatchedNodes {
public:
  MatchedNodes(HTMLContentElement* aInsertionPoint)
    : mIsContentElement(true), mContentElement(aInsertionPoint) {}

  MatchedNodes(XBLChildrenElement* aInsertionPoint)
    : mIsContentElement(false), mChildrenElement(aInsertionPoint) {}

  uint32_t Length() const
  {
    return mIsContentElement ? mContentElement->MatchedNodes().Length()
                             : mChildrenElement->mInsertedChildren.Length();
  }

  nsIContent* operator[](int32_t aIndex) const
  {
    return mIsContentElement ? mContentElement->MatchedNodes()[aIndex]
                             : mChildrenElement->mInsertedChildren[aIndex];
  }

  bool IsEmpty() const
  {
    return mIsContentElement ? mContentElement->MatchedNodes().IsEmpty()
                             : mChildrenElement->mInsertedChildren.IsEmpty();
  }
protected:
  bool mIsContentElement;
  union {
    HTMLContentElement* mContentElement;
    XBLChildrenElement* mChildrenElement;
  };
};

static inline MatchedNodes
GetMatchedNodesForPoint(nsIContent* aContent)
{
  if (aContent->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
    
    return MatchedNodes(static_cast<XBLChildrenElement*>(aContent));
  }

  
  MOZ_ASSERT(aContent->IsHTML(nsGkAtoms::content));
  return MatchedNodes(static_cast<HTMLContentElement*>(aContent));
}

nsIContent*
ExplicitChildIterator::GetNextChild()
{
  
  if (mIndexInInserted) {
    MOZ_ASSERT(mChild);
    MOZ_ASSERT(nsContentUtils::IsContentInsertionPoint(mChild));
    MOZ_ASSERT(!mDefaultChild);

    MatchedNodes assignedChildren = GetMatchedNodesForPoint(mChild);
    if (mIndexInInserted < assignedChildren.Length()) {
      return assignedChildren[mIndexInInserted++];
    }
    mIndexInInserted = 0;
    mChild = mChild->GetNextSibling();
  } else if (mShadowIterator) {
    
    
    
    nsIContent* nextChild = mShadowIterator->GetNextChild();
    if (nextChild) {
      return nextChild;
    }

    mShadowIterator = nullptr;
    mChild = mChild->GetNextSibling();
  } else if (mDefaultChild) {
    
    MOZ_ASSERT(mChild);
    MOZ_ASSERT(nsContentUtils::IsContentInsertionPoint(mChild));

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

  
  
  while (mChild) {
    
    
    if (ShadowRoot::IsShadowInsertionPoint(mChild)) {
      
      
      HTMLShadowElement* shadowElem = static_cast<HTMLShadowElement*>(mChild);
      ShadowRoot* projectedShadow = shadowElem->GetOlderShadowRoot();
      if (projectedShadow) {
        mShadowIterator = new ExplicitChildIterator(projectedShadow);
        nsIContent* nextChild = mShadowIterator->GetNextChild();
        if (nextChild) {
          return nextChild;
        }
        mShadowIterator = nullptr;
      }
      mChild = mChild->GetNextSibling();
    } else if (nsContentUtils::IsContentInsertionPoint(mChild)) {
      
      
      
      MatchedNodes assignedChildren = GetMatchedNodesForPoint(mChild);
      if (!assignedChildren.IsEmpty()) {
        
        mIndexInInserted = 1;
        return assignedChildren[0];
      }

      
      
      mDefaultChild = mChild->GetFirstChild();
      if (mDefaultChild) {
        return mDefaultChild;
      }

      
      
      mChild = mChild->GetNextSibling();
    } else {
      
      
      break;
    }
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

nsIContent*
ExplicitChildIterator::Get()
{
  MOZ_ASSERT(!mIsFirst);

  if (mIndexInInserted) {
    XBLChildrenElement* point = static_cast<XBLChildrenElement*>(mChild);
    return point->mInsertedChildren[mIndexInInserted - 1];
  } else if (mShadowIterator)  {
    return mShadowIterator->Get();
  }
  return mDefaultChild ? mDefaultChild : mChild;
}

nsIContent*
ExplicitChildIterator::GetPreviousChild()
{
  
  if (mIndexInInserted) {
    
    
    MatchedNodes assignedChildren = GetMatchedNodesForPoint(mChild);
    if (--mIndexInInserted) {
      return assignedChildren[mIndexInInserted - 1];
    }
    mChild = mChild->GetPreviousSibling();
  } else if (mShadowIterator) {
    nsIContent* previousChild = mShadowIterator->GetPreviousChild();
    if (previousChild) {
      return previousChild;
    }
    mShadowIterator = nullptr;
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

  
  
  while (mChild) {
    if (ShadowRoot::IsShadowInsertionPoint(mChild)) {
      
      
      HTMLShadowElement* shadowElem = static_cast<HTMLShadowElement*>(mChild);
      ShadowRoot* projectedShadow = shadowElem->GetOlderShadowRoot();
      if (projectedShadow) {
        
        mShadowIterator = new ExplicitChildIterator(projectedShadow, false);
        nsIContent* previousChild = mShadowIterator->GetPreviousChild();
        if (previousChild) {
          return previousChild;
        }
        mShadowIterator = nullptr;
      }
      mChild = mChild->GetPreviousSibling();
    } else if (nsContentUtils::IsContentInsertionPoint(mChild)) {
      
      
      
      MatchedNodes assignedChildren = GetMatchedNodesForPoint(mChild);
      if (!assignedChildren.IsEmpty()) {
        mIndexInInserted = assignedChildren.Length();
        return assignedChildren[mIndexInInserted - 1];
      }

      mDefaultChild = mChild->GetLastChild();
      if (mDefaultChild) {
        return mDefaultChild;
      }

      mChild = mChild->GetPreviousSibling();
    } else {
      
      
      break;
    }
  }

  if (!mChild) {
    mIsFirst = true;
  }

  return mChild;
}

} 
} 
