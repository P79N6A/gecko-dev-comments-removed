





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

  
  void Seek(nsIContent* aChildToFind)
  {
    
    
    

    nsIContent* child;
    do {
      child = GetNextChild();
    } while (child && child != aChildToFind);
  }

  bool XBLInvolved() { return mXBLInvolved; }

private:
  
  
  bool mXBLInvolved;
};

} 
} 

#endif
