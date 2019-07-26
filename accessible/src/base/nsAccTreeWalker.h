




#ifndef _nsAccTreeWalker_H_
#define _nsAccTreeWalker_H_

#include "nsAutoPtr.h"
#include "nsIContent.h"

namespace mozilla {
namespace a11y {

class Accessible;
class DocAccessible;

} 
} 

struct WalkState;




class nsAccTreeWalker
{
public:
  typedef mozilla::a11y::Accessible Accessible;
  typedef mozilla::a11y::DocAccessible DocAccessible;

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
