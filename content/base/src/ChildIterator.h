





#ifndef ChildIterator_h
#define ChildIterator_h










#include "nsIContent.h"

namespace mozilla {
namespace dom {







class ExplicitChildIterator
{
public:
  ExplicitChildIterator(nsIContent* aParent, bool aStartAtBeginning = true)
    : mParent(aParent),
      mChild(nullptr),
      mDefaultChild(nullptr),
      mIndexInInserted(0),
      mIsFirst(aStartAtBeginning)
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

  
  
  
  nsIContent* Get();

  
  
  nsIContent* GetPreviousChild();

protected:
  
  
  
  nsIContent* mParent;

  
  
  
  
  nsIContent* mChild;

  
  
  
  
  nsIContent* mDefaultChild;

  
  
  
  nsAutoPtr<ExplicitChildIterator> mShadowIterator;

  
  
  
  
  uint32_t mIndexInInserted;

  
  bool mIsFirst;
};




class FlattenedChildIterator : public ExplicitChildIterator
{
public:
  FlattenedChildIterator(nsIContent* aParent);

  bool XBLInvolved() { return mXBLInvolved; }

private:
  
  
  bool mXBLInvolved;
};

} 
} 

#endif
