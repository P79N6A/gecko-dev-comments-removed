




#include "TreeWalker.h"

#include "Accessible.h"
#include "nsAccessibilityService.h"
#include "DocAccessible.h"

#include "nsINodeList.h"

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
  TreeWalker(Accessible* aContext, nsIContent* aContent, bool aWalkCache) :
  mDoc(aContext->Document()), mContext(aContext),
  mWalkCache(aWalkCache), mState(nullptr)
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
    Accessible* accessible = mWalkCache ? mDoc->GetAccessible(childNode) :
      GetAccService()->GetOrCreateAccessible(childNode, mContext,
                                             &isSubtreeHidden);

    if (accessible)
      return accessible;

    
    if (!isSubtreeHidden) {
      if (!PushState(childNode))
        break;

      accessible = NextChildInternal(true);
      if (accessible)
        return accessible;
    }
  }

  
  PopState();

  return aNoWalkUp ? nullptr : NextChildInternal(false);
}

void
TreeWalker::PopState()
{
  WalkState* prevToLastState = mState->prevState;
  delete mState;
  mState = prevToLastState;
}

bool
TreeWalker::PushState(nsIContent* aContent)
{
  WalkState* nextToLastState = new WalkState(aContent);
  if (!nextToLastState)
    return false;

  nextToLastState->prevState = mState;
  mState = nextToLastState;

  return true;
}
