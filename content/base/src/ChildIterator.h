





#ifndef ChildIterator_h
#define ChildIterator_h










#include "nsIContent.h"

namespace mozilla {
namespace dom {





class ExplicitChildIterator
{
public:
  ExplicitChildIterator(nsIContent* aParent)
    : mParent(aParent),
      mChild(nullptr),
      mDefaultChild(nullptr),
      mIndexInInserted(0),
      mIsFirst(true)
  {
  }

  nsIContent* GetNextChild();

  
  
  
  
  bool Seek(nsIContent* aChildToFind, nsIContent* aBound = nullptr)
  {
    
    
    

    nsIContent* child;
    do {
      child = GetNextChild();
    } while (child && child != aChildToFind && child != aBound);

    return child == aChildToFind;
  }

protected:
  
  
  
  nsIContent* mParent;

  
  
  
  
  nsIContent* mChild;

  
  
  
  
  nsIContent* mDefaultChild;

  
  
  
  
  uint32_t mIndexInInserted;

  
  bool mIsFirst;
};




class FlattenedChildIterator : public ExplicitChildIterator
{
public:
  FlattenedChildIterator(nsIContent* aParent);

  
  
  
  nsIContent* Get();

  
  
  nsIContent* GetPreviousChild();

  bool XBLInvolved() { return mXBLInvolved; }

private:
  
  
  bool mXBLInvolved;
};

} 
} 

#endif
