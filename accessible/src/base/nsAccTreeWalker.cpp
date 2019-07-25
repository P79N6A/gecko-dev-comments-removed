




#include "nsAccTreeWalker.h"

#include "nsAccessible.h"
#include "nsAccessibilityService.h"
#include "DocAccessible.h"

#include "nsINodeList.h"





struct WalkState
{
  WalkState(nsIContent *aContent) :
    content(aContent), childIdx(0), prevState(nsnull) {}

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsINodeList> childList;
  PRUint32 childIdx;
  WalkState *prevState;
};





nsAccTreeWalker::
  nsAccTreeWalker(DocAccessible* aDoc, nsIContent* aContent,
                  bool aWalkAnonContent, bool aWalkCache) :
  mDoc(aDoc), mWalkCache(aWalkCache), mState(nsnull)
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




nsAccessible*
nsAccTreeWalker::NextChildInternal(bool aNoWalkUp)
{
  if (!mState || !mState->content)
    return nsnull;

  if (!mState->childList)
    mState->childList = mState->content->GetChildren(mChildFilter);

  PRUint32 length = 0;
  if (mState->childList)
    mState->childList->GetLength(&length);

  while (mState->childIdx < length) {
    nsIContent* childNode = mState->childList->GetNodeAt(mState->childIdx);
    mState->childIdx++;

    bool isSubtreeHidden = false;
    nsAccessible* accessible = mWalkCache ? mDoc->GetAccessible(childNode) :
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

  return aNoWalkUp ? nsnull : NextChildInternal(false);
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
