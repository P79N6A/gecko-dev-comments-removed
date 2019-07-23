






































#include "nsAccTreeWalker.h"

#include "nsAccessibilityService.h"





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
  nsAccTreeWalker(nsIWeakReference* aShell, nsIContent* aContent,
                  PRBool aWalkAnonContent) :
  mWeakShell(aShell), mState(nsnull)
{
  NS_ASSERTION(aContent, "No node for the accessible tree walker!");

  if (aContent)
    mState = new WalkState(aContent);

  mChildType = aWalkAnonContent ? nsIContent::eAllChildren :
                                  nsIContent::eAllButXBL;

  MOZ_COUNT_CTOR(nsAccTreeWalker);
}

nsAccTreeWalker::~nsAccTreeWalker()
{
  
  while (mState)
    PopState();

  MOZ_COUNT_DTOR(nsAccTreeWalker);
}




already_AddRefed<nsIAccessible>
nsAccTreeWalker::GetNextChildInternal(PRBool aNoWalkUp)
{
  if (!mState || !mState->content)
    return nsnull;

  if (!mState->childList)
    mState->childList = mState->content->GetChildren(mChildType);

  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));

  PRUint32 length = 0;
  if (mState->childList)
    mState->childList->GetLength(&length);

  while (mState->childIdx < length;) {
    nsIContent* childNode = mState->childList->GetNodeAt(mState->childIdx);
    mState->childIdx++;

    PRBool isHidden = PR_FALSE;
    nsCOMPtr<nsIAccessible> accessible;
    nsCOMPtr<nsIDOMNode> childDOMNode(do_QueryInterface(childNode));
    GetAccService()->GetAccessible(childDOMNode, presShell, mWeakShell, nsnull,
                                   &isHidden, getter_AddRefs(accessible));

    if (accessible)
      return accessible.forget();

    
    if (!isHidden) {
      if (!PushState(childNode))
        break;

      accessible = GetNextChildInternal(PR_TRUE);
      if (accessible)
        return accessible.forget();
    }
  }

  
  PopState();

  return aNoWalkUp ? nsnull : GetNextChildInternal(PR_FALSE);
}

void
nsAccTreeWalker::PopState()
{
  WalkState* prevToLastState = mState->prevState;
  delete mState;
  mState = prevToLastState;
}

PRBool
nsAccTreeWalker::PushState(nsIContent* aContent)
{
  WalkState* nextToLastState = new WalkState(aContent);
  if (!nextToLastState)
    return PR_FALSE;

  nextToLastState->prevState = mState;
  mState = nextToLastState;

  return PR_TRUE;
}
