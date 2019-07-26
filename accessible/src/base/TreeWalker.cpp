




#include "TreeWalker.h"

#include "Accessible.h"
#include "nsAccessibilityService.h"
#include "DocAccessible.h"

#include "mozilla/dom/Element.h"

using namespace mozilla::a11y;





namespace mozilla {
namespace a11y {

struct WalkState
{
  WalkState(nsIContent *aContent) :
    content(aContent), childIdx(0), prevState(nullptr) {}

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsINodeList> childList;
  uint32_t childIdx;
  WalkState *prevState;
};

} 
} 





TreeWalker::
  TreeWalker(Accessible* aContext, nsIContent* aContent, uint32_t aFlags) :
  mDoc(aContext->Document()), mContext(aContext),
  mFlags(aFlags), mState(nullptr)
{
  NS_ASSERTION(aContent, "No node for the accessible tree walker!");

  if (aContent)
    mState = new WalkState(aContent);

  mChildFilter = mContext->CanHaveAnonChildren() ?
    nsIContent::eAllChildren : nsIContent::eAllButXBL;

  mChildFilter |= nsIContent::eSkipPlaceholderContent;

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

  if (!mState->childList)
    mState->childList = mState->content->GetChildren(mChildFilter);

  uint32_t length = 0;
  if (mState->childList)
    mState->childList->GetLength(&length);

  while (mState->childIdx < length) {
    nsIContent* childNode = mState->childList->Item(mState->childIdx);
    mState->childIdx++;

    bool isSubtreeHidden = false;
    Accessible* accessible = mFlags & eWalkCache ?
      mDoc->GetAccessible(childNode) :
      GetAccService()->GetOrCreateAccessible(childNode, mContext,
                                             &isSubtreeHidden);

    if (accessible)
      return accessible;

    
    if (!isSubtreeHidden) {
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
    mState->childList = mState->content->GetChildren(mChildFilter);
    length = 0;
    if (mState->childList)
      mState->childList->GetLength(&length);

    while (mState->childIdx < length) {
      nsIContent* childNode = mState->childList->Item(mState->childIdx);
      mState->childIdx++;
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
  WalkState* nextToLastState = new WalkState(aContent);
  nextToLastState->prevState = mState;
  mState = nextToLastState;
}
