






































#ifndef _nsAccTreeWalker_H_
#define _nsAccTreeWalker_H_

#include "nsAutoPtr.h"
#include "nsIContent.h"
#include "nsIWeakReference.h"

class nsAccessible;
struct WalkState;




class nsAccTreeWalker
{
public:
  nsAccTreeWalker(nsIWeakReference *aShell, nsIContent *aNode, 
                  PRBool aWalkAnonymousContent);
  virtual ~nsAccTreeWalker();

  


  already_AddRefed<nsAccessible> GetNextChild()
  {
    return GetNextChildInternal(PR_FALSE);
  }

private:

  






  already_AddRefed<nsAccessible>
    GetNextChildInternal(PRBool aNoWalkUp = PR_FALSE);

  





  PRBool PushState(nsIContent *aNode);

  


  void PopState();

  nsCOMPtr<nsIWeakReference> mWeakShell;
  PRInt32 mChildFilter;
  WalkState* mState;
};

#endif 
