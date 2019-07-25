






































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
                  PRBool aWalkAnonymousContent, bool aWalkCache = false);
  virtual ~nsAccTreeWalker();

  






  inline nsAccessible* NextChild()
  {
    return NextChildInternal(false);
  }

private:

  






  nsAccessible* NextChildInternal(bool aNoWalkUp);

  





  PRBool PushState(nsIContent *aNode);

  


  void PopState();

  nsCOMPtr<nsIWeakReference> mWeakShell;
  PRInt32 mChildFilter;
  bool mWalkCache;
  WalkState* mState;
};

#endif 
