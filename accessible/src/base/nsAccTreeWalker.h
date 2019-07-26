




#ifndef _nsAccTreeWalker_H_
#define _nsAccTreeWalker_H_

#include "nsAutoPtr.h"
#include "nsIContent.h"

class Accessible;
class DocAccessible;
struct WalkState;




class nsAccTreeWalker
{
public:
  nsAccTreeWalker(DocAccessible* aDoc, Accessible* aContext, nsIContent* aNode,
                  bool aWalkCache = false);
  virtual ~nsAccTreeWalker();

  






  Accessible* NextChild()
  {
    return NextChildInternal(false);
  }

private:

  






  Accessible* NextChildInternal(bool aNoWalkUp);

  





  bool PushState(nsIContent *aNode);

  


  void PopState();

  DocAccessible* mDoc;
  Accessible* mContext;
  int32_t mChildFilter;
  bool mWalkCache;
  WalkState* mState;
};

#endif 
