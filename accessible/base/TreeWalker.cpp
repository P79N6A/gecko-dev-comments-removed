




#include "TreeWalker.h"

#include "Accessible.h"
#include "nsAccessibilityService.h"
#include "DocAccessible.h"

#include "mozilla/dom/ChildIterator.h"
#include "mozilla/dom/Element.h"

using namespace mozilla::a11y;





namespace mozilla {
namespace a11y {

struct WalkState
{
  WalkState(nsIContent *aContent, uint32_t aFilter) :
    content(aContent), prevState(nullptr), iter(aContent, aFilter) {}

  nsCOMPtr<nsIContent> content;
  WalkState *prevState;
  dom::AllChildrenIterator iter;
};

} 
} 





TreeWalker::
  TreeWalker(Accessible* aContext, nsIContent* aContent, uint32_t aFlags) :
  mDoc(aContext->Document()), mContext(aContext),
  mFlags(aFlags), mState(nullptr)
{
  NS_ASSERTION(aContent, "No node for the accessible tree walker!");

  mChildFilter = mContext->CanHaveAnonChildren() ?
    nsIContent::eAllChildren : nsIContent::eAllButXBL;
  mChildFilter |= nsIContent::eSkipPlaceholderContent;

  if (aContent)
    mState = new WalkState(aContent, mChildFilter);

  MOZ_COUNT_CTOR(TreeWalker);
}

TreeWalker::~TreeWalker()
{
  
  while (mState)
    PopState();

  MOZ_COUNT_DTOR(TreeWalker);
}




Accessible*
TreeWalker::NextChildInternal(bool aNoWalkUp)
{
  if (!mState || !mState->content)
    return nullptr;

  while (nsIContent* childNode = mState->iter.GetNextChild()) {
    bool isSubtreeHidden = false;
    Accessible* accessible = mFlags & eWalkCache ?
      mDoc->GetAccessible(childNode) :
      GetAccService()->GetOrCreateAccessible(childNode, mContext,
                                             &isSubtreeHidden);

    if (accessible)
      return accessible;

    
    if (!isSubtreeHidden && childNode->IsElement()) {
      PushState(childNode);
      accessible = NextChildInternal(true);
      if (accessible)
        return accessible;
    }
  }

  
  nsIContent* anchorNode = mState->content;
  PopState();
  if (aNoWalkUp)
    return nullptr;

  if (mState)
    return NextChildInternal(false);

  
  
  if (mFlags != eWalkContextTree)
    return nullptr;

  while (anchorNode != mContext->GetNode()) {
    nsINode* parentNode = anchorNode->GetFlattenedTreeParent();
    if (!parentNode || !parentNode->IsElement())
      return nullptr;

    PushState(parentNode->AsElement());
    while (nsIContent* childNode = mState->iter.GetNextChild()) {
      if (childNode == anchorNode)
        return NextChildInternal(false);
    }
    PopState();

    anchorNode = parentNode->AsElement();
  }

  return nullptr;
}

void
TreeWalker::PopState()
{
  WalkState* prevToLastState = mState->prevState;
  delete mState;
  mState = prevToLastState;
}

void
TreeWalker::PushState(nsIContent* aContent)
{
  WalkState* nextToLastState = new WalkState(aContent, mChildFilter);
  nextToLastState->prevState = mState;
  mState = nextToLastState;
}
