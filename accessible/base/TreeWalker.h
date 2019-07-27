




#ifndef mozilla_a11y_TreeWalker_h_
#define mozilla_a11y_TreeWalker_h_

#include "mozilla/Attributes.h"
#include <stdint.h>

class nsIContent;

namespace mozilla {
namespace a11y {

class Accessible;
class DocAccessible;

struct WalkState;




class TreeWalker MOZ_FINAL
{
public:
  enum {
    
    eWalkCache = 1,
    
    eWalkContextTree = 2 | eWalkCache
  };

  







  TreeWalker(Accessible* aContext, nsIContent* aNode, uint32_t aFlags = 0);
  ~TreeWalker();

  






  Accessible* NextChild()
  {
    return NextChildInternal(false);
  }

private:
  TreeWalker();
  TreeWalker(const TreeWalker&);
  TreeWalker& operator =(const TreeWalker&);

  






  Accessible* NextChildInternal(bool aNoWalkUp);

  





  void PushState(nsIContent* aNode);

  


  void PopState();

  DocAccessible* mDoc;
  Accessible* mContext;
  int32_t mChildFilter;
  uint32_t mFlags;
  WalkState* mState;
};

} 
} 

#endif
