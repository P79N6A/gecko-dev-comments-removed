






































#ifndef _nsAccTreeWalker_H_
#define _nsAccTreeWalker_H_

#include "nsIAccessible.h"
#include "nsIContent.h"
#include "nsIWeakReference.h"

struct WalkState;




class nsAccTreeWalker
{
public:
  nsAccTreeWalker(nsIWeakReference *aShell, nsIContent *aNode, 
                  PRBool aWalkAnonymousContent);
  virtual ~nsAccTreeWalker();

  


  already_AddRefed<nsIAccessible> GetNextChild()
  {
    return GetNextChildInternal(PR_FALSE);
  }

private:

  






  already_AddRefed<nsIAccessible>
    GetNextChildInternal(PRBool aNoWalkUp = PR_FALSE);

  





  PRBool PushState(nsIContent *aNode);

  


  void PopState();

  nsCOMPtr<nsIWeakReference> mWeakShell;
  PRInt32 mChildType;
  WalkState* mState;
};

#endif 
