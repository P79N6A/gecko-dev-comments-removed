





































#include "nsAccessibleTreeWalker.h"

#include "nsAccessibilityAtoms.h"
#include "nsAccessNode.h"

#include "nsIAnonymousContentCreator.h"
#include "nsIServiceManager.h"
#include "nsIContent.h"
#include "nsIDOMXULElement.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsWeakReference.h"

nsAccessibleTreeWalker::nsAccessibleTreeWalker(nsIWeakReference* aPresShell, nsIDOMNode* aNode, PRBool aWalkAnonContent): 
  mWeakShell(aPresShell), 
  mAccService(do_GetService("@mozilla.org/accessibilityService;1")),
  mWalkAnonContent(aWalkAnonContent)
{
  mState.domNode = aNode;
  mState.prevState = nsnull;
  mState.siblingIndex = eSiblingsUninitialized;
  mState.siblingList = nsnull;
  mState.isHidden = false;
  mState.frame = nsnull;

  MOZ_COUNT_CTOR(nsAccessibleTreeWalker);
}

nsAccessibleTreeWalker::~nsAccessibleTreeWalker()
{
  
  while (NS_SUCCEEDED(PopState()))
     ;
   MOZ_COUNT_DTOR(nsAccessibleTreeWalker);
}

void nsAccessibleTreeWalker::GetKids(nsIDOMNode *aParentNode)
{
  nsCOMPtr<nsIContent> parentContent(do_QueryInterface(aParentNode));
  if (!parentContent || !parentContent->IsNodeOfType(nsINode::eHTML)) {
    mState.frame = nsnull;  
  }

  UpdateFrame(PR_TRUE);

  
  if (mState.siblingIndex == eSiblingsWalkFrames) {
    return;
  }

  
  mState.siblingIndex = 0;   
  if (parentContent) {
    if (mWalkAnonContent) {
      
      nsIDocument* doc = parentContent->GetOwnerDoc();
      if (doc) {
        
        doc->GetXBLChildNodesFor(parentContent,
                                 getter_AddRefs(mState.siblingList));
      }
    }
    if (!mState.siblingList) {
      
      
      
      mState.parentContent = parentContent;
      mState.domNode = do_QueryInterface(parentContent->GetChildAt(0 ));
      return;
    }
  }
  else {
    
    
    aParentNode->GetChildNodes(getter_AddRefs(mState.siblingList));
    if (!mState.siblingList) {
      return;
    }
  }

  mState.siblingList->Item(0 , getter_AddRefs(mState.domNode));
}

NS_IMETHODIMP nsAccessibleTreeWalker::PopState()
{
  nsIFrame *frameParent = mState.frame? mState.frame->GetParent(): nsnull;
  if (mState.prevState) {
    WalkState *toBeDeleted = mState.prevState;
    mState = *mState.prevState; 
    mState.isHidden = PR_FALSE; 
    if (!mState.frame) {
      mState.frame = frameParent;
    }
    delete toBeDeleted;
    return NS_OK;
  }
  ClearState();
  mState.frame = frameParent;
  mState.isHidden = PR_FALSE;
  return NS_ERROR_FAILURE;
}

void nsAccessibleTreeWalker::ClearState()
{
  mState.siblingList = nsnull;
  mState.parentContent = nsnull;
  mState.accessible = nsnull;
  mState.domNode = nsnull;
  mState.siblingIndex = eSiblingsUninitialized;
}

NS_IMETHODIMP nsAccessibleTreeWalker::PushState()
{
  
  WalkState* nextToLastState= new WalkState();
  if (!nextToLastState)
    return NS_ERROR_OUT_OF_MEMORY;
  *nextToLastState = mState;  
  ClearState();
  mState.prevState = nextToLastState;   
  return NS_OK;
}

void nsAccessibleTreeWalker::GetNextDOMNode()
{
  
  if (mState.parentContent) {
    mState.domNode = do_QueryInterface(mState.parentContent->GetChildAt(++mState.siblingIndex));
  }
  else if (mState.siblingIndex == eSiblingsWalkFrames) {
    if (mState.frame) {
      mState.domNode = do_QueryInterface(mState.frame->GetContent());
    } else {
      mState.domNode = nsnull;
    }
  }
  else { 
    mState.siblingList->Item(++mState.siblingIndex, getter_AddRefs(mState.domNode));
  }
}

NS_IMETHODIMP nsAccessibleTreeWalker::GetNextSibling()
{
  
  NS_ASSERTION(mState.prevState && mState.siblingIndex != eSiblingsUninitialized,
               "Error - GetNextSibling() only works after a GetFirstChild(), so we must have a prevState.");
  mState.accessible = nsnull;

  while (PR_TRUE) {
    
    UpdateFrame(PR_FALSE);
    GetNextDOMNode();

    if (!mState.domNode) {  
      PopState();   
      if (!mState.prevState) {
        mState.accessible = nsnull;
        break; 
      }
    }
    else if ((mState.domNode != mState.prevState->domNode && GetAccessible()) || 
             NS_SUCCEEDED(GetFirstChild())) {
      return NS_OK; 
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAccessibleTreeWalker::GetFirstChild()
{
  mState.accessible = nsnull;
  if (mState.isHidden || !mState.domNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> parent(mState.domNode);

  PushState();
  GetKids(parent); 

  
  while (mState.domNode) {
    if ((mState.domNode != parent && GetAccessible()) || NS_SUCCEEDED(GetFirstChild()))
      return NS_OK;
    UpdateFrame(PR_FALSE);
    GetNextDOMNode();
  }

  PopState();  
  return NS_ERROR_FAILURE;
}

void nsAccessibleTreeWalker::UpdateFrame(PRBool aTryFirstChild)
{
  if (!mState.frame) {
    return;
  }

  if (aTryFirstChild) {
    
    
    nsIAnonymousContentCreator* creator = do_QueryFrame(mState.frame);
    mState.frame = mState.frame->GetFirstChild(nsnull);

    if (creator && mState.frame && mState.siblingIndex < 0) {
      mState.domNode = do_QueryInterface(mState.frame->GetContent());
      mState.siblingIndex = eSiblingsWalkFrames;
    }


#if 0
    if (mState.frame && mState.siblingIndex < 0) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      mState.domNode = do_QueryInterface(mState.frame->GetContent());
      mState.siblingIndex = eSiblingsWalkFrames;
    }
#endif
  }
  else {
    mState.frame = mState.frame->GetNextSibling();
  }
}





PRBool nsAccessibleTreeWalker::GetAccessible()
{
  if (!mAccService) {
    return PR_FALSE;
  }

  mState.accessible = nsnull;
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));

  mAccService->GetAccessible(mState.domNode, presShell, mWeakShell,
                             &mState.frame, &mState.isHidden,
                             getter_AddRefs(mState.accessible));
  return mState.accessible ? PR_TRUE : PR_FALSE;
}

