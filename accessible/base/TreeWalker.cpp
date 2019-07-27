




#include "TreeWalker.h"

#include "Accessible.h"
#include "nsAccessibilityService.h"
#include "DocAccessible.h"

#include "mozilla/dom/ChildIterator.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;





TreeWalker::
  TreeWalker(Accessible* aContext, nsIContent* aContent, uint32_t aFlags) :
  mDoc(aContext->Document()), mContext(aContext), mAnchorNode(aContent),
  mFlags(aFlags)
{
  NS_ASSERTION(aContent, "No node for the accessible tree walker!");

  mChildFilter = mContext->CanHaveAnonChildren() ?
    nsIContent::eAllChildren : nsIContent::eAllButXBL;
  mChildFilter |= nsIContent::eSkipPlaceholderContent;

  if (aContent)
    PushState(aContent);

  MOZ_COUNT_CTOR(TreeWalker);
}

TreeWalker::~TreeWalker()
{
  MOZ_COUNT_DTOR(TreeWalker);
}




Accessible*
TreeWalker::NextChild()
{
  if (mStateStack.IsEmpty())
    return nullptr;

  dom::AllChildrenIterator* top = &mStateStack[mStateStack.Length() - 1];
  while (top) {
    while (nsIContent* childNode = top->GetNextChild()) {
      bool isSubtreeHidden = false;
      Accessible* accessible = mFlags & eWalkCache ?
        mDoc->GetAccessible(childNode) :
        GetAccService()->GetOrCreateAccessible(childNode, mContext,
                                               &isSubtreeHidden);

      if (accessible)
        return accessible;

      
      if (!isSubtreeHidden && childNode->IsElement())
        top = PushState(childNode);
    }

    top = PopState();
  }

  
  
  if (mFlags != eWalkContextTree)
    return nullptr;

  nsINode* contextNode = mContext->GetNode();
  while (mAnchorNode != contextNode) {
    nsINode* parentNode = mAnchorNode->GetFlattenedTreeParent();
    if (!parentNode || !parentNode->IsElement())
      return nullptr;

    nsIContent* parent = parentNode->AsElement();
    top = mStateStack.AppendElement(dom::AllChildrenIterator(parent,
                                                             mChildFilter));
    while (nsIContent* childNode = top->GetNextChild()) {
      if (childNode == mAnchorNode) {
        mAnchorNode = parent;
        return NextChild();
      }
    }

    
    
    
    
    mAnchorNode = parent;
  }

  return nullptr;
}

dom::AllChildrenIterator*
TreeWalker::PopState()
{
  size_t length = mStateStack.Length();
  mStateStack.RemoveElementAt(length - 1);
  return mStateStack.IsEmpty() ? nullptr : &mStateStack[mStateStack.Length() - 1];
}
