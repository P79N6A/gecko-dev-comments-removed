





































#ifndef _nsAccessibleTreeWalker_H_
#define _nsAccessibleTreeWalker_H_





#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIAccessible.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIAccessibilityService.h"
#include "nsIWeakReference.h"
#include "nsIFrame.h"

enum { eSiblingsUninitialized = -1, eSiblingsWalkFrames = -2 };

struct WalkState {
  nsCOMPtr<nsIAccessible> accessible;
  nsCOMPtr<nsIDOMNode> domNode;
  nsCOMPtr<nsIDOMNodeList> siblingList;
  nsIContent *parentContent; 
  WalkState *prevState;
  nsWeakFrame frame;       
  PRInt32 siblingIndex;    
  PRBool isHidden;         
};
 





class nsAccessibleTreeWalker {
public:
  nsAccessibleTreeWalker(nsIWeakReference* aShell, nsIDOMNode* aContent, 
    PRBool mWalkAnonymousContent);
  virtual ~nsAccessibleTreeWalker();

  


  NS_IMETHOD GetNextSibling();

  


  NS_IMETHOD GetFirstChild();

  



  WalkState mState;

protected:

  


  PRBool GetAccessible();

  


  void GetKids(nsIDOMNode *aParent);

  


  void ClearState();

  



  NS_IMETHOD PushState();

  


  NS_IMETHOD PopState();

  


  void WalkFrames();

  


  void GetNextDOMNode();

  nsCOMPtr<nsIWeakReference> mWeakShell;
  nsCOMPtr<nsIAccessibilityService> mAccService;
  PRBool mWalkAnonContent;
};

#endif 
