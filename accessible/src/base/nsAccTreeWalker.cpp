




#include "nsAccTreeWalker.h"

#include "Accessible.h"
#include "nsAccessibilityService.h"
#include "DocAccessible.h"

#include "nsINodeList.h"





struct WalkState
{
  WalkState(nsIContent *aContent) :
    content(aContent), childIdx(0), prevState(nullptr) {}

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsINodeList> childList;
  uint32_t childIdx;
  WalkState *prevState;
};





nsAccTreeWalker::
  nsAccTreeWalker(DocAccessible* aDoc, nsIContent* aContent,
                  bool aWalkAnonContent, bool aWalkCache) :
  mDoc(aDoc), mWalkCache(aWalkCache), mState(nullptr)
{
  NS_ASSERTION(aContent, "No node for the accessible tree walker!");

  if (aContent)
    mState = new WalkState(aContent);

  mChildFilter = aWalkAnonContent ? nsIContent::eAllChildren :
                                  nsIContent::eAllButXBL;

  mChildFilter |= nsIContent::eSkipPlaceholderContent;

  MOZ_COUNT_CTOR(nsAccTreeWalker);
}

nsAccTreeWalker::~nsAccTreeWalker()
{
  
  while (mState)
    PopState();

  MOZ_COUNT_DTOR(nsAccTreeWalker);
}




Accessible*
nsAccTreeWalker::NextChildInternal(bool aNoWalkUp)
{
  if (!mState || !mState->content)
    return nullptr;

  if (!mState->childList)
    mState->childList = mState->content->GetChildren(mChildFilter);

  uint32_t length = 0;
  if (mState->childList)
    mState->childList->GetLength(&length);

  while (mState->childIdx < length) {
    nsIContent* childNode = mState->childList->GetNodeAt(mState->childIdx);
    mState->childIdx++;

    bool isSubtreeHidden = false;
    Accessible* accessible = mWalkCache ? mDoc->GetAccessible(childNode) :
      GetAccService()->GetOrCreateAccessible(childNode, mDoc, &isSubtreeHidden);

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
nsAccTreeWalker::PopState()
{
  WalkState* prevToLastState = mState->prevState;
  delete mState;
  mState = prevToLastState;
}

bool
nsAccTreeWalker::PushState(nsIContent* aContent)
{
  WalkState* nextToLastState = new WalkState(aContent);
  if (!nextToLastState)
    return false;

  nextToLastState->prevState = mState;
  mState = nextToLastState;

  return true;
}
