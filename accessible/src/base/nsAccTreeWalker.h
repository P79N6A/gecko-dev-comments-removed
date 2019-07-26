




#ifndef _nsAccTreeWalker_H_
#define _nsAccTreeWalker_H_

#include "nsAutoPtr.h"
#include "nsIContent.h"

class nsAccessible;
class nsDocAccessible;
struct WalkState;




class nsAccTreeWalker
{
public:
  nsAccTreeWalker(nsDocAccessible* aDoc, nsIContent* aNode, 
                  bool aWalkAnonymousContent, bool aWalkCache = false);
  virtual ~nsAccTreeWalker();

  






  inline nsAccessible* NextChild()
  {
    return NextChildInternal(false);
  }

private:

  






  nsAccessible* NextChildInternal(bool aNoWalkUp);

  





  bool PushState(nsIContent *aNode);

  


  void PopState();

  nsDocAccessible* mDoc;
  PRInt32 mChildFilter;
  bool mWalkCache;
  WalkState* mState;
};

#endif 
