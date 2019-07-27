




#ifndef mozilla_a11y_TreeWalker_h_
#define mozilla_a11y_TreeWalker_h_

#include "mozilla/Attributes.h"
#include <stdint.h>
#include "mozilla/dom/ChildIterator.h"
#include "nsCOMPtr.h"

class nsIContent;

namespace mozilla {
namespace a11y {

class Accessible;
class DocAccessible;




class TreeWalker MOZ_FINAL
{
public:
  enum {
    
    eWalkCache = 1,
    
    eWalkContextTree = 2 | eWalkCache
  };

  







  TreeWalker(Accessible* aContext, nsIContent* aNode, uint32_t aFlags = 0);
  ~TreeWalker();

  






  Accessible* NextChild();

private:
  TreeWalker();
  TreeWalker(const TreeWalker&);
  TreeWalker& operator =(const TreeWalker&);

  





  dom::AllChildrenIterator* PushState(nsIContent* aContent)
  {
    return mStateStack.AppendElement(dom::AllChildrenIterator(aContent,
                                                              mChildFilter));
  }

  


  dom::AllChildrenIterator* PopState();

  DocAccessible* mDoc;
  Accessible* mContext;
  nsIContent* mAnchorNode;
  nsAutoTArray<dom::AllChildrenIterator, 20> mStateStack;
  int32_t mChildFilter;
  uint32_t mFlags;
};

} 
} 

#endif
