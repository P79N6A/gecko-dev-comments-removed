




#ifndef mozilla_a11y_TreeWalker_h_
#define mozilla_a11y_TreeWalker_h_

#include "nsAutoPtr.h"

class nsIContent;

namespace mozilla {
namespace a11y {

class Accessible;
class DocAccessible;

struct WalkState;




class TreeWalker
{
public:
  TreeWalker(Accessible* aContext, nsIContent* aNode, bool aWalkCache = false);
  virtual ~TreeWalker();

  






  Accessible* NextChild()
  {
    return NextChildInternal(false);
  }

private:
  TreeWalker();
  TreeWalker(const TreeWalker&);
  TreeWalker& operator =(const TreeWalker&);

  






  Accessible* NextChildInternal(bool aNoWalkUp);

  





  bool PushState(nsIContent *aNode);

  


  void PopState();

  DocAccessible* mDoc;
  Accessible* mContext;
  int32_t mChildFilter;
  bool mWalkCache;
  WalkState* mState;
};

} 
} 

#endif 
